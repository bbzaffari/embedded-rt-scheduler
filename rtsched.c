#include <ucx.h>
#define STOP_TIME 13
#define KILL_IF_DEADLINE_MISS 0
//#define DEBUG
/* application tasks */
void task3(void)
{
	struct lstf_parameter *params;
	while (1) {
		params = ucx_task_ifno();
		printf("\n-----------------------------------------------------------------------\n");
		printf("-RUNNING \"task3\" <------------------------------------------------------\n");
		printf("|TASK ID: %u | ", ucx_task_id());
		if (params != NULL) {
			printf("Comp.: %u | ", params->computation);
			printf("Per.: %u | ", params->period);
			printf("Dl.: %u | ", params->deadline);
			printf("Slack: %u | ", params->slack);
			printf("Remaining: %u |\n", params->remaining);
		}
		else {
			printf("[!] Parâmetros indisponíveis para esta tarefa.\n");
		}
		ucx_task_wfi();
	}
}

void task2(void)
{
	struct lstf_parameter *params;
	while (1) {
		params = ucx_task_ifno();
		printf("\n-----------------------------------------------------------------------\n");
		printf("-RUNNING \"task2\" <------------------------------------------------------\n");
		printf("|TASK ID: %u | ", ucx_task_id());
		if (params != NULL) {
			printf("Comp.: %u | ", params->computation);
			printf("Per.: %u | ", params->period);
			printf("Dl.: %u | ", params->deadline);
			printf("Slack: %u | ", params->slack);
			printf("Remaining: %u |\n", params->remaining);
		}
		else {
			printf("[!] Parâmetros indisponíveis para esta tarefa.\n");
		}
		
		ucx_task_wfi();
	}
}

void task1(void)
{
	struct lstf_parameter *params;
	while (1) {
		params = ucx_task_ifno();
		printf("\n-----------------------------------------------------------------------\n");
		printf("-RUNNING \"taks1\" <-----------------------------------------------------\n");
		printf("|TASK ID: %u | ", ucx_task_id());
		if (params != NULL) {
			printf("Comp.: %u | ", params->computation);
			printf("Per.: %u | ", params->period);
			printf("Dl.: %u | ", params->deadline);
			printf("Slack: %u | ", params->slack);
			printf("Remaining: %u |\n", params->remaining);
		}
		else {
			printf("[!] Parâmetros indisponíveis para esta tarefa.\n");
		}
		ucx_task_wfi();
	}
}

void task0(void)
{
	struct lstf_parameter *params;
	while (1) {
		params = ucx_task_ifno();
		printf("\n-----------------------------------------------------------------------\n");
		printf("-RUNNING \"taks0\" <-----------------------------------------------------\n");
		printf("|TASK ID: %u | ", ucx_task_id());
		if (params != NULL) {
			printf("Comp.: %u | ", params->computation);
			printf("Per.: %u | ", params->period);
			printf("Dl.: %u | ", params->deadline);
			printf("Slack: %u | ", params->slack);
			printf("Remaining: %u |\n", params->remaining);
		}
		else {
			printf("[!] Parâmetros indisponíveis para esta tarefa.\n");
		}
		ucx_task_wfi();
	}
}

void task_idle(void)
{
	while (1) {
		if(kcb->ticks != 0) {
			printf("\n-----------------------------------------------------------------------\n");
			printf("-RUNNING \"task_idle\" <-----------------------------------------------------\n");
			printf("TASK ID: %u\n", ucx_task_id());
		}
		ucx_task_wfi(); // ou ucx_task_yield();?
	}
}

