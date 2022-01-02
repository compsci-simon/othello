/**
 * @file manager.c
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "manager.h"
#include "loader.h"

#define QUANTUM 1
#define TRUE 1
#define FALSE 0
#define DEBUG 1
#define RES_STRING_SIZE 100

/* Complete function definitions */
 
void process_release(struct pcb_t *pcb, struct instruction_t *instruct, struct resource_t **resource);

void process_request(struct pcb_t *pcb, struct instruction_t *instruct, struct resource_t **resource);

void process_send_message(struct pcb_t *pcb, 
    struct instruction_t *instruct, struct mailbox_t *mail);
void process_receive_message(struct pcb_t *pcb, 
    struct instruction_t *instruct, struct mailbox_t *mail);
 
int acquire_resource(char *resourceName, struct resource_t *resources, struct pcb_t *p);

int release_resource(char *resourceName, struct resource_t *resources, struct pcb_t *p);

void add_resource_to_process(struct pcb_t *current, char *resource); 
int processes_deadlocked(struct resource_t *resources, struct pcb_t 
	*ready_queue, struct pcb_t *waiting_queue, struct pcb_t *current);

int processes_finished(struct pcb_t *pcb);
void process_to_terminateq(struct pcb_t **current, struct pcb_t **terminated_queue);
void print_pcb_queue(char *list_name, struct pcb_t *pcb);
void add_resource_to_queue(struct resource_t *to_add, struct resource_t **resources2);
void move_resource_fromQ_toQ(char *resource_name, struct resource_t **resources, struct resource_t **resources2);
void add_pcb_to_queue(struct pcb_t *to_add, struct pcb_t **pcb);
void move_pcb_fromQ_toQ(char *pcb_name, struct pcb_t **pcb_from, struct pcb_t **pcb_to);
struct pcb_t *pcb_holding_resource (char *resource_name, struct pcb_t *waiting_queue);
char *pcb_waiting_for_res(int proc_number, struct pcb_t *waiting_queue);
int pcb_number_holding_res(char *res_name, struct pcb_t *waiting_queue);
int pcb_name_val(int pos, struct pcb_t *waiting_queue);
void free_deadlock(struct resource_t **resources, struct pcb_t 
	**ready_queue, struct pcb_t **waiting_queue, struct pcb_t **current);
void free_pcbs_requesting_res(char *res_freed, struct pcb_t **waiting_queue, struct pcb_t **ready_queue);
int current_pcb_index(struct pcb_t *pcb, struct pcb_t *waiting_queue);

/**
 * @brief Schedules each instruction of each process in a round-robin fashion.
 * The number of instruction to execute for each process is governed by the
 * QUANTUM variable.
 *
 * @param pcb The process control block which contains the current process as
 * well as a pointer to the next process control block.
 * @param resources The list of resources available to the system.
 * @param mail The list of mailboxes available to the system.
 * @param type The type of scheduling to perform (priority(1) or round robin(0))
 * @param quantum The number of instructions to complete for RR scheduling
 */
