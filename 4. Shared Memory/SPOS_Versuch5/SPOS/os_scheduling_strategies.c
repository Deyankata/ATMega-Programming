/*! \file

Scheduling strategies used by the Interrupt Service RoutineA from Timer 2 (in scheduler.c)
to determine which process may continue its execution next.

The file contains five strategies:
-even
-random
-round-robin
-inactive-aging
-run-to-completion
*/

#include "os_scheduling_strategies.h"

#include "defines.h"

#include <stdlib.h>

SchedulingInformation schedulingInfo;
uint8_t initialTimeSlices[4] = {1, 2, 4, 8};

// Initializes the given ProcessQueue with a predefined size
void pqueue_init(ProcessQueue *queue){
	queue->size = MAX_NUMBER_OF_PROCESSES; 
	queue->head = 0;
	queue->tail = 0;
}

// Resets the given ProcessQueue
void pqueue_reset(ProcessQueue *queue){
	queue->head = 0;
	queue->tail = 0;
}

// Checks whether there is next a ProcessID
bool pqueue_hasNext(const ProcessQueue *queue){
	return queue->head != queue->tail;
}

// Returns the first ProcessID of the given ProcessQueue
ProcessID pqueue_getFirst(const ProcessQueue *queue){
	return queue->data[queue->tail];
}

// Drops the first ProcessID of the given ProcessQueue
void pqueue_dropFirst(ProcessQueue *queue){
	if (pqueue_hasNext(queue)){
		queue->data[queue->tail] = 0;
		queue->tail = (queue->tail + 1) % queue->size;
	}
}

// Appends a ProcessID to the given ProcessQueue
void pqueue_append(ProcessQueue *queue, ProcessID pid){
	queue->data[queue->head] = pid;
	queue->head = (queue->head + 1) % queue->size;
}

// Removes a ProcessID from the given ProcessQueue
void pqueue_removePID(ProcessQueue *queue, ProcessID pid){
	uint8_t i = queue->tail;
	while (i != queue->head){
		if (queue->data[i] == pid){
			// Remove the process by shifting elements
			while (i != queue->head){
				queue->data[i] = queue->data[(i+1) % queue->size];
				i = (i + 1) % queue->size;
			}
			
			// Move the head pointer back
			queue->head = (queue->head - 1 + queue->size) % queue->size;
			break;
		}
		i = (i + 1) % queue->size;
	}
}

// Returns the corresponding ProcessQueue
ProcessQueue *MLFQ_getQueue(uint8_t queueID){
	if (queueID < 4){
		return &schedulingInfo.priorityQueues[queueID];
	}
	else{
		// Handle invalid queue ID
		return NULL;
	}
}

// Initializes the scheduling information
void os_initSchedulingInformation(){
	// Initialize queues for MLFQ
	for (uint8_t i = 0; i < 4; i++){
		pqueue_init(MLFQ_getQueue(i));
	}
}

/*!
 *  Reset the scheduling information for a specific strategy
 *  This is only relevant for RoundRobin and InactiveAging
 *  and is done when the strategy is changed through os_setSchedulingStrategy
 *
 *  \param strategy  The strategy to reset information for
 */
void os_resetSchedulingInformation(SchedulingStrategy strategy) {
    switch (strategy){
		case OS_SS_ROUND_ROBIN:
			schedulingInfo.timeSlice = os_getProcessSlot(os_getCurrentProc())->priority;
			break;
		case OS_SS_INACTIVE_AGING:
			for (int i = 0; i < MAX_NUMBER_OF_PROCESSES; i++){
				schedulingInfo.age[i] = 0;
			}
			break;
		case OS_SS_MULTI_LEVEL_FEEDBACK_QUEUE:			
			os_initSchedulingInformation();
			// Assign processes to queues
			for (ProcessID pid = 1; pid < MAX_NUMBER_OF_PROCESSES; pid++){
				if (os_getProcessSlot(pid)->state != OS_PS_UNUSED){
					os_resetProcessSchedulingInformation(pid);
				}
			}
		default:
			break;
	}
}

/*!
 *  Reset the scheduling information for a specific process slot
 *  This is necessary when a new process is started to clear out any
 *  leftover data from a process that previously occupied that slot
 *
 *  \param id  The process slot to erase state for
 */
