/**
 * @file manager.h
 */
#ifndef _MANAGER_H
#define _MANAGER_H

#include "loader.h"

void schedule_processes(struct pcb_t **pcb, 
    struct resource_t **resource, struct mailbox_t *mail, int type, int quantum);

#endif
