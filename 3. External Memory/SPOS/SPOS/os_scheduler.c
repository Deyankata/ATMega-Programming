/*! \file
 *  \brief Scheduling module for the OS.
 *
 * Contains everything needed to realise the scheduling between multiple processes.
 * Also contains functions to start the execution of programs.
 *
 *  \author   Lehrstuhl Informatik 11 - RWTH Aachen
 *  \date     2013
 *  \version  2.0
 */

#include "os_scheduler.h"

#include "lcd.h"
#include "os_core.h"
#include "os_input.h"
#include "os_memheap_drivers.h"
#include "os_memory.h"
#include "os_scheduling_strategies.h"
#include "os_taskman.h"
#include "util.h"

#include <avr/interrupt.h>
#include <stdbool.h>

//----------------------------------------------------------------------------
// Private Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

//! Array of states for every possible process
Process os_processes[MAX_NUMBER_OF_PROCESSES];

//! Index of process that is currently executed (default: idle)
ProcessID currentProc;

//----------------------------------------------------------------------------
// Private variables
//----------------------------------------------------------------------------

//! Currently active scheduling strategy
SchedulingStrategy currentStrat;

//! Count of currently nested critical sections
uint8_t criticalSectionCount;

//----------------------------------------------------------------------------
// Private function declarations
//----------------------------------------------------------------------------

//! ISR for timer compare match (scheduler)
ISR(TIMER2_COMPA_vect)
__attribute__((naked));

//----------------------------------------------------------------------------
// Function definitions
//----------------------------------------------------------------------------

// Wrapper to encapsulate processes.
void os_dispatcher(void){
	// Determine the active process ID and associated function pointer
	ProcessID activePID = os_getCurrentProc();
	Program *activeProgram = os_processes[activePID].program;
	
	// Call the program directly using the function pointer
	activeProgram();
	os_kill(activePID);
}

/*!
 *  Timer interrupt that implements our scheduler. Execution of the running
 *  process is suspended and the context saved to the stack. Then the periphery
 *  is scanned for any input events. If everything is in order, the next process
 *  for execution is derived with an exchangeable strategy. Finally the
 *  scheduler restores the next process for execution and releases control over
 *  the processor to that process.
 */
ISR(TIMER2_COMPA_vect) {
	saveContext(); // Step 2

	os_processes[currentProc].sp.as_int = SP; // Step 3
	SP = BOTTOM_OF_ISR_STACK; // Step 4
		
	if ((os_getInput() & (1 | (1 << 3))) == (1 | (1 << 3))){ // if Enter + ESC (Task manager)
		os_waitForNoInput();
		os_taskManMain();
		while(os_taskManOpen()){}
	}
		
	os_processes[currentProc].checksum = os_getStackChecksum(currentProc); 
	
	if (os_processes[currentProc].state != OS_PS_UNUSED){
		os_processes[currentProc].state = OS_PS_READY; // Step 5
	}
	
	switch (currentStrat){
		case OS_SS_EVEN:
			currentProc = os_Scheduler_Even(os_processes, currentProc);
			break;
		case OS_SS_RANDOM:
			currentProc = os_Scheduler_Random(os_processes, currentProc);
			break;
		case OS_SS_INACTIVE_AGING:
			currentProc = os_Scheduler_InactiveAging(os_processes, currentProc);
			break;
		case OS_SS_ROUND_ROBIN:
			currentProc = os_Scheduler_RoundRobin(os_processes, currentProc);
			break;
		case OS_SS_RUN_TO_COMPLETION:
			currentProc = os_Scheduler_RunToCompletion(os_processes, currentProc);
			break;
		default:
			break;
	}
	if (os_getStackChecksum(currentProc) != os_processes[currentProc].checksum){
			// The stack has changed, issue an error message
			os_error("Stack checksum has changed!");
 	}
	os_processes[currentProc].state = OS_PS_RUNNING; // Step 7
	SP = os_processes[currentProc].sp.as_int; // Step 8

	restoreContext(); // Step 9
}

