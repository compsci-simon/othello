/**
 * @file loader.c
 */

#include "loader.h"
#include "syntax.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void dealloc_page(struct page_t *p);
void dealloc_resource_list(struct resource_t *r);
void dealloc_mailboxes();
void dealloc_data_structures();

void print_pcb_list();
void print_resource_list();
void print_mailbox_list();
void print_instruction_now();

struct pcb_t *job_pool = NULL;
void add_to_job_pool(struct pcb_t *pcb); 

struct pcb_t *last_proc_pcb = NULL;
char *last_proc_name = "";
int last_proc_number = 0;

struct resource_t *first_resource = NULL;
struct resource_t *last_resource = NULL;

struct instruction_t *first_instruction = NULL;
struct instruction_t *last_instruction = NULL;

struct mailbox_t *first_mailbox = NULL;
struct mailbox_t *last_mailbox = NULL;

/**
 * @brief Initialises and loads the processes specified in the process.list
 * file.
 *
 * This function initialises a new process control block for the process being
 * loaded from the process.list file. It initialises a number of pointers to
 * NULL as well as setting the processState to NEW. 
 *
 * @param process_name The name of the new process to load
 */
void load_process ( char* process_name, int prio ) {
  struct page_t *new_page = malloc(sizeof(struct page_t));
  struct pcb_t *pcb = NULL;

  pcb = malloc(sizeof(struct pcb_t));
  pcb->page = new_page;
  pcb->state = NEW;
  pcb->next_instruction = NULL;
  pcb->priority = prio;
  pcb->resources = NULL;
  pcb->next = NULL;

  pcb->page->name = process_name;
  pcb->page->number = last_proc_number;
  last_proc_number++;


  add_to_job_pool(pcb);

}

/**
 * @brief Adds a priority to the last pcb in the job pool
 *
 * Adds a priority to the last pcb in the job pool
 *
 * @param the priority to add
 */
void load_priority (int priority) {

  struct pcb_t *current_pcb;

  if (job_pool == NULL) {
    printf("We shouldn't be here...\n\n");
    return;
  } else {
    current_pcb = job_pool;
    while (current_pcb->next != NULL) {
      current_pcb = current_pcb->next; 
    }
    current_pcb->priority = priority;
  }

}

/**
 * @brief Adds the new pcb to the job pool 
 *
 * Adds a newly created pcb to the job pool 
 *
 * @param the pcb to add
 */
void add_to_job_pool(struct pcb_t *new_pcb) {
  struct pcb_t *current_pcb;

  if (job_pool == NULL) {
    job_pool = new_pcb;
  } else {
    current_pcb = job_pool;
    while (current_pcb->next != NULL) {
      current_pcb = current_pcb->next; 
    }
    current_pcb->next = new_pcb;
  }

  #ifdef DEBUG
    printf("Added Process %s %d; ",new_pcb->page->name, new_pcb->page->number); 
    print_pcb_list(job_pool); 
  #endif
}

/**
 * @brief Loads the mailbox from the process.list file.
 *
 * Loads a mailbox resource and adds it to the list of mailboxes. 
 *
 * @param mailbox_name The name of the mailbox to load.
 */
void load_mailbox(char* mailbox_name) {
  struct mailbox_t *tmp_mailbox = NULL;
  
  if (first_mailbox == NULL) {
    first_mailbox = malloc(sizeof(struct mailbox_t)); 
    first_mailbox->next = NULL;
    last_mailbox = first_mailbox;
  } else {
    tmp_mailbox = malloc(sizeof(struct mailbox_t));
    last_mailbox->next = tmp_mailbox;
    last_mailbox = tmp_mailbox;
  }
  last_mailbox->name = mailbox_name;
  last_mailbox->next = NULL;

 #ifdef DEBUG
   printf("Added Mailbox %s; ", mailbox_name);
   print_mailbox_list();
 #endif
}

/**
 * @brief Loads a resource from the process.list file.
 *
 * Loads a resource and adds it to the list of resources. The resource
 * is indicated as available and the resource name is stored.
 *
 * @param resource_name The name of the resource to load.
 */