int32_t lstf_sched(void){
	struct node_s *LS_node = kcb->tasks->head->next;
	struct node_s *atual = kcb->tasks->head->next; //inicio
	struct lstf_parameter *parameters = NULL;
	struct tcb_s *LS_task = atual->data; 
	struct tcb_s *task= atual->data;
	int tempo_atual, tempo_relativo = 0;
	int ls = 255;	// UINT_MAX;
	int temp = 0;
	tempo_atual = (int)ticks_h(); 
	//--------------------------------------------------
	struct tcb_s *t = kcb->task_current->data;
	if (t->state == TASK_RUNNING) t->state = TASK_READY;
	//--------------------------------------------------
	printf("\n-----------------------------------------------------------------------\n");
	printf("=TEMPO TICKS %02d = \n", tempo_atual -1);
	LS_node = NULL;
	LS_task = NULL;
	while (atual) {
		if(task->rt_prio != 0 && task->priority !=TASK_IDLE_PRIO && task->state == TASK_READY ){
			parameters = (struct lstf_parameter *)task->rt_prio;
			tempo_relativo = (tempo_atual-1) % parameters->period;//(tempo_atual-1) pois no segundo 0 tem o IDLE
			// --------------------------------------------------------------------------------
			//DEBUG
			printf("---------------------------------------------------\n");
			printf("|Calculating Slack of TASK_ID %02d|\n", (int)task->id);
			if(parameters->remaining != 0 ){
				if (tempo_relativo== 0 && KILL_IF_DEADLINE_MISS ==0){	// RESTART parameters->remaining 
					parameters->remaining = parameters->computation;
					temp = (int)parameters->deadline - (int)parameters->computation; 
					printf("----->!!!!!!!!!!!!!!!DEADLINEMISS!!!!!!!!!!!!!!! ID %d\n", (int)task->id);
					printf("----->\"RESET\" TASK_ID %d\n", (int)task->id);
					parameters->slack =  (uint8_t)temp;	
				}
				
				temp = (int)parameters->deadline - (int)parameters->remaining - (int)tempo_relativo; 
				if (temp >=0 ){
					parameters->slack = (uint8_t)temp; //ATUALIZA SLACK e CONFERE MENOR SLACK VALIDO
					// -------------------------------------------------------------------------------------------
					if (ls > parameters->slack) {
						LS_task = task;
						LS_node = atual;
						ls = parameters->slack;
					}
					// -------------------------------------------------------------------------------------------
				}
				
				else { // Caso slack negativo e o remaining positivo siguinifica que ocorreu um deadlinemiss!
					printf("----->!!!!!!!!!!!!!!!DEADLINEMISS!!!!!!!!!!!!!!! ID %d\n", (int)task->id);
					
					if (KILL_IF_DEADLINE_MISS) fail_lsf_panic(ERR_LSF_FAIL, LS_task, tempo_atual);
					} 
			}			
			// -------------------------------------------------------------------------------------------
			if(parameters->remaining == 0){
				printf("----->\"FINISHED\" TASK_ID %d\n", (int)task->id);// Finished inside deadline/period
				tempo_relativo = (tempo_atual-1) % parameters->period;
				if((tempo_relativo)== 0 ){	// RESTART parameters->remaining 
					parameters->remaining = parameters->computation;
					temp = (int)parameters->deadline - (int)parameters->computation; 
					printf("----->\"RESET\" TASK_ID %d\n", (int)task->id);
					parameters->slack =  (uint8_t)temp;
					// -------------------------------------------------------------------------------------------
					if (ls > parameters->slack) { //ATUALIZA SLACK e CONFERE MENOR SLACK VALIDO
						LS_task = task;
						LS_node = atual;
						ls = parameters->slack;
					}
					// -------------------------------------------------------------------------------------------
				}
			}
			// --------------------------------------------------------------------------------
			//DEBUG
			printf("CONFERNDO:||Slack-> %02d|| (",(int)parameters->slack);
			printf("Dl.= %02d -",(int)parameters->deadline);
			printf("Rem.= %02d - ",(int)parameters->remaining );
			printf("T.R.= %02d) |", (int)tempo_relativo);
			printf("(Comp. %02d )|",(int)parameters->computation);
			printf("(T.R.= %02d = %d mod %d)|\n", (int)tempo_relativo, (tempo_atual-1), parameters->period);
			
			
		}
		atual  = atual->next; // Walk
		if(atual != NULL)
			task = atual->data;
	}
	if (LS_task == NULL) {
		/*printf("SEM TAREFAS DE TEMPO REAL\n");
		printf("=====================================================\n");*/
		return -1; 
	}
	// Deve siguinificar que todas ja foram feitas ou q n tem
	/* put the scheduled task in the running state and return its id */
	parameters = (struct lstf_parameter *)LS_task->rt_prio; 
	parameters->remaining--;
	kcb->task_current = LS_node;
	LS_task->state = TASK_RUNNING;
    /*printf("REAL TIME TASK SELECTED TASK ID %d\n", (int)LS_task->id);
    printf("=======================================================\n");*/
    //------------------------------------------------------------------------------------------------------------------
    if ((int)tempo_atual >= (int)STOP_TIME) fail_lsf_panic(ERR_LSF_FAIL, LS_task, tempo_atual); // LIMIT
	//------------------------------------------------------------------------------------------------------------------
	return (int32_t)LS_task->id;
}

