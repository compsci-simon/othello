/**
  * @file loader.h
  * @description A definition of the structures and functions to store and
  *              represent the different elements of an process.
  */

#ifndef _LOADER_H
#define _LOADER_H

/** The process NEW state */
#define NEW 0
/** The process READY state */
#define READY 1
/** The process RUNNNING state */
#define RUNNING 2
/** The process WAITING state */
#define WAITING 3
/** The process TERMINATED state */
#define TERMINATED 4

#define REQ_V 0
#define REL_V 1
#define SEND_V 2
#define RECV_V 3

/**
 * Each process has a list of instructions to execute, with a pointer to the
 * next instruction to execute. An instruction stores the type, participating
 * resource and if it is a send and receive instruction the message. A pointer
 * to the next instruction is also included.
 */
struct instruction_t {
  /** The type of instruction */
  int type;
  /** The resource or mailbox name used in the instruction */
  char *resource; /* any resource, including a mailbox name */
  /** The message of a send and receive instruction */
  char *msg;
  /** A pointer to the next instruction */
  struct instruction_t *next;
};

/**
 * A process page stores the name and number of the process.
 */
struct page_t {
  /** Stores the number of the process. */
  int number;
  /** The name of the process */
  char *name;
  /** A Linked list of the process's instructions */
  struct instruction_t *first_instruction;
};

/** 
 * Represents the mailbox resource. A message sent between two processes are
 * stored and retrieve from the mailbox struct.
 */
struct mailbox_t {
  /** The name of the mailbox. Used to find the correct mailbox for sending and
   * receiving */
  char *name;
  /** The variable is used to store the sent message for retrieval */
  char *msg;
  /** A pointer to the next mailbox in the system */
  struct mailbox_t *next;
};

/** 
 * Used to store the available resources in the system as well as the acquired
 * resources for each process.
 */
struct resource_t {
  /** The name of the resource */
  char *name;
  /** The status of the result, either available or occupied */
  int available;
  /** The next resource in the list */
  struct resource_t *next;
};

/** 
  * The process control block (PCB). The PCB should point to the
  * page-directory that should point to the pages in memory where
  * a process is stored. In this project each process will be
  * stored in a data structure, called a page; therefore  the PCB
  * can simply point to the page.
  */
struct pcb_t {
  /** The process page, which stores the number and name of the process */
  struct page_t *page;
  /** The current state of the process */
  int state;
  /** A pointer to the next instruction to be executed */
  struct instruction_t *next_instruction;
  /** The process priority */
  int priority; 
  /** The resources which the current process occupies */
  struct resource_t *resources;
  /** Pointer to the next process control block in memory */
  struct pcb_t *next;
};

/*
 * Creates a process control block (pcb) for process process_name 
 */
void load_process ( char* process_name, int prio );

/*
 * Adds a priority to the last process in the job pool
 */
void load_priority ( int priority );

/*
 * Loads and stores an instruction of process process_name 
 */
void load_instruction ( char* process_name, char* instruction, 
    char* resource_name, char *msg );

/*
 * Loads a mailbox 
 */
void load_mailbox ( char* mailboxName );

/*
 * Loads the available system resources 
 */
void load_resource ( char* resource_name );

/*
 * Returns a pointer to the first pcb in the list of loaded processes
 */
struct pcb_t* get_loaded_processes();

/*
 * Returns the list of available resources
 */
struct resource_t* get_available_resources();

/*
 * Returns a list of all the mailboxes
 */
struct mailbox_t* get_mailboxes();

/*
 * frees the job pool, resource lists, and mailboxes 
 */
void dealloc_data_structures();

/*
 * frees a linked list of PCBs 
 */
void dealloc_pcb_list();

/*
 * frees an instruction list
 */
void dealloc_instruction(struct instruction_t *i);

#endif
