/* file:          ucx.c
 * description:   UCX/OS kernel
 * date:          04/2021
 * author:        Sergio Johann Filho <sergio.johann@acad.pucrs.br>
 */

#include <ucx.h>

struct kcb_s kernel_state = {
	.tasks = 0,
	.task_current = 0,
	.rt_sched = krnl_noop_rtsched, 
	.id_next = 0,
	.ticks = 0,
	.timer_lst = 0,
};
	
struct kcb_s *kcb = &kernel_state;

/* kernel auxiliary functions */
static void stack_check(void)
{
	struct tcb_s *task = kcb->task_current->data;
	uint32_t check = 0x33333333;
	uint32_t *stack_p = (uint32_t *)task->stack;

	if (*stack_p != check) {
		printf("\n*** task %d, stack: 0x%p (size %d)\n", task->id,
			task->stack, task->stack_sz);
		krnl_panic(ERR_STACK_CHECK);
	}
		
}

static struct node_s *delay_update(struct node_s *node, void *arg)
{
	struct tcb_s *task = node->data;
	
	if (task->state == TASK_BLOCKED && task->delay > 0) {
		task->delay--;
		if (task->delay == 0)
			task->state = TASK_READY;
	}
	
	return 0;
}

static struct node_s *idcmp(struct node_s *node, void *arg)
{
	struct tcb_s *task = node->data;
	uint16_t id = (size_t)arg;
	
	if (task->id == id)
		return node;
	else
		return 0;
}

static struct node_s *refcmp(struct node_s *node, void *arg)
{
	struct tcb_s *task = node->data;
	
	if (task->task == arg)
		return node;
	else
		return 0;
}

void krnl_panic(uint32_t ecode)
{
	int err;
	
	_di();
	printf("\n*** HALT (%d)", ecode);
	for (err = 0; perror[err].ecode != ERR_UNKNOWN; err++)
		if (perror[err].ecode == ecode) break;
	printf(" - %s\n", perror[err].desc);
	
	for (;;);
}


/* 
 * Kernel scheduler and dispatcher 
 * actual dispatch/yield implementation may be platform dependent
 */
 
void dispatch(void) __attribute_ ((weak, alias ("dispatch")));
void yield(void) __attribute_ ((weak, alias ("yield")));

/*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * The scheduler switches tasks based on task states and priorities, using
 * a priority driven round robin algorithm. Current interrupted task is checked
 * for its state and if RUNNING, it is changed to READY. Task priority is
 * kept in the TCB entry in 16 bits, where the 8 MSBs? hold the task
 * priority and the 8 LSBs? keep current task priority, decremented on each
 * round - so high priority tasks have a higher chance of 'winning' the cpu.
 * Only a task on the READY state is considered as a viable option. NOTICE - 
 * if no task is ready (e.g. no 'idle' task added to the system and no other
 * task ready) the kernel will hang forever in that inner do .. while loop
 * and this is fine, as there is no hope in such system.
 * 
 * In the end, a task is selected for execution, has its priority reassigned
 * and its state changed to RUNNING.
 */


int32_t krnl_noop_rtsched(void)
{
	return -1;
}

//======================================================================================================

void fail_lsf_panic(uint32_t ecode, struct tcb_s *task, int tempo_atual)
{
	int err;
	
	_di();
	printf("\n*[ERR_LSF] O algoritimo do LSF deu errado:\n");
	printf("Task ID (%d)\n", (int)task->id);
	printf("At time rt(%d)\n", tempo_atual);
	printf("At time ticks(%d)\n", kcb->ticks);
	for (err = 0; perror[err].ecode != ERR_UNKNOWN; err++)
		if (perror[err].ecode == ecode) break;
	printf(" - %s\n", perror[err].desc);
	
	for (;;);
}


