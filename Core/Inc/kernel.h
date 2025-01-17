/*
 * kernel.h
 *
 *  Created on: Jun 24, 2024
 *      Author: mahir
 */

#ifndef INC_KERNEL_H_
#define INC_KERNEL_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define STACK_SIZE 0x400
#define TOTAL_STACK_SIZE 0x4000
#define TOTAL_STACKS TOTAL_STACK_SIZE/STACK_SIZE
#define DEFAULT_TIMEOUT_MS 5

#define SHPR2 *(uint32_t*)0xE000ED1C //for setting SVC priority, bits 31-24
#define SHPR3 *(uint32_t*)0xE000ED20 // PendSV is bits 23-16
#define _ICSR *(uint32_t*)0xE000ED04 //This lets us trigger PendSV

#define configMAX_PRIORITY 3

typedef struct k_thread {
	uint32_t* sp; //stack pointer
	void (*thread_function)(void*); //function pointer
	uint32_t timeslice;
	uint32_t runtime;
	uint8_t priority;
} thread;

void osKernelInitialize (void);
uint32_t* osAllocateStack (void);
bool osCreateThread (void (*thread_function)(void*), void* args);
bool osCreateThreadWithDeadline (void (*thread_function)(void* args), void* args, uint32_t deadline);
void osKernelStart (void);
void osSched(void);
void osYield(void);
void SVC_Handler_Main (unsigned int *svc_args);

#endif /* INC_KERNEL_H_ */
