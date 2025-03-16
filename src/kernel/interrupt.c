#include "uart.h"
#include "string.h"
#include "mem.h"
#include "interrupt.h"

// For test exception and interrupt handler
// #define TEST_EXCEPTION
// #define TEST_INT

// For test preemption
// #define BLOCK_READ
// #define BLOCK_TIMER

struct ring_buf timer_queue, task_queue;
extern struct ring_buf r_buf, w_buf;
static enum prio cur_priority = INIT_PRIO;

void add_timer(void(*callback)(void*), uint64_t duration, void *data)
{
    uint64_t count;
    struct timer_queue_element element;
    struct timer_queue_element *buf = (struct timer_queue_element*)timer_queue.buf;
    int idx = timer_queue.producer_idx;

    // Wait until the timer queue is not full
    while (ring_buf_full(&timer_queue)) ;

    asm volatile ("mrs %0, cntpct_el0" : "=r"(count));

    element.handler = callback;
    element.cur_ticks = count;
    element.duration_ticks = duration;
    element.data = data;
    ring_buf_produce(&timer_queue, &element, TIMER);

    for (; ; idx--)
    {
        int cmp_idx = (idx + BUFLEN - 1) % BUFLEN;

        if (idx == timer_queue.consumer_idx)
        {
            // Program a new timer interrupt
            asm volatile ("msr cntp_tval_el0, %0" :: "r"(duration));
            break;
        }
        
        if (count + duration < buf[cmp_idx].cur_ticks + buf[cmp_idx].duration_ticks)
        {
            // Move forward
            struct timer_queue_element tmp;
            
            tmp = buf[idx];
            buf[idx] = buf[cmp_idx];
            buf[cmp_idx] = tmp;
        }
        else
            break;

        if (idx == 0)
            idx += BUFLEN;
    }
}

void add_task(void(*callback)(void*), enum prio priority, void *data)
{
    struct task_queue_element element;

    element.handler = callback;
    element.priority = priority;
    element.data = data;

    if (priority < cur_priority) // Preempt
        process_task(&element);
    else
    {
        struct task_queue_element *buf = (struct task_queue_element*)task_queue.buf;
        int idx = task_queue.producer_idx;

        // Wait until the task queue is not full
        while (ring_buf_full(&task_queue)) ;

        ring_buf_produce(&task_queue, &element, TASK);

        for (; idx != task_queue.consumer_idx; idx--)
        {
            int cmp_idx = (idx + BUFLEN - 1) % BUFLEN;
            if (priority < buf[cmp_idx].priority)
            {
                // Move forward
                struct task_queue_element tmp;
                
                tmp = buf[idx];
                buf[idx] = buf[cmp_idx];
                buf[cmp_idx] = tmp;
            }
            else
                break;

            if (idx == 0)
                idx += BUFLEN;
        }
    }
}

void timer_int()
{
    struct timer_queue_element timer_data;

    disable_timer_int();

    if (ring_buf_empty(&timer_queue)) // The timer queue is empty (should not happen)
        return;

    ring_buf_consume(&timer_queue, &timer_data, TIMER);
    add_task(process_timer, TIMER_PRIO, &timer_data);

    process_task(NULL);
}

void process_timer(void *data)
{
    uint64_t count, ival;
    struct timer_queue_element *element_ptr, *next_element_ptr;

    element_ptr = (struct timer_queue_element*)data;

    // Program the next timer interrupt
    if (!ring_buf_empty(&timer_queue))
    {
        next_element_ptr = &((struct timer_queue_element*)timer_queue.buf)[timer_queue.consumer_idx];
        asm volatile ("mrs %0, cntpct_el0" : "=r"(count));
        ival = next_element_ptr->cur_ticks + next_element_ptr->duration_ticks - count;
        asm volatile ("msr cntp_tval_el0, %0" :: "r"(ival));
    }

    element_ptr->handler(element_ptr->data);
    enable_timer_int();
}