void load_resource ( char *resource_name ) {
  struct resource_t *tmp_resource = NULL;

  if (first_resource == NULL) {
    first_resource = malloc(sizeof(struct resource_t));
    last_resource = first_resource;
  } else {
    tmp_resource = malloc(sizeof(struct resource_t));
    last_resource->next = tmp_resource;
    last_resource = tmp_resource;
  }
  last_resource->name = resource_name;
  /* 1 = Available, 0 = Unavailable */
  last_resource->available = 1;
  last_resource->next = NULL;

#ifdef DEBUG
  printf("Added resource %s; ", resource_name);
  print_resource_list();
#endif
}

/**
 * @brief Loads an instruction for a process.
 *
 * The function uses the process_name to locate the process for 
 * which the instruction should be loaded as well as the resource
 * on which the action is performed.
 *
 * @param process_name The name of the process for which to load the
 * instruction.
 * @param resource_name The name of the resource used in the instruction.
 * @param instruction Indicates the next request, release or message to send.
 */
void load_instruction(char *process_name, char *instruction, 
    char *resource_name, char *msg) {

  struct instruction_t *tmp_instr = NULL;
  struct pcb_t *pcb = NULL;

#ifdef DEBUG
  printf("Load instruction for %s: %s -> %s\n", process_name, instruction, resource_name);
#endif
  
  if (strcmp(last_proc_name, process_name) != 0) {
    first_instruction = malloc(sizeof(struct instruction_t));
    first_instruction->next = NULL;
    last_instruction = first_instruction;
  } else {
    tmp_instr = malloc(sizeof(struct instruction_t));
    tmp_instr->next = NULL;
    last_instruction->next = tmp_instr;
    last_instruction = tmp_instr;
  }

  last_instruction->resource = resource_name;
  if (strcmp(instruction, REQ) == 0) {
    last_instruction->type = REQ_V;
    last_instruction->msg = NULL;
  } else if (strcmp(instruction, REL) == 0) {
    last_instruction->type = REL_V;
    last_instruction->msg = NULL;
  } else if (strcmp(instruction, SEND) == 0) {
    last_instruction->type = SEND_V; 
    last_instruction->msg = msg;
  } else if (strcmp(instruction, RECV) == 0) {
    last_instruction->type = RECV_V;
    last_instruction->msg = msg;
  }

  pcb = job_pool;
  if (pcb != NULL) {
    do { 
      if(strcmp(pcb->page->name, process_name) == 0) break;
      pcb = pcb->next;
    } while (pcb != NULL); 
    pcb->next_instruction = first_instruction;
    pcb->page->first_instruction = first_instruction;
  }
  last_proc_name = process_name; 

}

/**
 * @brief Returns a pointer to the first process in the job_pool 
 *
 * Returns a pointer to the first process in the list
 * of loaded processes.
 *
 * @return firstPCB Pointer to the first process control block.
 */
struct pcb_t* get_loaded_processes() {
	struct pcb_t *temp = job_pool;
	for (; job_pool != NULL; job_pool = job_pool->next) {
		job_pool->state = READY;
	}
	job_pool = temp;
  return job_pool;
}

/**
 * @brief Returns the first pointer to the available resources.
 * 
 * Returns the first pointer to the available resources.
 *
 * @return first_resource Pointer to the resource list.
 */
struct resource_t* get_available_resources() {
  return first_resource;
}

/**
 * @brief Returns the first pointer to the available mailboxes.
 *
 * Returns the first pointer to the available mailboxes.
 *
 * @return first_mailbox Pointer to the mailbox list.
 */
struct mailbox_t* get_mailboxes() {
  return first_mailbox;
}

/**
 * @brief Returns the job_pool 
 *
 * Initialises and returns a pointer to the job_pool. If the queue
 * has been initialised it just returns the pointer to the queue.
 *
 * @return job_pool Pointer 
 */
/* TODO: *
 */
struct pcb_t * return_job_pool() {
	return job_pool;
}


/**
 * @brief Returns the waiting_queue
 *
 * Initialises and returns a pointer to the waiting_queue. If the queue
 * has been initialised it just returns the pointer to the queue.
 *
 * @return waiting_queue Pointer
 */
