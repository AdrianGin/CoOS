.syntax  unified
.cpu cortex-m4
.thumb


.global _currentTask
.global _nextTask
.global _isRunning


.global pool
.global PendSV_Handler
.global SVC_Handler
.global SVC_Handler_C;

.global CoOS_InitMainStack
.global CoOS_InitProcessStack
.global CoOS_Yield

.global Reset_Handler

.global CoOS_InterruptHandler

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

	//Save current context (R4-R11)
	mrs R0, psp
	stmdb R0!, {R4-R11}
	//Update Stack Pointer of current task
	ldr R2, =[pool]
	ldr R2, [R2, R1 , LSL #2]
	str R0, [R2]

	//Get next task index;
	ldr R0, =[_nextTask]
	ldr R0, [R0]
	ldr R2, =[pool]
	//Get Next Stack Frame's SP
	ldr R1, [R2, R0 , LSL #2]
	ldr R1, [R1]

	//Unstack onto Registers
	ldmia R1!, {R4-R11}

	//Update current task
	str R0, [R3] //_currentTask = _nextTask

	//Update PSP with next task.
	msr psp, R1
	isb



ContextSwitch_Exit:

	//mov lr, #0xFFFFFFFD

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
	tst lr, #4 //Look at EXE_RETURN SP Main or Process
	ite  eq
	mrseq r0, msp	//return to main
	mrsne r0, psp	//return to process

	bl SVC_Handler_C

	//Load the current task's LR into LR;
	mrs r0, psp
	mov r1, #5
	ldr lr, [r0, r1, LSL #2]

	bx lr


.thumb_func
.type CoOS_InitMainStack,%function
CoOS_InitMainStack:
	//Reset MSP to init
	ldr r0, =0xE000ED08 //r0 = VTOR Use the VTOR offset register to locate the stack.
	ldr r0, [r0]		//r0 = *VTOR
	ldr r0, [r0]		//r0 = *(e_stack) (stack init)

	sub r0, #32		    //r0 = r0 - 32
	msr msp, r0			//MSP = r0

	ldr r1, =CoOS_InterruptHandler //r1 = &CoOS::ThreadStart
	add r1, #2
	str r1, [r0, #0x14]	// MSP.LR = &CoOS::ThreadStart
	str r1, [r0, #0x18]   // MSP.Return Addr = &CoOS::ThreadStart

	bx lr


.thumb_func
.type CoOS_InitProcessStack,%function
CoOS_InitProcessStack:

	//Reset MSP to init
	//mov r0, #0
	//ldr r0, [r0]
	//msr msp, r0

	ldr r0, =[_currentTask]
	ldr r0, [r0]
	ldr r2, =[pool]
	//Get Current Task Stack Frame's SP
	ldr r1, [r2, r0 , LSL #2]
	ldr r1, [r1]
	msr psp, r1

	//Set Thread to PSP
	mrs r0, control
	orr r0, r0, #2
	msr control, r0

	movs r1, #1
	ldr r0, =[_isRunning]
	strb r1, [r0]

	bx lr

_SCB_ICSR: .word 0xE000ED00
.type CoOS_Yield,%function
CoOS_Yield:
	ldr r0, =[_SCB_ICSR]
	ldr r1, [r0]
	ldr r2, [r1, #4]
	orr r2, r2, #0x10000000
	str r2, [r1, #4]
	bx lr


.end






