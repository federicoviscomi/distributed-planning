/**
 * \file dserver.c 
 * \author Federico Viscomi 412006
 * \brief il server
 * 
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.
 */

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <mcheck.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

#include "llist.h"
#include "lcscom.h"
#include "planners_table.h"
#include "pthread_mem.h"
#include "error_check.h"

#define USAGE "USAGE:\n\
dserver dir\n\
	avvia il server e carica le agende contenute in dir\n"

/* nome della directory che contiene le agende */
static char *planners_directory_name;

/*  
    se la directory esiste carica in memoria centrale le agende che contiene;
    altrimenti crea una nuova directory col nome dato 
*/
void loadPlanners()
{
    /* la directory che contiene le agende */
    DIR *dir;
    /* serve per leggere il contenuto della directory delle agende */
    struct dirent *entry;
    /* memorizza un agenda letta */
    elem_t *planner_list;
    /* il file dal quale leggere un agenda */
    FILE *planner_file;
    /* il nome di un agenda */
    char *planner_file_name;
    /* la lunghezza del nome della directory */
    int dir_len;

    dir_len = strlen(planners_directory_name);
    if ((dir = opendir(planners_directory_name)) == NULL) {
	/* la directory non esiste */
	if (errno == ENOENT) {
	    ec_meno1(mkdir(planners_directory_name, S_IRWXU), "\n dserver: load_planners: unable to create directory ");
	    return;
	}
	fprintf(stderr, "\n dserver: load_planners: unable to open directory ");
	exit(EXIT_FAILURE);
    }
    planner_file_name = Malloc(sizeof(char) * (LAGENDA + dir_len + 2));
    sprintf(planner_file_name, "%s/", planners_directory_name);
    initTable();
    for (errno = 0; (entry = readdir(dir)); errno = 0) {
	if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
	    sprintf(planner_file_name + dir_len + 1, "%s", entry->d_name);
	    ec_null(planner_file = fopen(planner_file_name, "r"), "\n dserver: load_planners: unable to open a file");
	    ec_meno1(loadAgenda(planner_file, &planner_list), "\n dserver: load_planners: unable to load a planner");
	    ec_nonzero(fclose(planner_file), "\n dserver: load_planners: ERROR: unable to close a file");
	    ec_meno1(plannersTableAdd(entry->d_name, planner_list),
		     "\ndserver: load_planners: ERROR: unable to add table entry for file %s");
	}
    }
    ec_nonzero(errno > 0, "\n dserver: load_planners: ERROR: while reading planners' directory");
    ec_meno1(closedir(dir), "\n dserver: load_planners: ERROR while closing planners' directory");
    free(planner_file_name);
}

/* conta il numero di readers in esecuzione */
static int readers_count = 0;
/* mutua esclusione su tutte le agende */
static pthread_mutex_t planners_mutex = PTHREAD_MUTEX_INITIALIZER;
/* mutua esclusione su count */
static pthread_mutex_t readers_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static void readerEnterCriticalRegion()
{
    pthread_mutex_lock(&readers_count_mutex);
    if (readers_count == 0)
	pthread_mutex_lock(&planners_mutex);
    readers_count++;
    pthread_mutex_unlock(&readers_count_mutex);
}
static void readerLeaveCriticalRegion()
{
    pthread_mutex_lock(&readers_count_mutex);
    readers_count--;
    if (readers_count == 0)
	pthread_mutex_unlock(&planners_mutex);
    pthread_mutex_unlock(&readers_count_mutex);
}

static void writerEnterCriticalRegion()
{
    pthread_mutex_lock(&planners_mutex);
}
static void writerLeaveCriticalRegion()
{
    pthread_mutex_unlock(&planners_mutex);
}