void schedule_processes(struct pcb_t **pcb, 
		struct resource_t **resources, struct mailbox_t *mail, int type, int
quantum){
		
	struct pcb_t *waiting_queue = NULL;
	struct pcb_t **ready_queue = pcb;
  	struct pcb_t *terminated_queue = NULL;
	struct pcb_t *current = NULL;

	move_pcb_fromQ_toQ((*ready_queue)->page->name, ready_queue, &current);
	current->state = RUNNING;

	
	
	while (*ready_queue != NULL || waiting_queue != NULL || current != NULL) {
		
		if (type == 0) {
			// Round robin scheduling
			// Current should never be null at this point
			if (current->next_instruction != NULL) {
				for (int current_quantum = 0; current_quantum < quantum && current->next_instruction != NULL; current_quantum++) {
					char *resource_name = NULL;
					switch (current->next_instruction->type) {
					case REQ_V:
						process_request(current, current->next_instruction, resources);
						if (current->state == WAITING) {
							#ifdef DEBUG
							printf("%s req %s: waiting;\n", current->page->name, current->next_instruction->resource);
							#endif
							move_pcb_fromQ_toQ(current->page->name, &current, &waiting_queue);
							if (*ready_queue != NULL) {
								move_pcb_fromQ_toQ((*ready_queue)->page->name, ready_queue, &current);
								current->state = RUNNING;
								current_quantum = -1;
							}
						}
						break;
					case REL_V:
						resource_name = current->next_instruction->resource;
						process_release(current, current->next_instruction, resources);
						if (waiting_queue != NULL && resource_name != NULL) {
							free_pcbs_requesting_res(resource_name, &waiting_queue, ready_queue);
						}
						break;
					case SEND_V:
						process_send_message(current, current->next_instruction, mail);
						break;
					case RECV_V:
						process_receive_message(current, current->next_instruction, mail);
						break;
					}
					if (current == NULL) {
						break;
					} else if (current->next_instruction == NULL) {
						break;
					}
				}
			}
		} else {
			// Priority scheduling
			// At this point of the loop current should not be null
			if (current->next_instruction != NULL) {
				while (TRUE) {
					char *resource_name = NULL;
					// This switch statement handles the isntructions of the currently running process
					switch (current->next_instruction->type) {
					case REQ_V:
						process_request(current, current->next_instruction, resources);
						if (current->state == WAITING) {
							#ifdef DEBUG
							printf("%s req %s: waiting;\n", current->page->name, current->next_instruction->resource);
							#endif
							move_pcb_fromQ_toQ(current->page->name, &current, &waiting_queue);
							if (*ready_queue != NULL) {
								move_pcb_fromQ_toQ((*ready_queue)->page->name, ready_queue, &current);
								current->state = RUNNING;
							}
						}
						break;
					case REL_V:
						resource_name = current->next_instruction->resource;
						process_release(current, current->next_instruction, resources);
						if (waiting_queue != NULL && resource_name != NULL) {
							free_pcbs_requesting_res(resource_name, &waiting_queue, ready_queue);
						}
						break;
					 case SEND_V:
						process_send_message(current, current->next_instruction, mail);
						break;
					 case RECV_V:
						process_receive_message(current, current->next_instruction, mail);
						break;
					}
					if (current == NULL) {
						break;
					} else if (current->next_instruction == NULL) {
						break;
					}
				// End of inner while loop
				}
			}
		}
		// end of conditional between type 0 and 1
		if (processes_finished(current) && current != NULL) {
			process_to_terminateq(&current, &terminated_queue);
			if (*ready_queue != NULL) {
				move_pcb_fromQ_toQ((*ready_queue)->page->name, ready_queue, &current);
				current->state = RUNNING;
			}
		} else if (*ready_queue != NULL) {
			current->state = WAITING;
			move_pcb_fromQ_toQ(current->page->name, &current, ready_queue);
			move_pcb_fromQ_toQ((*ready_queue)->page->name, ready_queue, &current);
			current->state = RUNNING;
		}
		// This if conditional handles deadlocks and blocked processes
		if (*ready_queue == NULL && waiting_queue != NULL) {
			int result = processes_deadlocked(*resources, *ready_queue, waiting_queue, current);
			if (result == 1) {
				free_deadlock(resources, ready_queue, &waiting_queue, &current);
			} else if (result == 2) {
				break;
			}
		}
	// end of while loop		
	}
}

/**
 * @brief Handles the request resource instruction.
 *
 * Executes the request instruction for the process. The function loops
 * through the list of resources and acquires the resource if it is available.
 * If the resource is not available the process sits in the waiting queue and
 * tries to acquire the resource on the next cycle.
 *
 * @param current The current process for which the resource must be acquired.
 * @param instruct The instruction which requests the resource.
 * @param resources The list of resources.
 */
