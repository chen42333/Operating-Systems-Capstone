#include "uart.h"
#include "string.h"
#include "mem.h"
#include "interrupt.h"
#include "printf.h"
#include "process.h"
#include "syscall.h"
#include "vmem.h"

// For test exception and interrupt handler
// #define TEST_EXCEPTION
// #define TEST_INT

// For test preemption
// #define BLOCK_READ
// #define BLOCK_TIMER

#define EL0_FREQ_REG_ACCESS // Give EL0 the permission to access timer frequency register or not

volatile int irq_nested_count = 1; // DAIF is masked initially

struct ring_buf timer_queue, task_queue;
struct timer_queue_element timer_queue_buf[BUFLEN];
struct task_queue_element task_queue_buf[BUFLEN];
extern struct ring_buf r_buf, w_buf;
static enum prio cur_priority = INIT_PRIO;
bool need_schedule = false;

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

    if (need_schedule)
    {
        need_schedule = false;
        enable_int(); // Since it may switch to a process with DAIF unmasked, here should enable interrupt temporary
        schedule();
        disable_int(); // Make sure DAIF is masked after interrupt handler
    }
}

void process_timer(void *data)
{
    uint64_t count, ival;
    struct timer_queue_element *element_ptr, *next_element_ptr;

    element_ptr = (struct timer_queue_element*)data;

    element_ptr->handler(element_ptr->data);

    // Program the next timer interrupt
    if (!ring_buf_empty(&timer_queue))
    {
        next_element_ptr = &((struct timer_queue_element*)timer_queue.buf)[timer_queue.consumer_idx];
        asm volatile ("mrs %0, cntpct_el0" : "=r"(count));
        ival = next_element_ptr->cur_ticks + next_element_ptr->duration_ticks - count;
        asm volatile ("msr cntp_tval_el0, %0" :: "r"(ival));
    }
    
    enable_timer_int();
}

void elasped_time(void* data)
{
    uint64_t freq;

    asm volatile ("mrs %0, cntfrq_el0" : "=r"(freq));
    // add_timer(elasped_time, freq / 1000 * TIMER_INT, NULL);
    add_timer(elasped_time, freq >> 5, NULL);
    need_schedule = true;
#ifdef TEST_INT
    uint64_t count;

    asm volatile ("mrs %0, cntpct_el0" : "=r"(count));
    printf("%ds\r\n", count / freq);
#endif
#ifdef BLOCK_TIMER
    while (true) ; // For test preemption
#endif
}

void print_msg(void *data)
{
    printf("%s\r\n", data);
    free(data);
}

void init_timer_queue()
{
    uint64_t freq;

    ring_buf_init(&timer_queue, timer_queue_buf);

    asm volatile ("mrs %0, cntfrq_el0" : "=r"(freq));
    add_timer(elasped_time, freq / 1000 * TIMER_INT, NULL);
}

void core_timer_enable()
{
#ifdef EL0_FREQ_REG_ACCESS
    uint64_t tmp;
    asm volatile("mrs %0, cntkctl_el1" : "=r"(tmp));
    tmp |= 1;
    asm volatile("msr cntkctl_el1, %0" : : "r"(tmp));
#endif
    enable_timer_int();
    init_timer_queue();
    set32(CORE0_TIMER_IRQ_CTRL, 2); // unmask timer interrupt
}

