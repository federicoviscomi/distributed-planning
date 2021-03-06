/**
 * \file pthread_mem.c 
 * \author Federico Viscomi 412006
 * \brief gestore dei thread worker
 * 
 * Gestiste la creazione/terminazione dei thread worker.
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.
 */

#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include "pthread_mem.h"

/** il mutex relativo alla condition variable cond */
static pthread_mutex_t cond_m = PTHREAD_MUTEX_INITIALIZER;
/** viene segnalata quando il numero di thread attivi e' zero
    e serve per attendere la terminazione dei thread creati */
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
/** conta il numero di thread creati e non ancora terminati */
static int count = 0;
/** flag che indica se il server si trova nella fase di 
    terminazione o meno. */
static bool shootDown = false;

/** usata per memorizzare i pthread_t dei thread da create 
    invece di allocare ogni volta una nuova area di memoria
    perche' complicherebbe la terminazione dei thread in quanto
    si dovrebbe gestire la deallocazione */
pthread_t thread_tid[SOMAXCONN] = { 0 };

/** questo metodo viene chiamato solo dal thread dispatcher del server.
    se il server si trova in fase di terminazione allora il metodo causa 
    la terminazione del thread chiamante. altrimenti crea un nuovo thread
    che esegue task ed ha come argomento un puntatore ad un'area di memoria
    appositamente allocata che contiene il valore di sc
    \param sc il canale di comunicazione del worker e del client
    \param task la funzione da far eseguire al nuovo thread
    \return true nel caso un cui non si riesca a creare un nuovo thred;
	    false altrimenti
    */
bool submitNewTask(channel_t sc, void *(*task) (void *))
{
    /** indice nella tabella hash */
    int index;
    /** argomento da passare al nuovo thread*/
    int *arg;

    pthread_mutex_lock(&cond_m);
    if (shootDown) {
	pthread_mutex_unlock(&cond_m);
	pthread_exit(EXIT_SUCCESS);
    }
    count++;
    pthread_mutex_unlock(&cond_m);

    /** il sistema operativo garantisce che sc e' unico per ogni
        sessione di comunicazione attiva e quindi per ogni worker
        inoltre il numero massimo di connessioni e' SOMAXCONN 
        quindi possiamo usare sc per indicizzare l'array dei pthread_t*/
    index = sc % SOMAXCONN;
    if ((arg = (int *) malloc(sizeof(int))) == NULL) {
	perror("malloc");
	return true;
    }
    *arg = sc;

    if (pthread_create(thread_tid + index, NULL, task, arg)) {
	perror("\n pthread_mem: submit_new_task: ERROR unable to create a new thread");
	return true;
    }

    return false;
}

/** se ci sono worker in esecuzione ne attende la terminazione 
    con una wait sulla condition variable */
void waitForThreadTermination()
{
    pthread_mutex_lock(&cond_m);
    shootDown = true;
    while (count != 0)
	pthread_cond_wait(&cond, &cond_m);
    pthread_mutex_unlock(&cond_m);
}

/** decrementa il numero di thread in esecuzione ed eventualmente 
    segnala la loro assenza sulla condition variable */
void setThreadNotActive()
{
    pthread_mutex_lock(&cond_m);
    count--;
    if (count == 0)
	pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&cond_m);
}