void process_request(struct pcb_t *current, struct instruction_t *instruct, struct resource_t **resources) {

  current->state = RUNNING;

	if (acquire_resource(instruct->resource, *resources, current)) {
		move_resource_fromQ_toQ(instruct->resource, resources, &(current->resources));
		current->next_instruction = current->next_instruction->next;

    #ifdef DEBUG
    char *resources_string = malloc(100*sizeof(char));
    memset(resources_string, 0, 100);
    struct resource_t *temp_res = *resources;
    for (; temp_res != NULL; temp_res = temp_res->next) {

      strcat(resources_string, " ");
      strcat(resources_string, temp_res->name);

    }
    printf("%s req %s: acquired; Available:%s\n", current->page->name, instruct->resource, resources_string);
    free(resources_string);
    #endif

	} else {
		current->state = WAITING;
	}

}

/**
 * @brief Handles the release resource instruction.
 *
 * Executes the release instruction for the process. If the resource can be
 * released the process is ready for next execution. If the resource can not
 * be released the process sits in the waiting queue.
 *
 * @param current The process which releases the resource.
 * @param instruct The instruction to release the resource.
 * @param resources The list of resources.
 */
/*
TODO: 
Implement function
*/
void process_release(struct pcb_t *current, struct instruction_t *instruct, struct resource_t **resources) {

  current->state = RUNNING;

  if (release_resource(instruct->resource, *resources, current)) {

    move_resource_fromQ_toQ(instruct->resource, &(current->resources), resources);
		current->next_instruction = current->next_instruction->next;

    #ifdef DEBUG
    char *resources_string = malloc(100*sizeof(char));
    memset(resources_string, 0, 100);
    struct resource_t *temp_res = *resources;
    for (; temp_res != NULL; temp_res = temp_res->next) {

      strcat(resources_string, " ");
      strcat(resources_string, temp_res->name);

    }
    printf("%s re1 %s: released; Available:%s\n", current->page->name, instruct->resource, resources_string);
    free(resources_string);
    #endif

	} else {
		current->next_instruction = current->next_instruction->next;
		#ifdef DEBUG
		printf("%s rel %s: Error: Nothing to release\n", current->page->name, instruct->resource);
		#endif
  }

}

/**
 * @brief Sends the message the prescribed mailbox.
 *
 * Sends the message specified in the instruction of the current process, to
 * the mailbox specified in the instruction. The function gets the pointer to 
 * the first mailbox and loops through all the mailboxes to find the one to
 * which the message must be sent.
 *
 * @param pcb The current process which instruct us to send a message.
 * @param instruct The current send instruction which contains the message.
 * @param mail The list of available mailboxes.
 */
void process_send_message(struct pcb_t *pcb, 
    struct instruction_t *instruct, struct mailbox_t *mail) {
  
  struct mailbox_t *current_mailbox;
  
  pcb->state = RUNNING;

  current_mailbox = mail;
  do {
    if ( strcmp(current_mailbox->name, instruct->resource) == 0 ) {
      /* We found the mailbox in which a message should be left */
      break;
    }
    current_mailbox = current_mailbox->next;
  } while ( current_mailbox != NULL );
 
  printf("%s sending message to mailbox %s which says \033[22;31m %s \033[0m\n",
      pcb->page->name, current_mailbox->name, instruct->msg);

  current_mailbox->msg = instruct->msg;
  pcb->next_instruction = pcb->next_instruction->next;
  dealloc_instruction(instruct);
}

/**
 * @brief Retrieves the message from the mailbox specified in the instruction
 * and stores it in the instruction message field.
 *
 * The function loops through the available mailboxes and finds the mailbox
 * from which the message must be retrieved. The retrieved message is stored
 * in the message field of the instruction of the process.
 *
 * @param pcb The current process which requests a message retrieval.
 * @param instruct The instruction to retrieve a message from a specific
 * mailbox.
 * @param mail The list of mailboxes.
 */
