/**
 * \file error_check.c 
 * \author Federico Viscomi 412006
 * \brief raccolta di funzioni per il controllo dell'errore
 *
 * Si dichiara che il contenuto di questo file e' in ogni sua parte 
 * opera originale dell'autore
 */

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

#include "error_check.h"

void *Malloc(size_t size)
{
    void *tmp;
    if ((tmp = malloc(size)) == NULL) {
	perror("\n malloc");
	exit(EXIT_FAILURE);
    }
    return tmp;
}

void ec_meno1(int n, char *error_message)
{
    if (n == -1) {
	perror(error_message);
	exit(errno);
    }
}

void ec_nonzero(int n, char *error_message)
{
    if (n != 0) {
	perror(error_message);
	exit(errno);
    }
}

void ec_null(void *p, char *error_message)
{
    if (p == NULL) {
	perror(error_message);
	exit(errno);
    }
}