/* TODO: 
 */
struct pcb_t * waiting_queue() {

	return NULL;

}


/**
 * @brief Returns the terminated_queue
 *
 * Initialises and returns a pointer to the terminated_queue. If the queue
 * has been initialised it just returns the pointer to the queue.
 *
 * @return terminated_queue Pointer
 */
/* TODO: Implement function
 */
struct pcb_t * terminated_queue() {

	return NULL;
 
}


/**
 * @brief Frees the allocated memory for the page struct.
 *
 * Frees the name stored in the structed followed by the struct.
 */
void dealloc_page(struct page_t *p) {
  if (p != NULL) {
    free(p->name);
    free(p);
  }
}

/**
 * @brief Frees the allocated memory for the instruction struct.
 *
 * Frees the instruction struct.
 */
void dealloc_instruction(struct instruction_t *i) {
  if(i != NULL) {
    free(i);
  }
}

/**
 * @brief Frees the resources used in the system.
 *
 * Frees a linked list of resources 
 */
void dealloc_resource_list(struct resource_t *r) {
  struct resource_t *current_resource;
  struct resource_t *next_resource;

  if (r != NULL) {
    current_resource = r;
    do {
      next_resource = current_resource->next;
      free(current_resource);
      current_resource = next_resource;
    } while (current_resource != NULL);
  }
}

/**
 * @brief Frees all the memory allocated for a linked list of PCBs.
 *
 * Iterates over the loaded process PCBs, starting from current_pcb, freeing all the
 * allocated memory assigned to each process.
 */
void dealloc_pcb_list(struct pcb_t *current_pcb) {  
  struct pcb_t *next_pcb;  

  if (current_pcb != NULL) {
    do {
      dealloc_page(current_pcb->page);
      /* Instructions are freed after they are executed */ 
      if (current_pcb->resources != NULL) {
        dealloc_resource_list(current_pcb->resources);
      }

      next_pcb = current_pcb->next;
      free(current_pcb);
      current_pcb = next_pcb;
    } while(current_pcb != NULL);
  }
}


/**
 * @brief Frees the mailboxes used in the system.
 *
 * Free each of the mailboxes which are available and used in the system.
 */
void dealloc_mailboxes() {
  struct mailbox_t *current_mailbox;
  struct mailbox_t *next_mailbox;
  
  current_mailbox = get_mailboxes();
  if(current_mailbox != NULL) {
    do {
      free(current_mailbox->name);
      if(current_mailbox->msg != NULL) {
        free(current_mailbox->msg);
      }
      next_mailbox = current_mailbox->next;
      free(current_mailbox);
      current_mailbox = next_mailbox;
    } while (current_mailbox != NULL);
  }
}

/**
 * @brief Frees the memory for all the data structures 
 *
 */
void dealloc_data_structures() {
  struct resource_t *availableResources;

  //job pool empty - all pcbs moved to ready queue 
  free(job_pool); 

  /* frees the memory for resources not assigned to processes */  
  availableResources = get_available_resources();
  dealloc_resource_list(availableResources);

  dealloc_mailboxes();
}

#ifdef DEBUG
void print_pcb_list() {
  struct pcb_t *current_pcb;
  current_pcb = job_pool;
  printf("pcb list: ");
  do {
    printf("%s ", current_pcb->page->name);

    current_pcb = current_pcb->next;
  } while(current_pcb != NULL);
  printf("\n");
}

void print_resource_list() {
  struct resource_t *current_resource;
  current_resource = first_resource;
  printf("avalabile resources: ");
  do {
    printf("%s ", current_resource->name);
    current_resource = current_resource->next;
  } while(current_resource != NULL);
  printf("\n");
}

void print_mailbox_list() {
  struct mailbox_t *current_mailbox;
  current_mailbox = first_mailbox;
  printf("mailboxes : ");
  do {
    printf("%s ", current_mailbox->name);
    current_mailbox = current_mailbox->next;
  } while(current_mailbox != NULL);
  printf("\n");
}
#endif