void process_receive_message(struct pcb_t *pcb, 
    struct instruction_t *instruct, struct mailbox_t *mail) {
  
  struct mailbox_t *current_mailbox;

  pcb->state = RUNNING;
  
  current_mailbox = mail;
  do {
    if ( strcmp(current_mailbox->name, instruct->resource) == 0 ) {
      /* We found the mailbox from which a message must be read. */
      break;
    }
    current_mailbox = current_mailbox->next;
  } while ( current_mailbox != NULL );

  printf("%s retrieved message from mailbox %s which says \033[22;32m %s \033[0m\n",
	      pcb->page->name, current_mailbox->name, current_mailbox->msg);


  instruct->msg = current_mailbox->msg;
  current_mailbox->msg = NULL;
  pcb->next_instruction = pcb->next_instruction->next;
  dealloc_instruction(instruct);
}

/**
 * @brief Acquires the resource specified by resourceName.
 *
 * The function iterates over the list of resources trying to acquire the
 * resource specified by resourceName. If the resources is available, the
 * process acquires the resource. The resource is indicated as not available
 * in the resourceList and 1 is returned indicating that the resource has been
 * acquired successfully.
 *
 * @param resourceName The name of the resource to acquire.
 * @param resources The list of resources.
 * @param p The process which acquires the resource.
 *
 * @return 1 for TRUE if the resource is available. 0 for FALSE if the resource
 * is not available.
 */
/*
TODO: 
Implement function 
*/
int acquire_resource(char *resourceName, struct resource_t *resources, struct pcb_t *p) {

	for (; resources != NULL; resources = resources->next) {

		if (strcmp(resources->name, resourceName) == 0) {

			resources->available = FALSE;

			return TRUE;

		}

	}

	return FALSE;

}

/**
 * @brief Releases the resource specified by resourceName
 *
 * Iterates over the resourceList finding the resource which must be set to 
 * available again. The resource is then released from the process. 
 *
 * @param resourceName The name of the resource to release.
 * @param resources The list of available resources.
 * @param p The current process.
 *
 * @return 1 (TRUE) if the resource was released succesfully else 0 (FALSE).
 */

/*
TODO: 
Implement function
*/ 
int release_resource(char *resourceName, struct resource_t *resources, struct pcb_t *p) {

  struct resource_t *temp_res_list = p->resources;
  for (; temp_res_list != NULL; temp_res_list = temp_res_list->next) {

    if (strcmp(temp_res_list->name, resourceName) == 0) {

      temp_res_list->available = TRUE;

      return TRUE;

    }

  }

  return FALSE;

}

/**
 * @brief Add process to the terminatedQueue 
 *
 * @param current The process to be moved to the terminated
 * @param terminated_queue The terminated queue
 */
void process_to_terminateq(struct pcb_t **current, struct pcb_t **terminated_queue) {

	if (*current == NULL) {
		return;
	}

	#ifdef DEBUG
	printf("%s terminated\n", (*current)->page->name);
	#endif

	(*current)->state = TERMINATED;
	move_pcb_fromQ_toQ((*current)->page->name, current, terminated_queue);

}

// TODO
/**
 * @brief Checks to see if the current process has finished all its instructions
 *
 * Checks to see if the current process has finished all its instructions.
 *
 * @param firstPCB A pointer to start of all the processes.
 *
 * @return 1 (TRUE) if all the processes are terminated else 0 (FALSE).
 */
int processes_finished(struct pcb_t *pcb) {

  if (pcb == NULL) {
    return TRUE;
  } else if (pcb->next_instruction == NULL) {
    return TRUE;
  } else {
  	return FALSE;
  }

}

/**
 * @brief Detects deadlock. 
 *
 * This function implements a deadlock detection algorithm.
 *
 * @param firstPCB A pointer to the start of all the processes.
 *
 * @return 1 if all the processes are in deadlock, 2 if there is a blocked process and 
 * 0 otherwise.
 */
