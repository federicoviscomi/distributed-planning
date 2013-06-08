/**
 * \file pthread_mem.h 
 * \author Federico Viscomi
 * \brief gestisce i thread worker 
 * 
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.
 */

#include "lcscom.h"

/** */
bool submitNewTask(channel_t sc, void *(*task) (void *));

void waitForThreadTermination();

void setThreadNotActive();