void exception_entry()
{
   #ifdef TEST_EXCEPTION
    size_t value;

    asm volatile ("mrs %0, spsr_el1" : "=r"(value));
    printf("SPSR_EL1: 0x%x\r\n", value);
    asm volatile ("mrs %0, elr_el1" : "=r"(value));
    printf("ELR_EL1: 0x%x\r\n", value);
    asm volatile ("mrs %0, esr_el1" : "=r"(value));
    printf("ESR_EL1: 0x%x\r\n", value);

#endif
    size_t esr, ec;

    asm volatile ("mrs %0, esr_el1" : "=r"(esr));
    ec = (esr & EC_MASK) >> 26;

    if (ec == EC_DATA_ABORT || ec == EC_INSTRUCTION_ABORT)
    {
        size_t dfsc;
        void *v_fault_addr;

        dfsc = (esr & DFSC_MASK) >> 2;
        asm volatile("mrs %0, far_el1" : "=r"(v_fault_addr));

        if (dfsc == DFSC_TRANSLATION)
        {
            struct pcb_t *pcb = get_current();
            struct section *s = list_find(&pcb->sections, in_section, v_fault_addr);

            if (s)
            {
                void *v_ttbr = p2v_trans_kernel(pcb->ttbr);
                size_t s_page_idx, e_page_idx;
                void *physical_addr;

                switch (s->type)
                {
                    case TEXT:
                        s_page_idx = (size_t)USR_CODE_START / PAGE_SIZE;
                        e_page_idx = s_page_idx + (s->size - 1) / PAGE_SIZE + 1;
                        physical_addr = pcb->code;
                        fill_page_table(v_ttbr, s_page_idx, e_page_idx, (size_t)physical_addr, PD_USER_ACCESS | PD_ACCESS); 
                        for (int i = 0; i < s->size; i += PAGE_SIZE)
                            ref_page(pcb->code + i);
                        invalidate_tlb();
                        break;
                    case STACK:
                    case HEAP:
                        s_page_idx = (size_t)v_fault_addr / PAGE_SIZE;
                        e_page_idx = s_page_idx + 1;
                        physical_addr = v2p_trans_kernel(malloc(PAGE_SIZE));
                        fill_page_table(v_ttbr, s_page_idx, e_page_idx, (size_t)physical_addr, PD_NX_EL0 | PD_USER_ACCESS | PD_ACCESS);
                        ref_page(physical_addr);
                        invalidate_tlb();
                        break;
                    case DEVICE:
                    default:
                        goto seg_fault;
                    break;
                }

                enable_int();
                printf("[Translation fault]: 0x%lx\r\n", v_fault_addr);
                disable_int();
            }
            else
                goto seg_fault;

            return;
        } else if (dfsc == DFSC_PERMISSION) {
            void *fault_addr = v2p_trans(v_fault_addr, NULL);
            void *fault_page = (void*)((size_t)fault_addr & ~(PAGE_SIZE - 1));
            
            if (get_w_permission(fault_page)) // COW
            {
                if (get_ref_count(fault_page) > 1)
                {
                    void *v_fault_page = p2v_trans_kernel(fault_page);
                    void *v_new_page = malloc(PAGE_SIZE);
                    void *new_page = v2p_trans_kernel(v_new_page);

                    memcpy(v_new_page, v_fault_page, PAGE_SIZE);
                    replace_page_entry(NULL, v_fault_addr, new_page, true);
                    invalidate_tlb();

                    deref_page(fault_page);
                    ref_page(new_page);

                    enable_int();
                    printf("[Copy-on-write]: 0x%lx\r\n", v_fault_addr);
                    disable_int();
                } else {
                    replace_page_entry(NULL, v_fault_addr, fault_addr, true);
                    invalidate_tlb();
                }
            }
            else
                goto seg_fault;

            return;
        }

seg_fault: 
        enable_int();
        printf("[Segmentation fault]: Kill Process\r\n");
        exit();
        disable_int();
    } else {
        enable_int();
        printf("ESR: 0x%lx\r\nEC, 0x%lx\r\n", esr, ec);
        disable_int();
    }
}

void process_task(struct task_queue_element *task) // Interrupt is disabled after process_task()
{
    struct task_queue_element element;
    enum prio tmp = cur_priority;

    if (task != NULL)
        element = *task;
    else 
    {
        if (ring_buf_empty(&task_queue) || cur_priority < ((struct task_queue_element*)task_queue.buf)[task_queue.consumer_idx].priority)
            return;
        
        ring_buf_consume(&task_queue, &element, TASK);
    }

    cur_priority = element.priority;
    
    enable_int(); // Execute interrupt handler with enabled interrupt to allow preemption
    element.handler(element.data);

    disable_int();
    cur_priority = tmp; // Critical section
    process_task(NULL);
}

void tx_int_task(void *data)
{
    set8(AUX_MU_IO_REG, *(char*)data);

    if (!list_empty(&wait_queue[WRITE]) && 
    ring_buf_remain_e(&w_buf) >= ((struct pcb_t*)list_top(&wait_queue[WRITE]))->wait_data)
        wait_to_ready(list_pop(&wait_queue[WRITE]));
    
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
    while (true) ; // For test preemption
#endif
    ring_buf_produce(&r_buf, (char*)data, CHAR);

    if (!list_empty(&wait_queue[READ]) && 
    ring_buf_num_e(&r_buf) >= ((struct pcb_t*)list_top(&wait_queue[READ]))->wait_data)
        wait_to_ready(list_pop(&wait_queue[READ]));

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

    data = malloc(strlen(msg) + 1);
    strcpy((char*)data, msg);
    
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(freq));
    add_timer(print_msg, freq * sec, data);

    return 0;
}