int processes_deadlocked(struct resource_t *resources, struct pcb_t 
	*ready_queue, struct pcb_t *waiting_queue, struct pcb_t *current) {

	#ifdef DEBUG
	printf("Checked for deadlock\n");
	#endif

	if (current != NULL) {
		return FALSE;
	}

	int num_processes = 0;
	struct pcb_t *temp = waiting_queue;

	for (; temp != NULL; temp = temp->next) {
		num_processes++;
	}

	// This is an array which lists all processes in the first loop
	int deadlock_links[num_processes];
	int blocked_list[num_processes];
	int current_pcb_num = 0;
	int previous_pcb_num = 0;
	char *res_waiting_for = NULL;

	for (int i = 0; i < num_processes; i++) {
		deadlock_links[i] = 0;
	}

	for (int i = 0; i < num_processes; i++) {
		current_pcb_num = i;
		res_waiting_for = NULL;
		for (int j = 0; j < num_processes; j++) {

			res_waiting_for = pcb_waiting_for_res(current_pcb_num, waiting_queue);
			previous_pcb_num = current_pcb_num;
			current_pcb_num = pcb_number_holding_res(res_waiting_for, waiting_queue);
			if (current_pcb_num == -1) {
				blocked_list[previous_pcb_num] = 1;
				break;
			}
			deadlock_links[current_pcb_num] = 1;
		}
	}

	// Checks to see if process is blocked
	for (int i = 0; i < num_processes; i++) {
		if (blocked_list[i] == 1) {
			
			printf("No deadlock found but blocked process(es) found: ");
			for (int j = 0; j < num_processes; j++) {
				if (blocked_list[j] == 1) {
					printf("P%d ", pcb_name_val(j, waiting_queue));
				}
			}
			printf("\n");
			
			return 2;
		}
	}

	// Otherwise a deadlock was found
	
	printf("Deadlock found: P%d", 1);
	for (int i = 1; i < num_processes; i++) {
		if (deadlock_links[i] != 0) {
			printf(" -> P%d", i + 1);
		}
	}
	for (int i = 0; i < num_processes; i++) {
		if (deadlock_links[i] != 0) {
			printf(" -> P%d", i + 1);
			break;
		}
	}
	printf("\n");
	
	
	return TRUE;

}

/**
 * @brief Returns the index of a pcb in the waiting queue
 * 
 * Used to find the index of a pcb in the waiting queue
 *
 * @param pcb The pcb to find the index for
 * @param waiting_queue The waiting queue of all processes
 *
 * @return int The index of the pcb in the waiting queue. -1 if it
 * is not found in the waiting queue.
 */
int current_pcb_index(struct pcb_t *pcb, struct pcb_t *waiting_queue) {

	int i = 0;
	for (; waiting_queue != NULL; waiting_queue = waiting_queue->next) {
		if (strcmp(pcb->page->name, waiting_queue->page->name) == 0) {
			return i;
		}
		i++;
	}
	return -1;
}

/**
 * @brief Used for constructing a pcb name
 * 
 * Used in for constructing a pcb name by finding the value of the second 
 * char in a pcb's name
 *
 * @param index The index of the pcb in the waiting queue
 * @param waiting_queue The waiting queue of all processes
 *
 * @return int The index of the pcb in the waiting queue
 */
int pcb_name_val(int index, struct pcb_t *waiting_queue) {
	for (int i = 0; i < index; i++) {
		waiting_queue = waiting_queue->next;
	}
	return waiting_queue->page->name[1] - 48;
}

/**
 * @brief Gets the name of the resource that a specific pcb is waiting for
 * 
 * Used in deadlock detection, getting the name of a resource that a pcb is
 * waiting for is used to determine the index of a pcb holding that resource.
 * Finds the resource that a pcb at the specified index in the waiting queue
 * is waiting for.
 *
 * @param index The index of the pcb in the waiting queue
 * @param waiting_queue The waiting queue of all processes
 *
 * @return int The index of the pcb in the waiting queue
 */