uint16_t krnl_schedule(void) {
	uint8_t first_lst_zero = 0x00;
	uint8_t LSB, temp = 0;
	uint8_t high = 0x00;
	uint16_t id = -1;
	//----------------------------------------------------
	struct node_s *cursor  = kcb->tasks->head->next; 
	struct node_s *idle_node  = kcb->tasks->head->next; 
	struct node_s *best_node  = kcb->tasks->head->next;
	struct tcb_s *task_with_higher_priority = cursor->data;
	struct tcb_s *task_with_starvation = cursor->data;
	struct tcb_s *task_idle = cursor->data;
	struct tcb_s *tcb = cursor->data;
	//----------------------------------------------------
	struct tcb_s *t = kcb->task_current->data;
	if (t->state == TASK_RUNNING) t->state = TASK_READY;
    //----------------------------------------------------
    task_with_higher_priority = NULL;
    task_with_starvation = NULL;
    task_idle = NULL;
    best_node = NULL;
    cursor  = kcb->tasks->head;
    //----------------------------------------------------
	while(cursor){
		//printf("ID : %d", (int)tcb->id);
		if (tcb->priority == TASK_IDLE_PRIO) { 
			task_idle = tcb;
			//printf("ACHEI A IDLE EM ID : %d", (int)tcb->id);
			idle_node = cursor;
		}
		else{
			LSB = tcb->priority & 0xff;
			
			if (tcb->rt_prio == 0 && tcb->priority != TASK_REALTIME_PRIO && tcb->state == TASK_READY) {
				//printf("LSB %u\n", LSB);
				if (LSB > high) {
					high = LSB;
					//printf("high %u\n", high);
					task_with_higher_priority = tcb;
					best_node = cursor;
				}
				if(LSB==0x00){
					if (first_lst_zero != 0x00){
						task_with_starvation = tcb;
						first_lst_zero = 0xff;
						best_node = cursor;
						break;
					}
				}
			}
		}
		cursor = cursor->next;	
		if(cursor != NULL)
			tcb = cursor->data;
	} 

	if (task_with_starvation != NULL) {
		id = task_with_starvation->id;
		kcb->task_current = best_node;
		task_with_starvation->priority |= (task_with_starvation->priority >> 8) & 0xff;
		task_with_starvation->state = TASK_RUNNING;
		task_with_starvation->priority = ((0x1f << 8) | 0x1f); // RESET
		printf("ID task_with_starvation %d \n", (int)id);
	}
	
	else if (task_with_higher_priority != NULL) {
		id = task_with_higher_priority->id;
		task_with_higher_priority->state = TASK_RUNNING;
		kcb->task_current = best_node;
		LSB = task_with_higher_priority->priority & 0xff;
		//printf("LSB %u\n", LSB);
		temp = LSB - 1;
		//printf("TEMP %u\n", temp);
		if (temp > 0 && temp <= 0x1f){
			task_with_higher_priority->priority = ((0x1f << 8) | (uint8_t)temp);
		}
		else task_with_higher_priority->priority = ((0x1f << 8) | 0x1f); // RESET
		printf("ID task_with_higher_priority %d\n", (int)id);
		printf("prioridade %d\n", (int)task_with_higher_priority->priority);
	}
	else if(task_idle !=NULL){
		id = task_idle->id;
		//printf("IDLE ID: %d\n", (int)id);
		task_idle->state = TASK_RUNNING;
		kcb->task_current = idle_node;
	}
	else{
		printf("Nenhuma TASK disponivel.... nem a IDLE\n");
	}
	
	return id;
}

void timer_hp(void){
	int temp =  1 + (int)kcb->ticks_hp;
	kcb->ticks_hp = temp;
	//printf("kcb->ticks_hp %d\n", (int)kcb->ticks_hp);
	
}
int ticks_h(void){
	//return kcb->ticks_hp;
	return kcb->ticks;
}

struct lstf_parameter* ucx_task_ifno(void)
{
	if (!kcb || !kcb->task_current || !kcb->task_current->data)
		return NULL;

	struct tcb_s *task = (struct tcb_s *)kcb->task_current->data;

	if (task->rt_prio != 0) {
		return (struct lstf_parameter *)task->rt_prio; // cast direto, sem criar variável extra
	}

	return NULL;
}

//======================================================================================================
/*  
 * Kernel task dispatch and yield routines. This is highly platform dependent,
 * so it is implemented by generic calls to _dispatch() and _yield(), defined
 * (or not) in a platform HAL. When not implemented, both dispatch() and yield(),
 * defined as weak symbols will be used by default, and context switch based on
 * setjmp() and longjmp() calls is implemented. On the other hand, if there is
 * specific hardware support or expectations for the context switch, the mechanism
 * should be defined in the HAL, implementing both _dispatch() and _yield().
 * 
 * You are not expected to understand this.
 */

void krnl_dispatcher(void)
{
	kcb->ticks++;
	CRITICAL_ENTER();
	_dispatch();
	CRITICAL_LEAVE();
}

void dispatch(void)
{	
	//printf("dispatcher\n");
	struct tcb_s *task = (struct tcb_s *)kcb->task_current->data;
	// Marca a tarefa atual como pronta se estiver rodando
	// if (task->state == TASK_RUNNING) task->state = TASK_READY;
	//printf("10\n");
	if (!kcb->tasks->length)
		krnl_panic(ERR_NO_TASKS);
	//printf("11\n");
	if (!setjmp(task->context)) {
		stack_check();
		//printf("12\n");
		list_foreach(kcb->tasks, delay_update, (void *)0);
		//printf("13\n");
		if (kcb->rt_sched() == -1)  
			krnl_schedule();
		//printf("14\n");
		task = kcb->task_current->data;
		//printf("15\n");
		_interrupt_tick();
		//printf("16\n");
		longjmp(task->context, 1);
		//printf("17\n");
	}
	//printf("18\n");
}