void os_resetProcessSchedulingInformation(ProcessID id) {
	schedulingInfo.age[id] = 0;
	
	MLFQ_removePID(id);
	// Reset MLFQ-specific information if needed
	uint8_t firstTwoMSBs = (os_getProcessSlot(id)->priority) >> 6;
	switch (firstTwoMSBs){
		case 0b00:
			schedulingInfo.timeSlicesMLFQ[id] = 8;
			pqueue_append(MLFQ_getQueue(3), id);
			break;
		case 0b01:
			schedulingInfo.timeSlicesMLFQ[id] = 4;
			pqueue_append(MLFQ_getQueue(2), id);
			break;
		case 0b10:
			schedulingInfo.timeSlicesMLFQ[id] = 2;
			pqueue_append(MLFQ_getQueue(1), id);
			break;
		case 0b11:
			schedulingInfo.timeSlicesMLFQ[id] = 1;
			pqueue_append(MLFQ_getQueue(0), id);
			break;
		default:
			break;
	}
}

/*!
 *  This function implements the even strategy. Every process gets the same
 *  amount of processing time and is rescheduled after each scheduler call
 *  if there are other processes running other than the idle process.
 *  The idle process is executed if no other process is ready for execution
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the even strategy.
 */
ProcessID os_Scheduler_Even(const Process processes[], ProcessID current) {
	for (ProcessID i = current + 1; i < MAX_NUMBER_OF_PROCESSES; i++){
		if (processes[i].state == OS_PS_READY){
			return i;
		}
	}

	for (ProcessID j = 1; j <= current; j++){
		if (processes[j].state == OS_PS_READY){
			return j;
		}
	}
	
	// Check for a blocked process and select it again
	if(processes[current].state == OS_PS_BLOCKED){
		return current;
	}
	
	return 0;
}

/*!
 *  This function implements the random strategy. The next process is chosen based on
 *  the result of a pseudo random number generator.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the random strategy.
 */
ProcessID os_Scheduler_Random(const Process processes[], ProcessID current) {
	uint8_t numReadyProcesses = 0;
	
	 // Count the number of processes in OS_PS_READY state
	for (ProcessID i = 1; i < MAX_NUMBER_OF_PROCESSES; i++){
		if (processes[i].state == OS_PS_READY){
			numReadyProcesses++;
		}
	}
	
	uint8_t randomValue = rand() % numReadyProcesses;
	
	// Find the process with the corresponding index
	for (ProcessID i = 1; i < MAX_NUMBER_OF_PROCESSES; i++){
		if(processes[i].state == OS_PS_READY) {
			if(randomValue == 0) {
				return i;
			} 
			else {
				randomValue--;
			}
		}
	}
	
	// Check for a blocked process and select it again
	if(processes[current].state == OS_PS_BLOCKED){
		return current;
	}

	return 0;
}

/*!
 *  This function implements the round-robin strategy. In this strategy, process priorities
 *  are considered when choosing the next process. A process stays active as long its time slice
 *  does not reach zero. This time slice is initialized with the priority of each specific process
 *  and decremented each time this function is called. If the time slice reaches zero, the even
 *  strategy is used to determine the next process to run.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed determined on the basis of the round robin strategy.
 */
ProcessID os_Scheduler_RoundRobin(const Process processes[], ProcessID current) {	
	schedulingInfo.timeSlice--;
	if (processes[current].state == OS_PS_READY && schedulingInfo.timeSlice > 0) {
		return current;
	}

	ProcessID nextProcessId = os_Scheduler_Even(processes, current);
	schedulingInfo.timeSlice = processes[nextProcessId].priority;

	return nextProcessId;
}

/*!
 *  This function realizes the inactive-aging strategy. In this strategy a process specific integer ("the age") is used to determine
 *  which process will be chosen. At first, the age of every waiting process is increased by its priority. After that the oldest
 *  process is chosen. If the oldest process is not distinct, the one with the highest priority is chosen. If this is not distinct
 *  as well, the one with the lower ProcessID is chosen. Before actually returning the ProcessID, the age of the process who
 *  is to be returned is reset to its priority.
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed, determined based on the inactive-aging strategy.
 */