void elasped_time(void* data)
{
#ifdef TEST_INT

    uint64_t count, freq;

    asm volatile ("mrs %0, cntpct_el0" : "=r"(count));
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(freq));
    add_timer(elasped_time, 2 * freq, NULL);
    uart_write_dec(count / freq);
    uart_write_string(" seconds after booting\r\n");
#ifdef BLOCK_TIMER
    // while (true) ; // For test preemption
#endif

#endif
}

void print_msg(void *data)
{
    uart_write_string((char*)data);
    uart_write_newline();
}

void init_timer_queue()
{
    uint64_t freq;

    ring_buf_init(&timer_queue, TIMER);

    asm volatile ("mrs %0, cntfrq_el0" : "=r"(freq));
    add_timer(elasped_time, freq * 2, NULL);
}

void core_timer_enable()
{
    enable_timer_int();
    init_timer_queue();
    set32(CORE0_TIMER_IRQ_CTRL, 2); // unmask timer interrupt
}

void exception_entry()
{
#ifdef TEST_EXCEPTION

    uint64_t value;

    asm volatile ("mrs %0, spsr_el1" : "=r"(value));
    uart_write_string("SPSR_EL1: ");
    uart_write_hex(value, sizeof(uint64_t));
    uart_write_newline();
    asm volatile ("mrs %0, elr_el1" : "=r"(value));
    uart_write_string("ELR_EL1: ");
    uart_write_hex(value, sizeof(uint64_t));
    uart_write_newline();
    asm volatile ("mrs %0, esr_el1" : "=r"(value));
    uart_write_string("ESR_EL1: ");
    uart_write_hex(value, sizeof(uint64_t));
    uart_write_newline();

#endif
}

void process_task(struct task_queue_element *task)
{
    struct task_queue_element element;
    enum prio tmp = cur_priority;

    if (task != NULL)
        element = *task;
    else 
    {
        if (ring_buf_empty(&task_queue) || cur_priority < ((struct task_queue_element*)task_queue.buf)[task_queue.consumer_idx].priority)
        {
            asm volatile ("msr DAIFClr, 0b10"); // Enable interrupt
            return;
        }    
        
        ring_buf_consume(&task_queue, &element, TASK);
    }

    cur_priority = element.priority;
    
    asm volatile ("msr DAIFClr, 0b10"); // Enable interrupt
    element.handler(element.data);

    asm volatile ("msr DAIFSet, 0b10"); // Disable interrupt
    cur_priority = tmp; // Critical section
    process_task(NULL);
}

void tx_int_task(void *data)
{
    set8(AUX_MU_IO_REG, *(char*)data);
    
    if (!ring_buf_empty(&w_buf))
        enable_write_int();
}

void tx_int()
{
    char write_data;

    disable_write_int();

    ring_buf_consume(&w_buf, &write_data, CHAR);
    add_task(tx_int_task, WRITE_PRIO, &write_data);

    process_task(NULL);
}

void rx_int_task(void *data)
{
#ifdef BLOCK_READ
    // while (true) ; // For test preemption
#endif
    ring_buf_produce(&r_buf, (char*)data, CHAR);
    enable_read_int();
}

void rx_int()
{
    char read_data;

    disable_read_int();

    read_data = get8(AUX_MU_IO_REG);
    add_task(rx_int_task, READ_PRIO, &read_data);

    process_task(NULL);
}

int set_timeout()
{
    char *msg, *sec_str, *tail;
    void *data;
    uint32_t sec;
    uint64_t freq;

    if ((msg = strtok(NULL, " ")) == NULL)
        return -1;

    if ((sec_str = strtok(NULL, " ")) == NULL)
        return -1;
    
    if ((tail = strtok(NULL, "")) != NULL)
        return -1;
    
    if ((sec = str2u32(sec_str, strlen(sec_str))) == 0)
        return -1;

    data = simple_malloc(strlen(msg) + 1);
    strcpy((char*)data, msg);
    
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(freq));
    add_timer(print_msg, freq * sec, data);

    return 0;
}