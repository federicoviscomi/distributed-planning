/**
 * \file error_check.h
 * \author Federico Viscomi e lcs
 * \brief macro e funzioni per il controllo dell'errore
 * 
 * Si dichiara che il contenuto di questo file e' in ogni sua
 * parte opera degli autori originali
 */


#ifndef ASSERT_H_
#define ASSERT_H_

#include <stdarg.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

void ec_meno1(int n, char *error_message);

void ec_null(void *c, char *error_message);

void ec_nonzero(int n, char *error_message);

void *Malloc_(size_t size, char *error_message, bool exit_on_error);

void *Malloc(size_t size);

#endif				/* ASSERT_H_ */