ProcessID os_Scheduler_InactiveAging(const Process processes[], ProcessID current) {    
	ProcessID oldestProcessID = 0;
	Age oldestAge = 0;
	Priority oldestPriority = 0;
	
	for (ProcessID i = 1; i < MAX_NUMBER_OF_PROCESSES; i++){
		// Increase the age of each waiting process by its priority
		schedulingInfo.age[i] += processes[i].priority;
		
		// Check if the process is older than the current oldest process
		if (processes[i].state == OS_PS_READY && schedulingInfo.age[i] >= oldestAge){
			// Update if the age is greater or priority is higher
			if (schedulingInfo.age[i] > oldestAge || processes[i].priority > oldestPriority || (processes[i].priority == oldestPriority && i < oldestProcessID)){
				oldestAge = schedulingInfo.age[i];
				oldestPriority = processes[i].priority;
				oldestProcessID = i;
			}
		}
	}
	
	 // Check for a blocked process and select it again
	 if (processes[current].state == OS_PS_BLOCKED && oldestProcessID == 0){
		 schedulingInfo.age[current] = 0;
		 return current;
	 }
	 
	// Reset the age of the chosen process to 0
	schedulingInfo.age[oldestProcessID] = 0;
	
	return oldestProcessID;
}

/*!
 *  This function realizes the run-to-completion strategy.
 *  As long as the process that has run before is still ready, it is returned again.
 *  If  it is not ready, the even strategy is used to determine the process to be returned
 *
 *  \param processes An array holding the processes to choose the next process from.
 *  \param current The id of the current process.
 *  \return The next process to be executed, determined based on the run-to-completion strategy.
 */
ProcessID os_Scheduler_RunToCompletion(const Process processes[], ProcessID current) {	
	if (processes[current].state == OS_PS_READY){
		return current;
	}
	
	return os_Scheduler_Even(processes, current);
}

// MultiLevelFeedbackQueue strategy
ProcessID os_Scheduler_MLFQ(const Process processes[], ProcessID current){
	
	// The following code snippet acts as a garbage collector for UNUSED processes and cleans up the queues from any unused processes
	for (uint8_t queueID = 0; queueID < 4; queueID++){
		ProcessQueue *currentQueue = MLFQ_getQueue(queueID);
		for (uint8_t index = currentQueue->tail; index != currentQueue->head; index = (index + 1) % currentQueue->size){
			if (processes[currentQueue->data[index]].state == OS_PS_UNUSED){
				pqueue_removePID(currentQueue, currentQueue->data[index]);
			}
		}
	}

	// Consider the case of a process yielding its computational time early
	if (processes[current].state == OS_PS_BLOCKED){
		for (uint8_t queueID = 0; queueID < 4; queueID++){
			ProcessQueue *currentQueue = MLFQ_getQueue(queueID);
			if (pqueue_getFirst(currentQueue) == current){
				// If the yielding process has no computational time left, append it to the lower priority queue
				if (schedulingInfo.timeSlicesMLFQ[current] == 0){
					pqueue_removePID(currentQueue, current);
					if (queueID < 3){
						pqueue_append(currentQueue + 1, current);
						schedulingInfo.timeSlicesMLFQ[current] = initialTimeSlices[queueID + 1];
					}
					else{
						pqueue_append(currentQueue, current);
						schedulingInfo.timeSlicesMLFQ[current] = initialTimeSlices[queueID];
					}
					break;
				}
				else {
					pqueue_removePID(currentQueue, current);
					pqueue_append(currentQueue, current);
					break;
				}
			}
		}
	}

	// Iterate through each priority queue and find the next process
	for (uint8_t queueID = 0; queueID < 4; queueID++){
		ProcessQueue *currentQueue = MLFQ_getQueue(queueID);
		if (pqueue_hasNext(currentQueue)){
			ProcessID nextPID = pqueue_getFirst(currentQueue);
			if (processes[nextPID].state == OS_PS_READY){
				// Process has only 1 time slice left, append it to the next lower priority queue and return
				if (schedulingInfo.timeSlicesMLFQ[nextPID] == 1){
					pqueue_removePID(currentQueue, nextPID);
					if (queueID < 3){
						pqueue_append(currentQueue + 1, nextPID);
						schedulingInfo.timeSlicesMLFQ[nextPID] = initialTimeSlices[queueID + 1];
					}
					else{
						pqueue_append(currentQueue, nextPID);
						schedulingInfo.timeSlicesMLFQ[nextPID] = initialTimeSlices[queueID];
					}
					return nextPID;
				}
				schedulingInfo.timeSlicesMLFQ[nextPID]--;
				return nextPID;
			}
		}		 
	}
	
	if (processes[current].state == OS_PS_BLOCKED){
		return current;
	}
	
	return 0;
}

// Function that removes the given ProcessID from the ProcessQueues	
void MLFQ_removePID(ProcessID pid){
	for (uint8_t queueID = 0; queueID < 4; queueID++){
		pqueue_removePID(MLFQ_getQueue(queueID), pid);
	}
}