char *pcb_waiting_for_res(int index, struct pcb_t *waiting_queue) {

	char *res_name = NULL;

	if (index == -1) {
		return NULL;
	}
	for (int i = 0; i < index; i++, waiting_queue = waiting_queue->next)
		;
	if (waiting_queue == NULL) {
		printf("We should not be here...\n");
	}
	res_name = waiting_queue->next_instruction->resource;
	if (res_name == NULL) {
		printf("Houston, we have a problem...\n");
	}
	return res_name;
}

/**
 * @brief Gets the index of the first pcb holding a specific resource
 * 
 * Used in deadlock detection, getting the index of a pcb holding a resource
 * helps to create an array that is used in determing a deadlock. Loops through
 * the waiting queue and each pcb's resources to find a specific resource.
 *
 * @param res_name The name of the resource we want to find a pcb index for
 * @param waiting_queue The waiting queue of all processes
 *
 * @return int The index of the pcb in the waiting queue
 */
int pcb_number_holding_res(char *res_name, struct pcb_t *waiting_queue) {

	struct resource_t *tmp_res = NULL;
	for (int i = 0; waiting_queue != NULL; waiting_queue = waiting_queue->next, i++) {
		for (tmp_res = waiting_queue->resources; tmp_res != NULL; tmp_res = tmp_res->next) {
			if (strcmp(tmp_res->name, res_name) == 0) {
				return i;
			}
		}
	}
	return -1;

}

/**
 * @brief Gets the scheduler out of a deadlocked state
 * 
 * If a deadlock occurs then this function is used to resolve the deadlock by freeing 
 * the resources of the first pcb in the waiting queue. It then also moves the first pcb
 * in the ready queue to be the currently running process.
 *
 * @param resources The list of all resources that are available to pcbs
 * @param ready_queue The ready queue of all processes
 * @param waiting_queue The waiting queue of all processes
 * @param current The currently running process
 *
 * @return void
 */
void free_deadlock(struct resource_t **resources, struct pcb_t 
	**ready_queue, struct pcb_t **waiting_queue, struct pcb_t **current) {

	#ifdef DEBUG
	printf("Attempt to recover from deadlock\n");
	#endif

	char *res_to_free = (*waiting_queue)->next_instruction->resource;
	struct pcb_t *temp_pcb = NULL;
	struct resource_t *temp_res = NULL;

	// Free the resource from the process that is currently holding it
	for (temp_pcb = *waiting_queue; temp_pcb != NULL; temp_pcb = temp_pcb->next) {
		for (temp_res = temp_pcb->resources; temp_res != NULL; temp_res = temp_res->next) {
			if (strcmp(temp_res->name, res_to_free) == 0) {
				move_resource_fromQ_toQ(res_to_free, &(temp_pcb->resources), resources);
				free_pcbs_requesting_res(res_to_free, waiting_queue, ready_queue);
				break;
			}
		}
	}
	if (*ready_queue == NULL) {
		printf("Ready queue should not be null.\n");
	}
	if (*resources == NULL) {
		printf("Resources should not be null\n");
	}
	move_pcb_fromQ_toQ((*ready_queue)->page->name, ready_queue, current);
	(*current)->state = RUNNING;

}

/**
 * @brief Moves certain pcbs from waiting queue to ready queue
 * 
 * After a resource has been released then the waiting queue is checked for processes 
 * requesting that resource. If such processes are found then they are moved to the 
 * ready queue as their resource is now available
 *
 * @param res_freed Name of the resource that has been freed
 * @param waiting_queue The waiting queue of all processes
 * @param ready_queue The ready queue of all processes
 *
 * @return void
 */
void free_pcbs_requesting_res(char *res_freed, struct pcb_t **waiting_queue, struct pcb_t **ready_queue) {

	struct pcb_t *temp_pcb = NULL;

	for (temp_pcb = *waiting_queue; temp_pcb != NULL; temp_pcb = temp_pcb->next) {
		// Comparing the resource this pcb is requesting to the resource that was freed
		if (strcmp(temp_pcb->next_instruction->resource, res_freed) == 0) {
			move_pcb_fromQ_toQ(temp_pcb->page->name, waiting_queue, ready_queue);
		}
	}
}