int32_t app_main(void)
{
	/* uint8_t */
	struct lstf_parameter parameters[4] = {
		{.computation= 0x03, .period = 0x05, .deadline = 0x05, .slack = 0x02, .remaining = 0x03},
		{.computation= 0x05, .period = 0x0A, .deadline = 0x0A, .slack = 0x05, .remaining = 0x05},
		{.computation= 0x08, .period = 0x0f, .deadline = 0x0f, .slack = 0x07, .remaining = 0x08},
		{.computation= 0x01, .period = 0x0C, .deadline = 0x0C, .slack = 0x0B, .remaining = 0x01},
	};
	
	int ERR = -20;
	/* tasks of the set */ 
	kcb->rt_sched = lstf_sched;// dispatcher

	// IDLE ===========================================================
	/* Init Task*/
	// Idle - ID = 0
	uint16_t idT_Idle = ucx_task_spawn(task_idle, DEFAULT_STACK_SIZE);
	ucx_task_priority(idT_Idle, TASK_IDLE_PRIO);
	//=================================================================

	// NORMAL =========================================================
	/* Init Task */
	//0 - ID = 1
	uint16_t idT0 = ucx_task_spawn(task0, DEFAULT_STACK_SIZE);
	printf("Tarefa: 0 ID: %d\n",  (int)idT0);
	
	//1 - ID = 2
	uint16_t idT1 = ucx_task_spawn(task1, DEFAULT_STACK_SIZE);
	printf("Tarefa: 1 ID: %d\n",  (int)idT1);
	
	//2 - ID = 3
	uint16_t idT2 = ucx_task_spawn(task2, DEFAULT_STACK_SIZE);
	printf("Tarefa: 2 ID: %d\n",  (int)idT2);
	
	//3 - ID = 4
	//uint16_t idT3 = ucx_task_spawn(task3, DEFAULT_STACK_SIZE);
	//printf("Tarefa: 3 ID: %d\n",  (int)idT3);
	// ================================================================

	// RT =============================================================
	/* Setup our custom scheduler and set RT priorities to three*/
	//0 - ID = 1
	ERR = ucx_task_rt_priority(idT0, &parameters[0]);
	printf("ERR T0 %d\n", (int)ERR);
	printf("Endereço do parameters[0] %d\n", (int)&parameters[0]);
	
	//1 - ID = 2
	ERR = ucx_task_rt_priority(idT1, &parameters[1]);
	printf("ERR T1 %d\n", (int)ERR);
	printf("Endereço do parameters[1] %d\n", (int)&parameters[1]);
	
	//2 - ID = 3
	ERR = ucx_task_rt_priority(idT2, &parameters[2]);
	printf("ERR T2 %d\n", (int)ERR);
	printf("Endereço do parameters[2] %d\n", (int)&parameters[2]);
	
	//3 - ID = 4
	//ERR = ucx_task_rt_priority(idT3, &parameters[3]);
	//printf("ERR T3 %d\n", (int)ERR);
	//printf("Endereço do parameters[3] %d\n", (int)&parameters[3]);

	// start UCX/OS, preemptive mode
	return 1;
}