/*
    riceve una richiesta da un client e la serve.
    \param arg il puntatore alla locazione che contiene
	   il file descriptor del canale di comunicazione
	   col client
*/
void *worker(void *arg)
{
    /* il canale di comunicazione tra il server e il client  */
    channel_t client_ch = 0;
    /* il messaggio di richiesta da ricevere dal client */
    message_t request;
    /* il messaggio di risposta da inviare al client */
    message_t answer;
    /* l'eventuale agenda relativa alla richiesta del client */
    elem_t **planner = NULL;
    /* l'eventuale lista di eventi trovati nel caso di richieste
       di ricerca tipo MSG_EMESE MSG_EGIORNO */
    elem_t *items_found = NULL;
    /* la data usata nella ricerca  */
    char data[LDATA] = { 0 };
    /* server per bloccare SIGPIPE */
    sigset_t blockSigPipe;


    client_ch = *((int *) arg);
    free(arg);
    /* blocca SIGPIPE nel thread */
    ec_meno1(sigemptyset(&blockSigPipe), "\n dserer: ERROR: sigemptyset()");
    ec_meno1(sigaddset(&blockSigPipe, SIGPIPE), "\n dserer: ERROR: sigaddset(PIPE)");
    if (pthread_sigmask(SIG_BLOCK, &blockSigPipe, NULL)) {
	perror("\n dserver: worker: ERROR in pthread_sigmask");
	setThreadNotActive();
	(void) closeConnection(client_ch);
	return NULL;
    }

    /* riceve la richiesta dal client */
    if (receiveMessage(client_ch, &request) <= 0) {
	fprintf(stderr,
		"\n dserver: void *worker(void *arg) thread worker %d: unable to receive message from client ",
		client_ch);
	setThreadNotActive();
	(void) closeConnection(client_ch);
	return NULL;
    }

    /* acquisisce la mutua esclusione */
    if (request.type == MSG_EGIORNO || request.type == MSG_EMESE)
	readerEnterCriticalRegion();
    else
	writerEnterCriticalRegion();
    /* calcola la risposta da dare al client */
    if ((planner = plannerTableGet(request.buffer)) == NULL && request.type != MSG_MKAGENDA) {
	answer.type = MSG_ERROR;
	answer.buffer = "Agenda not existent";
    } else {
	switch (request.type) {
	case MSG_MKAGENDA:
	case MSG_RMAGENDA:{
		/* il nome dell'agenda relativa alla richiesta del client
		   preceduto dal path della directory delle agende per come e' stato
		   specificato da riga di comando. quindi non necessariamente
		   il percorso assoluto */
		char *fully_qualified_planner_name = NULL;
		if ((fully_qualified_planner_name =
		     malloc(sizeof(char) * (strlen(planners_directory_name) + LAGENDA + 2))) == NULL) {
		    perror("\n dserver: worker: memory error");
		    setThreadNotActive();
		    (void) closeConnection(client_ch);
		    return NULL;
		}
		sprintf(fully_qualified_planner_name, "%s/", planners_directory_name);
		sprintf(fully_qualified_planner_name + strlen(planners_directory_name) + 1, "%s", request.buffer);
		if (request.type == MSG_MKAGENDA) {
		    int new_fd;
		    /* buffer=nome dell’agenda */
		    if ((new_fd = open(fully_qualified_planner_name, O_EXCL | O_CREAT, S_IRWXU)) == -1) {
			answer.type = MSG_ERROR;
			if (errno == EEXIST)
			    answer.buffer = "Cannot create, agenda already present";
			else
			    answer.buffer = "Cannot create agenda";
		    } else {
			(void) close(new_fd);
			if (plannersTableAdd(request.buffer, NULL)) {
			    answer.type = MSG_ERROR;
			    answer.buffer = "Cannot create agenda";
			    (void) unlink(fully_qualified_planner_name);
			} else {
			    answer.type = MSG_OK;
			    answer.buffer = "Created";
			}
		    }
		} else if (request.type == MSG_RMAGENDA) {
		    /* buffer=nome  dell’agenda. */
		    if (*planner != NULL) {
			answer.type = MSG_ERROR;
			answer.buffer = "Agenda not empty, cannot remove";
		    } else {
			if (plannerTableRemove(request.buffer)
			    || remove(fully_qualified_planner_name) == -1) {
			    answer.type = MSG_ERROR;
			    answer.buffer = "\n dserver: ERROR: unable to remove planner file";
			} else {
			    answer.buffer = "Removed";
			    answer.type = MSG_OK;
			}
		    }
		}
		free(fully_qualified_planner_name);
		break;
	    }
	case MSG_INSERT:{
		evento_t *ev;
		int i, j;
		char *p;
		/* buffer=agenda\0data\0utente\0descrizione\0 */
		if ((ev = malloc(sizeof(evento_t))) == NULL) {
		    perror("\n dserver: thread worker: MEMORY ERROR");
		    setThreadNotActive();
		    (void) closeConnection(client_ch);
		    free(request.buffer);
		    writerLeaveCriticalRegion();
		    return NULL;
		}
		answer.type = MSG_OK;
		p = request.buffer + strlen(request.buffer) + 1;
		for (i = 0; p[i]; i++)
		    ev->data[i] = p[i];
		ev->data[i] = '\0';
		for (i++, j = 0; p[i]; i++, j++)
		    ev->utente[j] = p[i];
		ev->utente[j] = '\0';
		for (i++, j = 0; p[i]; i++, j++)
		    ev->descrizione[j] = p[i];
		ev->descrizione[j] = '\0';
		if (add(planner, ev)) {
		    answer.type = MSG_ERROR;
		    answer.buffer = " Error unable to add event to list ";
		} else {
		    answer.buffer = "Success ";
		    plannerTableSetModified(request.buffer);
		}
		break;
	    }
	case MSG_RMPATTERN:{
		/* buffer=agenda\0pattern\0 */
		*planner = rimuovi(request.buffer + strlen(request.buffer) + 1, *planner);
		answer.type = MSG_OK;
		answer.buffer = "Success ";
		plannerTableSetModified(request.buffer);
		break;
	    }
	case MSG_EGIORNO:
	case MSG_EMESE:{
		if (request.type == MSG_EGIORNO) {
		    strncpy(data, request.buffer + strlen(request.buffer) + 1, LDATA);
		} else {
		    int i;
		    data[0] = data[1] = '*';
		    data[2] = '-';
		    request.buffer = request.buffer + strlen(request.buffer) + 1;
		    for (i = 0; request.buffer[i]; i++)
			data[i + 3] = request.buffer[i];
		}
		if (cerca(data, *planner, &items_found) == -1) {
		    perror("\n dserver: worker: error in cerca");
		    setThreadNotActive();
		    (void) closeConnection(client_ch);
		    free(request.buffer);
		    readerLeaveCriticalRegion();
		    return NULL;
		}
		if (items_found == NULL) {
		    answer.buffer = "No events registered";
		    answer.type = MSG_ERROR;
		} else {
		    answer.type = MSG_OK;
		    answer.buffer = "Success ";
		}
		break;
	    }
	}
    }

    /* rilascia la mutua esclusione */
    if (request.type == MSG_EGIORNO || request.type == MSG_EMESE)
	readerLeaveCriticalRegion();
    else
	writerLeaveCriticalRegion();

    /* invia le/a risposte/a al client */
    if (answer.type == MSG_ERROR || (request.type != MSG_EMESE && request.type != MSG_EGIORNO)) {
	answer.length = strlen(answer.buffer);
	if (sendMessage(client_ch, &answer) == -1) {
	    perror("\n dserver: worker: unable to send message to client");
	    setThreadNotActive();
	    (void) closeConnection(client_ch);
	    return NULL;
	}
    } else {
	elem_t *list;
	char record[LRECORD];

	list = items_found;
	for (list = items_found; list; list = list->next) {
	    if (convertiEvento(list->ptev, record) == -1) {
		perror("\n dserver: void *worker(void *arg): ERROR: convertiEvento((*trovati)->ptev, record)");
		setThreadNotActive();
		(void) closeConnection(client_ch);
		return NULL;
	    }
	    answer.buffer = record;
	    answer.length = strlen(answer.buffer);
	    if (sendMessage(client_ch, &answer) == -1) {
		perror("\n dserver: void *worker(void *arg): ERROR: sendMessage(client_ch, &msg)");
		setThreadNotActive();
		(void) closeConnection(client_ch);
		return NULL;
	    }
	}
	dealloca_lista(items_found);
    }
    setThreadNotActive();
    closeConnection(client_ch);
    return 0;
}