/*!
 *  This is the idle program. The idle process owns all the memory
 *  and processor time no other process wants to have.
 */
void idle(void){
	while(1){
		lcd_writeProgString(PSTR("...."));
		delayMs(DEFAULT_OUTPUT_DELAY);
	}
}

/*!
 *  This function is used to register the given program for execution.
 *  A stack will be provided if the process limit has not yet been reached.
 *  This function is multitasking safe. That means that programs can repost
 *  themselves, simulating TinyOS 2 scheduling (just kick off interrupts ;) ).
 *
 *  \param program  The function of the program to start.
 *  \param priority A priority ranging 0..255 for the new process:
 *                   - 0 means least favourable
 *                   - 255 means most favourable
 *                  Note that the priority may be ignored by certain scheduling
 *                  strategies.
 *  \return The index of the new process or INVALID_PROCESS as specified in
 *          defines.h on failure
 */
ProcessID os_exec(Program *program, Priority priority) {
	os_enterCriticalSection();
	
	// Step 1: Find free space in the os_processes array
	ProcessID pid = INVALID_PROCESS;
	for (ProcessID i = 0; i < MAX_NUMBER_OF_PROCESSES; i++){
		if (os_processes[i].state == OS_PS_UNUSED){
			os_resetProcessSchedulingInformation(i);
			pid = i;
			break;
		}
	}
	
	if (pid == INVALID_PROCESS){	
		os_leaveCriticalSection();
		// All slots are occupied, return INVALID_PROCESS
		return INVALID_PROCESS;
	}
	
	// Step 2: Check program pointer for validity
	if (program == NULL){
		os_leaveCriticalSection();
		return INVALID_PROCESS;
	}
	
	// Step 3: Save program, process state, and process priority
	os_processes[pid].state = OS_PS_READY;
	os_processes[pid].program = program;
	os_processes[pid].priority = priority;
	
	// Step 4: Prepare the process stack
	StackPointer stack_ptr;
	stack_ptr.as_int = PROCESS_STACK_BOTTOM(pid);
	
	// Load the program counter (PC) from the program pointer
	*stack_ptr.as_ptr = (uint8_t)(((uint16_t)os_dispatcher) & 0xFF); // Low byte
	stack_ptr.as_ptr--;
	*stack_ptr.as_ptr = (uint8_t)(((uint16_t)os_dispatcher >> 8) & 0xFF); // High byte
	stack_ptr.as_ptr--;
	
	// Initialize the runtime context with 33 null bytes
	for (int i = 0; i < 33; i++){
		stack_ptr.as_ptr--;
		*stack_ptr.as_ptr = 0;
	}
	
	// Set the stack pointer of the new process
	os_processes[pid].sp = stack_ptr;
	
	// Step 5: Calculate and set the checksum for the process stack
	StackChecksum checksum = os_getStackChecksum(pid);
	os_processes[pid].checksum = checksum;
	
	os_leaveCriticalSection();
	
	return pid;
}

/*!
 *  If all processes have been registered for execution, the OS calls this
 *  function to start the idle program and the concurrent execution of the
 *  applications.
 */
void os_startScheduler(void) {
	currentProc = 0; // idle process
	os_processes[currentProc].state = OS_PS_RUNNING;
	SP = os_processes[currentProc].sp.as_int;
	restoreContext();
}

/*!
 *  In order for the Scheduler to work properly, it must have the chance to
 *  initialize its internal data-structures and register.
 */
void os_initScheduler(void){
	// Set all process states to OS_PS_UNUSED
	for (ProcessID pid = 0; pid < MAX_NUMBER_OF_PROCESSES; pid++){
		os_processes[pid].state = OS_PS_UNUSED;
	}
	
	// Start idle process with PID 0
	ProcessID idlePID = os_exec(idle, DEFAULT_PRIORITY);
	
	// Iterate through autostart_head linked list
	while (autostart_head != NULL){
		if (os_isRunnable(os_getProcessSlot(idlePID))){ // if the idle process is runnable
			os_exec(autostart_head->program, DEFAULT_PRIORITY);
		}
		autostart_head = autostart_head->next;
	}
}

