/*
 * kernel.c
 *
 *  Created on: Jun 24, 2024
 *      Author: mahir
 */

#include "kernel.h"

extern void runFirstThread(void);

uint32_t* prev_stack;
uint32_t* msp_init_val;

thread threads[TOTAL_STACKS];
uint8_t curr_thread;
uint8_t total_threads;

void osKernelInitialize (void) {
	msp_init_val = *(uint32_t**)0x0;
	prev_stack = msp_init_val;
	for (uint8_t i = 0; i < TOTAL_STACKS; i++) {
		threads[i].sp = NULL;
		threads[i].thread_function = NULL;
		threads[i].timeslice = 0x0;
		threads[i].runtime = 0x0;
		threads[i].priority = 0x0;
	}
	curr_thread = 0;
	total_threads = 0;

	//set the priority of PendSV to almost the weakest
	SHPR3 |= 0xFE << 16; //shift the constant 0xFE 16 bits to set PendSV priority
	SHPR2 |= 0xFDU << 24; //Set the priority of SVC higher than PendSV
}

uint32_t* osAllocateStack(void) {
	uint32_t* new_stack = prev_stack - STACK_SIZE;
	if((uint32_t)msp_init_val - (uint32_t)new_stack > TOTAL_STACK_SIZE) {
		return NULL;
	}
	prev_stack = new_stack;
	return new_stack;
}

bool osCreateThread (void (*thread_function)(void* args), void* args) {
	return osCreateThreadWithDeadline ((void*)thread_function, args, DEFAULT_TIMEOUT_MS);
}

bool osCreateThreadWithDeadline (void (*thread_function)(void* args), void* args, uint32_t deadline) {
	uint32_t* stackptr = osAllocateStack();
	if(stackptr == NULL) {
		return false;
	}

	__disable_irq(); //Enter critical section

	*(--stackptr) = 1 << 24; //xPSR
	*(--stackptr) = (uint32_t)thread_function; //PC
	for (uint8_t  i = 0; i < 14; i++) {
		if(i == 5){ //R0
			*(--stackptr) = args;
		} else {
			*(--stackptr) = 0xA;
		}
	}

	threads[total_threads].sp = stackptr;
	threads[total_threads].thread_function = thread_function;

	threads[total_threads].timeslice = deadline;
	threads[total_threads].runtime = deadline;
	total_threads++;

	__enable_irq(); //Exit critical section

	return true;
}

void osSched(void){
	threads[curr_thread].runtime = threads[curr_thread].timeslice;
	threads[curr_thread].sp = (uint32_t*)(__get_PSP() - 8*4);
	curr_thread = (curr_thread + 1) % total_threads;
	__set_PSP((uint32_t)threads[curr_thread].sp);
}

void osKernelStart (void) {
	__asm("SVC #1");
}

void osYield(void) {
	__asm("SVC #2");
}

void SVC_Handler_Main(unsigned int *svc_args) {
	unsigned int svc_number;
	/*
	* Stack contains:
	* r0, r1, r2, r3, r12, r14, the return address and xPSR
	* First argument (r0) is svc_args[0]
	*/
	svc_number = ((char *)svc_args[6])[-2];
	switch(svc_number) {
		case 1:
			__set_PSP((uint32_t)threads[curr_thread].sp);
			runFirstThread();
			break;
		case 2:
			_ICSR |= 1 << 28;
			__asm("isb");
			break;
		default: /* unknown SVC */
			break;
	}
}
