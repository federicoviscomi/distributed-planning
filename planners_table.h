/**
 * \file planners_table.h 
 * \author Federico Viscomi
 * \brief gestisce le tabelle in memoria principale
 * 
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.
 */

#ifndef _PLANNERS_TABLE_H
#define _PLANNERS_TABLE_H

#include <stdbool.h>

struct hash_entry {
    struct hash_entry *next;
    char planner_name[LAGENDA];
    elem_t *planner_list_head;
    char modified;
};

void initTable();

bool plannersTableAdd(char *planner_name, elem_t * planner_list);

elem_t **plannerTableGet(char *planner_name);

bool plannerTableRemove(char *planner_name);

bool plannerTableNext(elem_t ** planner_list, char **planner_name);

void print_table();

void plannerTableSetModified(char *planner_name);

#endif				/* _PLANNERS_TABLE_H 
				 */
