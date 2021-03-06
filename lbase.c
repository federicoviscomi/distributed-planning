/**
 * \file lbase.c
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


#define SKIP_SPACES(x) while(isspace(*x)) x++;
#define MIN(x, y) ((x) < (y) ? (x) : (y))

int checkDate(char date[])
{
    if (date == NULL)
	return false;
    if ((!isdigit(date[0]) && date[0] != '*')
	|| (!isdigit(date[1]) && date[1] != '*')
	|| date[2] != '-' || !isdigit(date[3])
	|| !isdigit(date[4]) || date[5] != '-' || !isdigit(date[6])
	|| !isdigit(date[7])
	|| !isdigit(date[8]) || !isdigit(date[9])
	)
	return false;
    return true;
}


evento_t *convertiRecord(char r[])
{
    /** l'evento creato */
    evento_t *new_event;
    /** puntatore alla stringa da convertire */
    char *p;

    int i;

    if (r == NULL) {
	errno = EINVAL;
	return NULL;
    }
    p = r;
    if ((new_event = (evento_t *) malloc(sizeof(evento_t))) == NULL) {
	perror("\n libdplan: convertiRecord(char []): memory error");
	return NULL;
    }
    /* converte la data */
    SKIP_SPACES(p);
    if ((i = sscanf(p, "%2s-%2s-%4s", new_event->data, new_event->data + 3, new_event->data + 6)) != 3) {
	fprintf(stderr, "\n libdplan: convertiRecord(char []): ERROR invalid data from record \"%s\"", r);
	errno = EINVAL;
	free(new_event);
	return NULL;
    }
    new_event->data[2] = new_event->data[5] = '-';
    if (!checkDate(new_event->data)) {
	fprintf(stderr,
		"\n libdplan: convertiRecord(char []): ERROR invalid data \"%s\" from record \"%s\"",
		new_event->data, r);
	errno = EINVAL;
	free(new_event);
	return NULL;
    }
    new_event->data[LDATA] = '\0';
    p = p + LDATA;
    /* converte il nome utente */
    SKIP_SPACES(p);
    new_event->utente[0] = '\0';
    for (i = 0; i < LUTENTE && *p && *p != '#'; i++, p++)
	new_event->utente[i] = *p;

    if (new_event->utente[0] == '\0')
	fprintf(stderr, "\n libdplan: convertiRecord(char []): WARNING empty user name from record \"%s\"", r);
    new_event->utente[i] = '\0';
    if (i == LUTENTE && *p && *p != '#') {
	fprintf(stderr,
		"\n libdplan: convertiRecord(char []): WARNING user name excedes maximum length %d from record \"%s\"",
		LUTENTE, r);
	while (*p && *p != '#')
	    p++;
	if (*p != '#') {
	    fprintf(stderr, "\n libdplan: convertiRecord(char []): ERROR missing \'#\' in record \"%s\"", r);
	    errno = EINVAL;
	    free(new_event);
	    return NULL;
	}
    }
    if (*p == '\0') {
	fprintf(stderr, "\n libplan: convertiRecord(char []): ERROR missing \'#\' in record \"%s\"", r);
	errno = EINVAL;
	free(new_event);
	return NULL;
    }
    /* converte la descrizione */
    p++;
    for (i = 0; i < LDESCRIZIONE && *p && *p != '\n'; i++, p++)
	new_event->descrizione[i] = *p;
    new_event->descrizione[i] = '\0';
    if (i == LDESCRIZIONE && *p != '\n' && *p)
	fprintf(stderr,
		"\n libdplan: convertiRecord(char []): WARNING description excedes maximum length %d in record \"%s\"",
		LDESCRIZIONE, r);
    /* elimina gli spazi finali */
    for (i--; i && isspace(new_event->descrizione[i]); i--)
	new_event->descrizione[i] = '\0';
    return new_event;
}

int convertiEvento(evento_t * e, char r[])
{
    /** lunghezza massima della descrizione degli eventi calcolata
        in base alle lunghezze massime della data del record e alla
        lunghezza del nome utente */
    int description_length;

    if (e == NULL || r == NULL)
	return -1;
    description_length = LRECORD - 2 - LDATA - strlen(e->utente);
    if (snprintf
	(r, LRECORD + 1, "%*.*s %.*s#%-*.*s", LDATA, LDATA, e->data,
	 LUTENTE, e->utente, description_length, description_length, e->descrizione) < LDATA + 2) {
	fprintf(stderr, "\n lbase: convertiEvento(evento_t *e, charr[]): evento non valido ");
	return -1;
    }
    return 0;
}

int matchData(char b[], evento_t * e)
{
    if (b == NULL || e == NULL)
	return -1;
    if (!checkDate(b) || !checkDate(e->data)) {
	fprintf(stderr, "\n lbase: matchData(%s, evento_t*[%s]): date not valid ", b, e->data);
	return -1;
    }
    if (b[0] != '*' && (b[0] != e->data[0] || b[1] != e->data[1]))
	return 0;
    return (strncmp(b + 2, e->data + 2, LDATA - 2) == 0) ? 1 : 0;
}

int matchPattern(char p[], evento_t * e)
{
    if (e == NULL || e->data == NULL || e->utente == NULL || e->descrizione == NULL)
	return -1;
    if (p == NULL || (strstr(e->data, p) != NULL)
	|| (strstr(e->utente, p) != NULL)
	|| (strstr(e->descrizione, p) != NULL)
	)
	return 1;
    return 0;
}
