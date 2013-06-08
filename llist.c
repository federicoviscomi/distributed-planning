/*
 *
 * \file llist.c 
 * \author Federico Viscomi 412006
 * 
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "lbase.h"
#include "llist.h"

static bool is_empty(char *s)
{
    while (*s)
	if (!isspace(*s++))
	    return false;
    return true;
}

int add(elem_t ** agenda, evento_t * ev)
{
    /* il nuovo elemento da aggiungere nell'agenda */
    elem_t *new_element;
    /* usato per scorrere l'agenda */
    elem_t *previous;

    if (agenda == NULL || ev == NULL)
	return -1;
    if ((new_element = malloc(sizeof(elem_t))) == NULL) {
	fprintf(stderr, "\n libdplan: add(elem_t**, evento_t*): memory error");
	return 1;
    }
    memset(new_element, 0, sizeof(elem_t));
    if ((new_element->ptev = malloc(sizeof(evento_t))) == NULL) {
	fprintf(stderr, "\n libdplan: add(elem_t**, evento_t*): memory error");
	return 1;
    }
    memcpy(new_element->ptev, ev, sizeof(evento_t));
    if (*agenda == NULL) {
	new_element->next = NULL;
	*agenda = new_element;
	return 0;
    }
    if (strncmp((*agenda)->ptev->data, new_element->ptev->data, LDATA) > 0) {
	new_element->next = *agenda;
	*agenda = new_element;
    } else {
	previous = *agenda;
	while (previous->next && strncmp(previous->next->ptev->data, new_element->ptev->data, LDATA) <= 0)
	    previous = previous->next;
	new_element->next = previous->next;
	previous->next = new_element;
    }
    return 0;
}

int cerca(char data[], elem_t * agenda, elem_t ** trovati)
{
    /* numero di elementi trovati */
    int elements;
    /* la coda della lista degli eventi trovati */
    elem_t *tail;

    if (agenda == NULL || trovati == NULL || !checkDate(data)) {
	fprintf(stderr, "\n argomenti non validi\n ");
	if (trovati != NULL)
	    *trovati = NULL;
	return -1;
    }
    *trovati = NULL;
    tail = NULL;
    for (elements = 0; agenda != NULL; agenda = agenda->next) {
	/* nuovo elemento da aggiungere nella lista degli eventi trovati */
	elem_t *new_element;
	if (matchData(data, agenda->ptev) == 1) {
	    elements++;
	    if ((new_element = malloc(sizeof(elem_t))) == NULL) {
		fprintf(stderr, "\n llist: MEMORY ERROR");
		if (*trovati) {
		    dealloca_lista(*trovati);
		    *trovati = NULL;
		}
		return -1;
	    }
	    if ((new_element->ptev = malloc(sizeof(evento_t))) == NULL) {
		fprintf(stderr, "\n llist: MEMORY ERROR");
		if (*trovati) {
		    dealloca_lista(*trovati);
		    *trovati = NULL;
		}
		return -1;
	    }
	    memcpy(new_element->ptev, agenda->ptev, sizeof(evento_t));
	    new_element->next = NULL;
	    if (*trovati == NULL) {
		*trovati = new_element;
		tail = new_element;
	    } else {
		tail->next = new_element;
		tail = new_element;
	    }
	}
    }
    return elements;
}


elem_t *rimuovi(char pattern[], elem_t * agenda)
{
    /* memoria da deallocare */
    elem_t *to_free;
    /* usati per scandire l'agenda */
    elem_t *previous, *current;

    to_free = NULL;
    while (agenda != NULL && matchPattern(pattern, agenda->ptev) == 1) {
	to_free = agenda;
	agenda = agenda->next;
	free(to_free->ptev);
	free(to_free);
    }
    if (agenda == NULL)
	return NULL;
    for (current = agenda->next, previous = agenda; current != NULL;) {
	if (matchPattern(pattern, current->ptev) == 1) {
	    to_free = current;
	    previous->next = current->next;
	    current = current->next;
	    free(to_free->ptev);
	    free(to_free);
	} else {
	    previous = current;
	    current = current->next;
	}
    }
    return agenda;

}

int loadAgenda(FILE * ingresso, elem_t ** agenda)
{
    /* numero di eventi letti */
    int elements = 0;
    /* buffer usato per leggere i record dal file */
    char event_buffer[LRECORD + 20];
    /* l'evento corrente nella scansione dell'agenda */
    evento_t *event;

    for (*agenda = NULL; fgets(event_buffer, LRECORD, ingresso) != NULL;) {
	if (!is_empty(event_buffer)) {
	    elements++;
	    if ((event = convertiRecord(event_buffer)) == NULL) {
		fprintf(stderr,
			"\n llist: loadAgenda(FILE *, elem_t **): error while creating event from line \"%s\" \n",
			event_buffer);
		return -1;
	    }
	    if (add(agenda, event)) {
		fprintf(stderr,
			"\n llist: loadAgenda(FILE *, elem_t **): error while adding event parsed from line \"%s\" \n",
			event_buffer);
		return -1;
	    }
	    free(event);
	}
    }

    return elements;
}

int storeAgenda(FILE * uscita, elem_t * agenda)
{
    /* il numero di eventi memorizzati */
    int elements;
    /* il buffer usato per convertire gli eventi in stringhe
       e memorizzarle sul file */
    char buff[LRECORD + 2];
    for (elements = 0; agenda != NULL; agenda = agenda->next, elements++) {
	if (convertiEvento(agenda->ptev, buff)) {
	    fprintf(stderr, "\n llist: storeAgenda(FILE *, elemt_t *): error in convertiEvento: event not valid");
	    return -1;
	}
	if (fputs(buff, uscita) == EOF || fputc('\n', uscita) == EOF) {
	    fprintf(stderr, "\n llist: storeAgenda(FILE *, elemt_t *): error in writing on file");
	    return -1;
	}
    }
    return elements;
}

void dealloca_lista(elem_t * lista)
{
    /* la memoria da deallocare */
    elem_t *to_free;

    while (lista != NULL) {
	to_free = lista;
	lista = lista->next;
	free(to_free->ptev);
	free(to_free);
    }
}
