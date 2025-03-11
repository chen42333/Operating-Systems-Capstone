#include "uart.h"
#include "interrupt.h"

struct t_q timer_queue;

void add_timer(void(*callback)(void*), uint64_t duration, void *data)
{
    uint64_t count, freq;
    struct timer_queue_element *element;
    int idx = timer_queue.producer_idx;

    // Wait until the timer queue is not full
    while ((timer_queue.producer_idx + 1) % TIMER_QUEUE_LEN == timer_queue.consumer_idx) ;

    asm volatile ("mrs %0, cntpct_el0" : "=r"(count));
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(freq));

    element = &timer_queue.buf[timer_queue.producer_idx++];
    element->handler = callback;
    element->cur_ticks = count;
    element->duration_ticks = duration;
    element->data = data;
    timer_queue.producer_idx %= TIMER_QUEUE_LEN;

    for (; idx >= timer_queue.consumer_idx; idx--)
    {
        if (idx == timer_queue.consumer_idx)
        {
            // Program a new timer interrupt
            asm volatile ("msr cntp_tval_el0, %0" :: "r"(duration));
            break;
        }
        
        if (count + duration < timer_queue.buf[idx - 1].cur_ticks + timer_queue.buf[idx - 1].duration_ticks)
        {
            // Move forward
            struct timer_queue_element tmp;
            
            tmp = timer_queue.buf[idx];
            timer_queue.buf[idx] = timer_queue.buf[idx - 1];
            timer_queue.buf[idx - 1] = tmp;
        }
        else
            break;
    }
}

void process_timer()
{
    uint64_t count, ival;
    struct timer_queue_element *element;

    if (timer_queue.producer_idx == timer_queue.consumer_idx) // The timer queue is empty (should not happen)
        return;
    
    element = &timer_queue.buf[timer_queue.consumer_idx++];
    timer_queue.consumer_idx %= TIMER_QUEUE_LEN;

    // Program the next timer interrupt
    asm volatile ("mrs %0, cntpct_el0" : "=r"(count));
    ival = element->cur_ticks + element->duration_ticks - count;
    asm volatile ("msr cntp_tval_el0, %0" :: "r"(ival));

    element->handler(element->data);
}

void elasped_time(void* data)
{
    uint64_t count, freq;

    asm volatile ("mrs %0, cntpct_el0" : "=r"(count));
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(freq));
    uart_write_dec(count / freq);
    uart_write_string(" seconds after booting\r\n");
    
    add_timer(elasped_time, 2 * freq, NULL);
}

void init_timer_queue()
{
    uint64_t freq;

    timer_queue.buf = (struct timer_queue_element*)simple_malloc(sizeof(struct timer_queue_element) * TIMER_QUEUE_LEN);
    timer_queue.producer_idx = 0;
    timer_queue.consumer_idx = 0;

    asm volatile ("mrs %0, cntfrq_el0" : "=r"(freq));
    add_timer(elasped_time, freq * 2, NULL);
}

void exception_entry()
{
    uint64_t value;

    asm volatile ("mrs %0, spsr_el1" : "=r"(value));
    uart_write_string("SPSR_EL1: ");
    uart_write_hex(value, sizeof(uint64_t));
    uart_write_string("\r\n");
    asm volatile ("mrs %0, elr_el1" : "=r"(value));
    uart_write_string("ELR_EL1: ");
    uart_write_hex(value, sizeof(uint64_t));
    uart_write_string("\r\n");
    asm volatile ("mrs %0, esr_el1" : "=r"(value));
    uart_write_string("ESR_EL1: ");
    uart_write_hex(value, sizeof(uint64_t));
    uart_write_string("\r\n");
}

void tx_int()
{
    char c;

    c = ring_buf_consume(&w_buf);
    set8(AUX_MU_IO_REG, c);
    
    if (ring_buf_empty(&w_buf))
        disable_write_int();
}

void rx_int()
{
    char c;

    c = get8(AUX_MU_IO_REG);
    ring_buf_produce(&r_buf, c);
}