void yield(void)
{
	struct tcb_s *task = kcb->task_current->data;
	
	if (!kcb->tasks->length)
		krnl_panic(ERR_NO_TASKS);
	
	if (!setjmp(task->context)) {
		stack_check();
		if (kcb->preemptive == 'n')
			list_foreach(kcb->tasks, delay_update, (void *)0);
		krnl_schedule();
		task = kcb->task_current->data;
		longjmp(task->context, 1);
	}
}

/* Task Management API */
/* The ucx_task_spawn function creates a new task in the UCX kernel.
 * It allocates memory for the task's control block (TCB) and stack,
 * initializes the TCB, and adds it to the global task list.
 * 
 * Parameters:
 * - task: Pointer to the function that will be executed by the task.
 * - stack_size: Size of the stack to be allocated for the task.
 * 
 * Returns:
 * - The ID of the newly created task on success.
 * - An error code on failure.
 * */
//int ucx_task_spawn(void (*task)(void), uint16_t stack_size) ?
uint16_t ucx_task_spawn(void *task, uint16_t stack_size)
{
	struct tcb_s *new_tcb;
	struct node_s *new_task;
	//printf("LENGHT %d\n", (int)kcb->tasks->length);
	// Allocate new Task Control Block (TCB)
	new_tcb = malloc(sizeof(struct tcb_s));
	if (!new_tcb)
		krnl_panic(ERR_TCB_ALLOC);

	// Allocate stack before inserting TCB into the task list
	new_tcb->stack = malloc(stack_size);
	if (!new_tcb->stack) {
		free(new_tcb);  // prevent memory leak on failure
		krnl_panic(ERR_STACK_ALLOC);
	}

	CRITICAL_ENTER(); // Enter critical section to protect task list

	// Insert the new task into the global task list
	new_task = list_pushback(kcb->tasks, new_tcb); // [1]
	if (!new_task) {
		free(new_tcb->stack); // talvez n...
		free(new_tcb);
		krnl_panic(ERR_TCB_ALLOC);
	}

	// Initialize TCB fields
	new_task->data     = new_tcb; //[1]
	new_tcb->task      = task;//n era pra ser callback?
	new_tcb->rt_prio   = 0;
	new_tcb->delay     = 0;
	new_tcb->stack_sz  = stack_size;
	new_tcb->id        = kcb->id_next++;  // Assign unique task ID
	new_tcb->state     = TASK_STOPPED;
	new_tcb->priority  = TASK_NORMAL_PRIO;


	CRITICAL_LEAVE(); // Exit critical section

	memset(new_tcb->stack, 0x69, stack_size);                     // Stack padding pattern
	memset(new_tcb->stack, 0x33, 4);                              // Canary at stack base
	memset((uint8_t *)new_tcb->stack + stack_size - 4, 0x33, 4);  // Canary at stack top

	// Initialize execution context for the new task
	// Casting to (size_t) ensures compatibility across 32-bit and 64-bit architectures
	_context_init(&new_tcb->context, (size_t)new_tcb->stack, stack_size, (size_t)task);

	printf("task %d: 0x%p, stack: 0x%p, size %d\n", (int)new_tcb->id,
	        new_tcb->task, new_tcb->stack, new_tcb->stack_sz);

	// Mark task as ready to be scheduled
	new_tcb->state = TASK_READY;

	return new_tcb->id;
}

int32_t ucx_task_cancel(uint16_t id)
{
	struct node_s *node;
	struct tcb_s *task;
	
	if (id == ucx_task_id())
		return ERR_TASK_CANT_REMOVE;

	CRITICAL_ENTER();
	node = list_foreach(kcb->tasks, idcmp, (void *)(size_t)id);
	
	if (!node) {
		CRITICAL_LEAVE();
		
		return ERR_TASK_NOT_FOUND;
	}
	
	task = node->data;
	free(task->stack);
	free(task);
	
	list_remove(kcb->tasks, node);
	CRITICAL_LEAVE();
	
	return ERR_OK;
}

void ucx_task_yield()
{
	_yield();
}

void ucx_task_delay(uint16_t ticks)		// FIXME: delay any task
{
	struct tcb_s *task;
	
	CRITICAL_ENTER();
	task = kcb->task_current->data;
	task->delay = ticks;
	task->state = TASK_BLOCKED;
	CRITICAL_LEAVE();
	ucx_task_yield();
}

