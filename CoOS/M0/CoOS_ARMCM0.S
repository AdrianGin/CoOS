.syntax  unified
.cpu cortex-m0
.thumb


.global _currentTask
.global _nextTask
.global _isRunning


.global pool
.global PendSV_Handler
.global SVC_Handler
.global SVC_Handler_C;

.global CoOS_InitProcessStack
.global CoOS_Yield

/**
 * PendSV_Handler Must:
 * Store R4-R11 of exception frame to current task stack
 * Switch PSP Frame to next task
 * Load R4-R11 of next task to current PSP frame
 */
.section .text, "ax"
.type PendSV_Handler,%function
PendSV_Handler:

	//_currentTask != _nextTask
	ldr R3, =[_currentTask]
	ldr R1, [R3]
	ldr R2, =[_nextTask]
	ldr R2, [R2]
	cmp R2, R1
	beq ContextSwitch_Exit

	//R1 = CurrentTask
	//R2 = Next Task
	//R3 = Pool
	mrs R0, psp
	subs R0, #32
	//Update Stack Pointer of current task
	ldr R3, =[pool]
	lsls r1, 2

	ldr R3, [R3, R1]
	str R0, [R3]

	//Save current context (R4-R11)
	stmia R0!, {R4-R7}
	mov R4, R8
	mov R5, R9
	mov R6, R10
	mov R7, R11
	stmia R0!, {R4-R7}

	//Get next task index;
	//R0 = NextTask
	//R1 = TCB SP
	//R2 = pool
	ldr R0, =[_nextTask]
	ldr R0, [R0]

	//Set Current Task = NextTask
	ldr R3, =[_currentTask]
	str R0, [R3]


	ldr R2, =[pool]

	lsls r0, 2
	//Get Next Stack Frame's SP
	ldr R1, [R2, R0]
	ldr R1, [R1]

	ldmia R1!, {R4-R7}
	mov R8, R4
	mov R9, R5
	mov R10, R6
	mov R11, R7
	ldmia R1!, {R4-R7}

	//Update PSP with next task.
	msr psp, R1



ContextSwitch_Exit:
	bx lr


//Determine SVC Call, pass to C handler
/*
* SVC Start OS
* 	Set PSP to current Task
*	Set Control so Thread mode uses PSP
*	Start a Task Switch to start first Task
*/
.type SVC_Handler,%function
SVC_Handler:
	bx lr



.thumb_func
.type CoOS_InitProcessStack,%function
CoOS_InitProcessStack:
	ldr r0, =[_currentTask]
	ldr r0, [r0]
	ldr r2, =[pool]
	//Get Next Stack Frame's SP
	lsls r0, 2
	ldr r1, [r2, r0]
	ldr r1, [r1]
	msr psp, r1

	//Set Thread to r0PSP
	mrs r0, control
	movs r1, #2
	orrs r0, r0, r1
	msr control, r0

	movs r1, #1
	ldr r0, =[_isRunning]
	strb r1, [r0]

	isb
	bx lr


.end