/*!
 *  A simple getter for the slot of a specific process.
 *
 *  \param pid The processID of the process to be handled
 *  \return A pointer to the memory of the process at position pid in the os_processes array.
 */
Process *os_getProcessSlot(ProcessID pid) {
    return os_processes + pid;
}

/*!
 *  A simple getter to retrieve the currently active process.
 *
 *  \return The process id of the currently active process.
 */
ProcessID os_getCurrentProc(void) {
    return currentProc;
}

/*!
 *  Sets the current scheduling strategy.
 *
 *  \param strategy The strategy that will be used after the function finishes.
 */
void os_setSchedulingStrategy(SchedulingStrategy strategy){
	currentStrat = strategy;
	os_resetSchedulingInformation(currentStrat);
}

/*!
 *  This is a getter for retrieving the current scheduling strategy.
 *
 *  \return The current scheduling strategy.
 */
SchedulingStrategy os_getSchedulingStrategy(void) {
    return currentStrat;
}

/*!
 *  Enters a critical code section by disabling the scheduler if needed.
 *  This function stores the nesting depth of critical sections of the current
 *  process (e.g. if a function with a critical section is called from another
 *  critical section) to ensure correct behaviour when leaving the section.
 *  This function supports up to 255 nested critical sections.
 */
void os_enterCriticalSection(void) {
	uint8_t globalInterruptBit = SREG >> 7;
	cli(); // Disable global interrupts
	if (criticalSectionCount == 255){
		os_error("Overflow");
	}
	else{
		criticalSectionCount++;
	}
	TIMSK2 &= ~(1 << OCIE2A);
	SREG |= globalInterruptBit << 7; // Restore interrupt state
}

/*!
 *  Leaves a critical code section by enabling the scheduler if needed.
 *  This function utilizes the nesting depth of critical sections
 *  stored by os_enterCriticalSection to check if the scheduler
 *  has to be reactivated.
 */
void os_leaveCriticalSection(void){
	uint8_t globalInterruptBit = SREG >> 7;
	cli();
	if (criticalSectionCount == 0){
		os_error("Underflow");
	}
	else{
		criticalSectionCount--;
	}
	if (criticalSectionCount == 0){
		// Enable the scheduler if leaving the outermost critical section
		TIMSK2 |= (1 << OCIE2A);
	}
		
	SREG |= globalInterruptBit << 7; // Restore interrupt state
}

/*!
 *  Calculates the checksum of the stack for a certain process.
 *
 *  \param pid The ID of the process for which the stack's checksum has to be calculated.
 *  \return The checksum of the pid'th stack.
 */
StackChecksum os_getStackChecksum(ProcessID pid) {
	StackChecksum checksum = 0;
	
	uint16_t pointer = os_processes[pid].sp.as_int; 
	uint16_t bottom = PROCESS_STACK_BOTTOM(pid);
	
	while(bottom > pointer){	
		pointer++;
		checksum ^= *(uint8_t*)pointer;
	}

	return checksum;
}

// Used to kill a running process and clear the corresponding process slot. 
bool os_kill(ProcessID  pid){
	os_enterCriticalSection();
	
	if (pid == INVALID_PROCESS || pid <= 0 || pid >= MAX_NUMBER_OF_PROCESSES || pid == OS_PS_UNUSED){
		os_leaveCriticalSection();
		return false; // Invalid process ID
	}
	os_processes[pid].state = OS_PS_UNUSED;
	
	for (uint8_t i = 0; i < os_getHeapListLength(); i++){
		os_freeProcessMemory(os_lookupHeap(i), pid);
	}
	
	if(os_getCurrentProc() == pid){		// If current process is killing itself
		criticalSectionCount = 1;
		os_leaveCriticalSection();
		while(1){} // idle
		return true;
	}

	os_leaveCriticalSection();
	return true;
}