int32_t ucx_task_suspend(uint16_t id)
{
	struct node_s *node;
	struct tcb_s *task;

	CRITICAL_ENTER();
	node = list_foreach(kcb->tasks, idcmp, (void *)(size_t)id);
	
	if (!node) {
		CRITICAL_LEAVE();
		return ERR_TASK_NOT_FOUND;
	}

	task = node->data;
	if (task->state == TASK_READY || task->state == TASK_RUNNING) {
		task->state = TASK_SUSPENDED;
	} else {
		CRITICAL_LEAVE();
		
		return ERR_TASK_CANT_SUSPEND;
	}
	CRITICAL_LEAVE();
	
	if (kcb->task_current == node)
		ucx_task_yield();

	return ERR_OK;
}

int32_t ucx_task_resume(uint16_t id)
{
	struct node_s *node;
	struct tcb_s *task;

	CRITICAL_ENTER();
	node = list_foreach(kcb->tasks, idcmp, (void *)(size_t)id);
	
	if (!node) {
		CRITICAL_LEAVE();
		
		return ERR_TASK_NOT_FOUND;
	}

	task = node->data;
	if (task->state == TASK_SUSPENDED) {
		task->state = TASK_READY;
	} else {
		CRITICAL_LEAVE();
		
		return ERR_TASK_CANT_RESUME;
	}
	CRITICAL_LEAVE();

	return ERR_OK;
}

int32_t ucx_task_priority(uint16_t id, uint16_t priority)
{
	struct node_s *node;
	struct tcb_s *task;

	switch (priority) {
	case TASK_CRIT_PRIO:
	case TASK_REALTIME_PRIO:
	case TASK_HIGH_PRIO:
	case TASK_ABOVE_PRIO:
	case TASK_NORMAL_PRIO:
	case TASK_BELOW_PRIO:
	case TASK_LOW_PRIO:
	case TASK_IDLE_PRIO:
		break;
	default:
		return ERR_TASK_INVALID_PRIO;
	}

	CRITICAL_ENTER();
	// Pra q converter pra size_t (void *)(size_t)id?
	// Convertemos o inteiro 'id' para (void *) usando (size_t) como cast intermediário.
	// Isso garante que o valor caiba no ponteiro com segurança, especialmente em sistemas 64 bits,
	// onde sizeof(void *) > sizeof(int). Esse padrão evita warnings e comportamento indefinido,
	// mantendo portabilidade e compatibilidade entre arquiteturas.

	node = list_foreach(kcb->tasks, idcmp, (void *)(size_t)id);//Callback = Dont't repeat yourself(DIY)
	
	if (!node) {
		CRITICAL_LEAVE();
		return ERR_TASK_NOT_FOUND;
	}

	task = node->data;
	task->priority = priority;
	CRITICAL_LEAVE();

	return ERR_OK;
}

int32_t ucx_task_rt_priority(uint16_t id, void *priority)
{
	struct node_s *node;
	struct tcb_s *task;

	if (!priority)
		return ERR_TASK_INVALID_PRIO;

	CRITICAL_ENTER();
	node = list_foreach(kcb->tasks, idcmp, (void *)(size_t)id);
	
	if (!node) {
		CRITICAL_LEAVE();
		return ERR_TASK_NOT_FOUND;
	}
	
	task = node->data;
	task->rt_prio =  priority;
	printf("task->rt_prio %d\n", (int)task->rt_prio);
	task->priority = TASK_REALTIME_PRIO;
	CRITICAL_LEAVE();

	return ERR_OK;
}

uint16_t ucx_task_id()
{
	struct tcb_s *task = kcb->task_current->data;
	
	return task->id;
}

int32_t ucx_task_idref(void *task)
{
	struct node_s *node;
	struct tcb_s *task_tcb;

	CRITICAL_ENTER();
	node = list_foreach(kcb->tasks, refcmp, task);
	
	if (!node) {
		CRITICAL_LEAVE();
		
		return ERR_TASK_NOT_FOUND;
	}
	
	task_tcb = node->data;
	CRITICAL_LEAVE();
	
	return task_tcb->id;	
}

void ucx_task_wfi()//way for interrupt

// {
// 	/* this routine is blocking and must be called inside a task in a preemptive context. */
// 	CRITICAL_ENTER();
// 	_yield();
// 	CRITICAL_LEAVE();
// }

{
	volatile uint32_t s;
	
	if (kcb->preemptive == 'n')
		return;
	
	s = kcb->ticks;
	while (s == kcb->ticks);
}

uint16_t ucx_task_count()
{
	return kcb->tasks->length;
}

uint32_t ucx_ticks()
{
	return kcb->ticks;
}

uint64_t ucx_uptime()
{
	return _read_us()/1000;
}