/* il thread dispatcher esegue questa funzione che
   in un ciclo infinito:
   	1. accetta una connessione da parte di un client
	2. crea un thread worker che si occupa del client trovato
  \param listen il canale sul quale mettersi in ascolto per accettare
                connessioni da parte dei client   
*/
void *dispatcher(void *listen)
{
    while (true) {
	channel_t next_client;
	if ((next_client = acceptConnection(*((serverChannel_t *) listen))) != -1) {
	    if (submitNewTask(next_client, worker)) {
		fprintf(stderr,
			"\n dserver: main ERROR: cannot create thread worker on client %d. closing connection ",
			next_client);
		(void) closeConnection(next_client);
	    }
	}
    }
}

int main(int argc, char *argv[])
{
    /* il canale sul quale il server si mettera' in ascolto
       per accettare connessioni da parte dei client */
    serverChannel_t listen;
    /* la bit-map usata per bloccare SIGTERM */
    sigset_t sigTermOnly;
    /* il pthread_t del thread dispatcher */
    pthread_t dispatcher_tid;
    /* il segnale che riceve la sigwait */
    int signum;
    /* il nome di un agenda relativo alla 
       directory della agende */
    char *rel_planner_name = NULL;
    /* il nome di un agenda preceduto dal
       percorso della directory delle agende */
    char *abs_planner_name = NULL;
    /* gli elementi di un agenda */
    elem_t *planner_list = NULL;
    /* un file di un agenda */
    FILE *planner_file = NULL;
    /* usato per ignorare SIGPIPE e SIGQUIT */
    struct sigaction sa;

    mtrace();

    /* controllo degli argomenti */
    if (argc != 2) {
	fprintf(stderr, "%s", USAGE);
	return EXIT_FAILURE;
    }

    /* ignora SIGQUIT e SIGPIPE */
    memset(&sa, sizeof(sa), 0);
    sa.sa_handler = SIG_IGN;
    ec_meno1(sigaction(SIGQUIT, &sa, NULL), NULL);
    ec_meno1(sigaction(SIGPIPE, &sa, NULL), NULL);

    /* blocca SIGTERM nella maschera del thread */
    ec_meno1(sigemptyset(&sigTermOnly), "\n dserer: ERROR: sigemptyset(&set)");
    ec_meno1(sigaddset(&sigTermOnly, SIGTERM), "\n dserer: ERROR: sigaddset(&sigTermPipe, SIGTERM)");
    if (pthread_sigmask(SIG_BLOCK, &sigTermOnly, NULL)) {
	fprintf(stderr, "\n dserver: main pthread_sigmask");
	return EXIT_FAILURE;
    }

    /* carica le agende o crea la directory delle agende */
    planners_directory_name = argv[1];
    while (isspace(*planners_directory_name))
	planners_directory_name++;
    if (*planners_directory_name == '\0') {
	fprintf(stderr, "\n dserver: directory name is empty");
	return EXIT_FAILURE;
    }
    loadPlanners();

    /* crea il canale di connessione */
    if (mkdir("./tmp", S_IRWXU) == -1 && errno != EEXIST) {
	fprintf(stderr, "\n dserver: main: unable to create socket directory ");
	return EXIT_FAILURE;
    }
    if ((listen = createServerChannel(SOCKET_PATH)) <= 0) {
	fprintf(stderr, "\n dserver: main ERROR: unable to createServerChannel(%s)", SOCKET_PATH);
	return EXIT_FAILURE;
    }

    abs_planner_name = Malloc(sizeof(char) * (strlen(planners_directory_name) + LAGENDA + 2));

    /* crea il thread dispatcher */
    if (pthread_create(&dispatcher_tid, NULL, dispatcher, (void *) (&listen))) {
	fprintf(stderr, "\n dserver: main ERROR: unable to create dispatcher thread");
	return EXIT_FAILURE;
    }

    /* si mette in attesa di un SIGTERM */
    if (sigwait(&sigTermOnly, &signum)) {
	perror("\n dserver: main: sigwait");
	return EXIT_FAILURE;
    }

    /* il SIGTERM e' stato catturato. inizia la procedura di terminazione
       del server  */
    fprintf(stderr, "\n dserver: SIGTERM cathced. server is halting ... ");
    sprintf(abs_planner_name, "%s/", planners_directory_name);
    waitForThreadTermination();
    (void) close(listen);
    (void) unlink(SOCKET_PATH);
    /* salva nel file system le agende che sono state modificate in memoria centrale */
    while (plannerTableNext(&planner_list, &rel_planner_name)) {
	sprintf(abs_planner_name + strlen(planners_directory_name) + 1, "%s", rel_planner_name);
	if ((planner_file = fopen(abs_planner_name, "w")) == NULL) {
	    fprintf(stderr, "\n dserver: ERROR: unable to open file %s: %s\n", abs_planner_name, strerror(errno));
	} else if (storeAgenda(planner_file, planner_list) == -1) {
	    fprintf(stderr, "\n dserver: ERROR: unable to store file %s: %s\n", abs_planner_name, strerror(errno));
	} else if (fclose(planner_file)) {
	    fprintf(stderr,
		    "\n dserver: thread waiter: ERROR: unable to close file %s: %s\n",
		    abs_planner_name, strerror(errno));
	} else
	    fprintf(stderr, "\n dserver: stored file %s", abs_planner_name);
    }
    fprintf(stderr, "\n SERVER HALTED \n");
    exit(EXIT_SUCCESS);
}
