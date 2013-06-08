/**
 * \file planners_table.c 
 * \author Federico Viscomi 412006
 * \brief gestore delle agende in memoria principale
 * 
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.
 */

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "llist.h"
#include "planners_table.h"

#define TABLE_LEN 512

/** tabella hash */
static struct hash_entry *buckets[TABLE_LEN];

/** inizializza la tabella */
void initTable()
{
    memset(buckets, 0, sizeof(buckets));
}

/** calcola l'indice hash in base al nome dell'agenda 
    \param il nome dell'agenda
    \return l'indice hash calcolato
*/
static unsigned int getIndex(char *planner_name)
{
    /** l'hash di planner_name*/
    unsigned int index;

    for (index = 0; *planner_name; planner_name++)
	index = index * 7 + *planner_name;

    return index % TABLE_LEN;
}

/** aggiunge nella tabella l'agenda con nome planner_name e elementi planner_list
    \param planner_name il nome dell'agenda da aggiungere
    \param planner_list il puntatore al primo elemento della lista che 
                        rappresenta l'agenda
   \return true in caso di fallimento; false altrimenti */
bool plannersTableAdd(char *planner_name, elem_t * planner_list)
{
    /** l'indice hash di planner_name */
    unsigned int index;
    /** il nuovo elemento della tabella */
    struct hash_entry *new_entry;

    index = getIndex(planner_name);
    if ((new_entry = malloc(sizeof(struct hash_entry))) == NULL) {
	fprintf(stderr, "\n deserver: planners_table: add_entry_to_planners_table: ERROR: memory error");
	return true;
    }
    new_entry->next = buckets[index];
    new_entry->planner_list_head = planner_list;
    new_entry->modified = true;
    strncpy(new_entry->planner_name, planner_name, LAGENDA);
    buckets[index] = new_entry;
    return false;
}

/** cerca l'agenda di nome planner_name nella tabella
    se la trova restituisce il puntatore al primo elemento della lista
    che memorizza l'agenda altrimenti restituisce NULL
    \param planner_name il nome dell'agenda da cercare
    \return l'agenda nel caso un cui sia presente altrimenti NULL*/
elem_t **plannerTableGet(char *planner_name)
{
    /** l'indice hash di planner_name*/
    unsigned int index;

    struct hash_entry *bucket;

    index = getIndex(planner_name);
    for (bucket = buckets[index]; bucket; bucket = bucket->next)
	if (strncmp(planner_name, bucket->planner_name, LAGENDA) == 0)
	    return &(bucket->planner_list_head);

    return NULL;
}

/** se l'agenda di nome planner_name si trova nella tabella 
    imposta il flag relativo alla modifica
    \param planner_name */
void plannerTableSetModified(char *planner_name)
{
    /** l'indice hash di planner_name */
    unsigned int index;
    struct hash_entry *bucket;

    index = getIndex(planner_name);
    for (bucket = buckets[index]; bucket; bucket = bucket->next)
	if (strncmp(planner_name, bucket->planner_name, LAGENDA) == 0)
	    bucket->modified = true;
}

/** rimuove l'agenda di nome planner_name dalla tabella se e' presente 
    \param planner_name il nome dell'agenda da rimuovere
    \return true se non e' stata rimossa nessun agenda; 
            false altrimenti */
bool plannerTableRemove(char *planner_name)
{
    /** l'indice hash di planner_name */
    unsigned int index;
    struct hash_entry *bucket, *to_free = NULL;

    index = getIndex(planner_name);
    bucket = buckets[index];
    if (bucket == NULL)
	return true;
    if (strncmp(bucket->planner_name, planner_name, LAGENDA) == 0) {
	to_free = bucket;
	buckets[index] = buckets[index]->next;
	free(to_free);
	return false;
    }
    for (; bucket->next; bucket = bucket->next)
	if (strncmp(bucket->next->planner_name, planner_name, LAGENDA) == 0) {
	    to_free = bucket->next;
	    bucket->next = bucket->next->next;
	    free(to_free);
	    return false;
	}
    return true;
}

/** implementa un iteratore su tutte le agende.
    memorizza nelle locazioni puntate dagli argomenti rispettivamente
    il puntatore al primo elemento e il nome dell'agenda successiva
    \param planner_list il puntatore alla locazione nella quale memorizzare l'agenda
    \param planner_name il puntatore alla locazione nella quale memorizzare il nome dell'agenda
    \return true quando non ci sono piu' agende; false altrimenti 
    */
bool plannerTableNext(elem_t ** planner_list, char **planner_name)
{
    /** lo stato interno della funzione. specifica l'indice della lista di bucket */
    static int bucket = 0;
    /** il bucket all'interno della lista corrente */
    static struct hash_entry *next_bucket;

    if (bucket == 0)
	next_bucket = buckets[0];
    if (next_bucket != NULL && next_bucket->modified) {
	*planner_list = next_bucket->planner_list_head;
	*planner_name = next_bucket->planner_name;
	next_bucket = next_bucket->next;
	bucket = 1;
	return true;
    }
    for (; bucket < TABLE_LEN; bucket++) {
	if (buckets[bucket] != NULL && buckets[bucket]->modified) {
	    next_bucket = buckets[bucket];
	    *planner_list = next_bucket->planner_list_head;
	    *planner_name = next_bucket->planner_name;
	    next_bucket = next_bucket->next;
	    bucket++;
	    return true;
	}
    }
    return false;
}