/**
 * @brief Adds a resource to a queue
 * 
 * Add a resource to a queue
 *
 * @param to_add Pointer to the resource to add to the list
 * @param resources_to The queue to which the resource must be added
 *
 * @return void
 */
void add_resource_to_queue(struct resource_t *to_add, struct resource_t **resources_to) {

  struct resource_t *res_list = *resources_to;

  if (*resources_to == NULL) {
    *resources_to = to_add;
  } else {
    for (; res_list->next != NULL; res_list = res_list->next)
    	;
    res_list->next = to_add;
  }
}

/**
 * @brief Moves a resource from one queue to another
 * 
 * Moves a resource from one queue to another. This is a very common task so it was
 * more efficient to write a function for this task alone.
 *
 * @param resource_name The name of the resource to move from the one queue to the other
 * @param resources The queue from which the resource must be removed
 * @param resources_to The queue to which the resource must be migrated
 *
 * @return void
 */
void move_resource_fromQ_toQ(char *resource_name, struct resource_t **resources, struct resource_t **resources_to) {

  struct resource_t *previous = *resources;
  struct resource_t *current_res = *resources;
  struct resource_t *temp = NULL;

  if (strcmp((*resources)->name, resource_name) == 0) {

    temp = *resources;
    *resources = (*resources)->next;
    temp->next = NULL;
    add_resource_to_queue(temp, resources_to);

  } else {
	// printf("matchy = %d\n\n", strcmp(current_res->next->name, resource_name) == 0);
    for (; current_res != NULL; previous = current_res, current_res = current_res->next) {
      if (strcmp(current_res->name, resource_name) == 0) {
        previous->next = current_res->next;
        current_res->next = NULL;
        add_resource_to_queue(current_res, resources_to);
      }
    }

  }
}

/**
 * @brief Adds a pcb to a queue
 * 
 * Add a pcb to a queue
 *
 * @param to_add Pointer to the pcb to add to the list
 * @param pcb The queue to which the pcb must be added
 *
 * @return void
 */
void add_pcb_to_queue(struct pcb_t *to_add, struct pcb_t **pcb) {

  struct pcb_t *pcb_list = *pcb;

  if (*pcb == NULL) {
    *pcb = to_add;
  } else {
    for (; pcb_list->next != NULL; pcb_list = pcb_list->next) {
			if (strcmp(pcb_list->page->name, to_add->page->name) == 0) {
				printf("PCB already in list!\n");
				return;
			}
		}
    pcb_list->next = to_add;
  }
}

/**
 * @brief Moves a pcb from one queue to another
 * 
 * Moves a pcb from one queue to another. This is a very common task so it was
 * more efficient to write a function for this task alone.
 *
 * @param pcb_name The name of the pcb to move from the one queue to the other
 * @param pcb_from The queue from which the pcb must be removed
 * @param pcb_to The queue to which the pcb must be migrated
 *
 * @return void
 */
void move_pcb_fromQ_toQ(char *pcb_name, struct pcb_t **pcb_from, struct pcb_t **pcb_to) {

  if (*pcb_from == NULL) {
    return;
  }

  struct pcb_t *previous_pcb = NULL;
  struct pcb_t *current_pcb = *pcb_from;
  struct pcb_t *temp = NULL;

  if (strcmp((*pcb_from)->page->name, pcb_name) == 0) {
    temp = *pcb_from;
    *pcb_from = (*pcb_from)->next;
    temp->next = NULL;
    add_pcb_to_queue(temp, pcb_to);
  } else {
    for (; current_pcb != NULL; previous_pcb = current_pcb, current_pcb = current_pcb->next) {
      if (strcmp(current_pcb->page->name, pcb_name) == 0) {
        previous_pcb->next = current_pcb->next;
        current_pcb->next = NULL;
        add_pcb_to_queue(current_pcb, pcb_to);
      }
    } 
  }
}
