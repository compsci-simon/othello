/**
 * @file main.c
 * @author Francois de Villiers (Demi 2011-2012)
 * @description The main function file for the process management application.
 *
 * @mainpage Process Simulation
 *
 * @section intro_sec Introduction
 *
 * The project consists of 3 main files parser, loader and manager. Which 
 * respectively handles the loading, parsing and management of the processes.
 *
 * @section make_sec Compile
 * 
 * $ make
 *
 * @section run_sec Execute
 *
 * $ ./process-management data/process.list
 * 
 */


#include <stdio.h>
#include <stdlib.h>

#include "parser.h"
#include "loader.h"
#include "manager.h"
#define DEBUG 1
void print_ready_queue ( struct pcb_t *pcb );
void print_mailboxes ( struct mailbox_t *mail );
void print_instruction( struct instruction_t *instr);
void print_resources_list(struct resource_t *resources);
void print_process_resources(struct pcb_t *process);
void order_ready_queue_on_priority(struct pcb_t **pcb_queue);

int main(int argc, char** argv) {
  char* filename;
  struct pcb_t *ready_queue;
  struct resource_t *resources;
  struct mailbox_t *mailboxes;
  int type = 0;
  int quantum = 0;

  filename = NULL;

  if ( argc < 1 ) {
    return EXIT_FAILURE; 
  }
  
  filename = argv[1];
  if (argc > 2) {
    type = atoi(argv[2]);
  }
  if (argc > 3) {
    quantum = atoi(argv[3]);
  }
  parse_process_file(filename, type);

  ready_queue = get_loaded_processes();
  resources = get_available_resources();
  mailboxes = get_mailboxes();

  if (type == 1) {
    order_ready_queue_on_priority(&ready_queue);
  }

#ifdef DEBUG
  print_ready_queue(ready_queue);
  print_mailboxes(mailboxes);
#endif

  //TODO: Schedule processes
	schedule_processes(&ready_queue, &resources, mailboxes, type, quantum);
	
  //dealloc_pcb_list(ready_queue);
  //dealloc_data_structures();
  
  return EXIT_SUCCESS;
}

#ifdef DEBUG
void print_ready_queue ( struct pcb_t *pcb ) {
  struct pcb_t *current_pcb;
  current_pcb = pcb;
  printf("pcb full list:\n");
  do {
    printf("  %s (%d):\n", current_pcb->page->name, current_pcb->state);
    print_instruction(current_pcb->next_instruction);
    current_pcb = current_pcb->next;
  } while(current_pcb != NULL);
  printf("\n");
}

void print_instruction(struct instruction_t *instr) {
    while (instr != NULL) {
	switch (instr->type) {
	case REQ_V: 
          printf("    (req %s)\n", instr->resource);
	  break;
	case REL_V: 
          printf("    (rel %s)\n", instr->resource);
	  break;
	case SEND_V :
          printf("    (send %s %s)\n", instr->resource, instr->msg);
	  break;
	case RECV_V:
          printf("    (recv %s %s)\n", instr->resource, instr->msg);
	  break;
      }
      instr = instr->next;
    }
}

void print_mailboxes ( struct mailbox_t *mailbox ) {
  struct mailbox_t *current_mailbox = mailbox;

  printf("Mailboxes: ");
  while (current_mailbox != NULL) {
    printf("%s ", current_mailbox->name);
    current_mailbox = current_mailbox->next;
  } 
  printf("\n");
}
#endif

void print_resources_list(struct resource_t *resources) {

	printf("Printing resource queue:\n");
	for (; resources != NULL; resources = resources->next) {

		printf("\tResource %s\n\tIs ready: %d\n", resources->name, resources->available);

	}
	printf("\n");

}

void print_process_resources(struct pcb_t *process) {

  struct resource_t *res = process->resources;
  printf("Printing process resources:\n");
  for (; res != NULL; res = res->next) {

    printf("\tRes = %s\n", res->name);

  }

}

/**
 * @brief Orders the ready queue if priority scheduling is used
 *
 * Orders the ready queue based on the priority of the pcb's. The 
 * lower the priority, the closer to the head of the queue.
 *
 * @param ready_queue The ready queue to be reordered
 */
void order_ready_queue_on_priority(struct pcb_t **ready_queue) {

  struct pcb_t *new_start = NULL;
  struct pcb_t *current_old = *ready_queue;
  struct pcb_t *current_new = NULL;
  struct pcb_t *previous_new = NULL;
  struct pcb_t *temp = NULL;

  temp = current_old;
  current_old = current_old->next;
  temp->next = NULL;
  current_new = temp;
  new_start = current_new;

  while (current_old != NULL) {

    current_new = new_start;
    previous_new = NULL;
    temp = NULL;
    while (current_new != NULL) {

      if (current_old->priority < current_new->priority) {
        // insert into new list
        temp = current_old;
        current_old = current_old->next;
        temp->next = current_new;
        current_new = temp;
        if (previous_new != NULL) {
          previous_new->next = temp;
        } else {
          new_start = current_new;
        }
        break;
      } else {
        // loop to next pcb in new list
        previous_new = current_new;
        current_new = current_new->next;
        if (current_new == NULL) {
          // enter here if pcb has lowest priority in new pcb queue
          temp = current_old;
          current_old = current_old->next;
          previous_new->next = temp;
          temp->next = NULL;
          break;
        }
      }

    // end of inner loop
    }
    // end of outer loop
  }

  *ready_queue = new_start;

}



