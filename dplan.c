/**
 * \file dplan.c
 * \author Federico Viscomi 412006
 * \brief il client 
 * 
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <mcheck.h>
#include <stdbool.h>


#include "lbase.h"
#include "lcscom.h"
#include "error_check.h"

#define USAGE1 "\
	dplan -c aname\n\
		-> crea una nuova agenda di nome ''aname''\n\
	dplan -q aname\n\
		-> rimuove una agenda di nome ''aname'' (solo se vuota)\n\
	dplan agenda -d gg-mm-aaaa -u utente#descrizione\n\
		-> inserisce in ''agenda'' un nuovo evento\n\
		-> -d specifica la data\n\
		-> -u specifica l'utente che sta effettuando la registrazione e la descrizione dell'evento\n\
	dplan agenda -g gg-mm-aaaa\n\
		-> richiede gli eventi registrati per un certo giorno su ''agenda'' e li stampa sullo stdout\n"
#define USAGE2 "\
	dplan agenda -m mm-aaaa\n\
		-> richiede gli eventi registrati su ''agenda'' per un certo mese e li stampa sullo stdout\n\
	dplan agenda -r pattern\n\
		-> elimina tutti gli eventi di ''agenda'' che contengono ''pattern'' come sottostringa in un qualsiasi campo\n\
	dplan\n\
		-> stampa questo messaggio\n"

/* le opzioni possibili */
#define OPT_STRING ":l:q:d:g:m:r:u:c:"

/**
  legge gli argomenti passati da riga di comando e memorizza i campi significativi 
  nelle locazioni puntate dagli argomenti della funzione
  \param argc il numero di argomenti da riga di comando
  \param argv l'array degli argomenti da riga di comando
  \param optarg1 la locazione nella quale memorizzare l'argomento della prima opzione
  \param optarg2 la locazione nella quale memorizzare l'argomento della seconda opzione
  \param agenda la locazione nella quale memorizzare il nome dell'agenda
  \return un codice che identifica il tipo di richiesta
*/
int fetch(int argc, char *argv[], char **optarg1, char **optarg2, char **agenda)
{
    /* serve per leggere le opzioni */
    int opt;
    /* il codice che identifica il tipo di richiesta */
    int case_;

    if (argc == 1 || (argc != 2 && argc != 3 && argc != 4 && argc != 6)) {
	printf("%s%s", USAGE1, USAGE2);
	exit(EXIT_SUCCESS);
    }
    opt = getopt(argc, argv, OPT_STRING);
    switch (opt) {
    case -1:
	printf("dplan: Formato scorretto del comando\n");
	exit(EXIT_FAILURE);
    case '?':
	printf("dplan: -%c: Opzione errata\n", optopt);
	exit(EXIT_FAILURE);
    case ':':
	if ((optopt == 'c' || optopt == 'q') && argc == 3) {
	    opt = optopt;
	    break;
	}
	if (optopt == 'r' || optopt == 'd' || optopt == 'u' || optopt == 'm')
	    printf("dplan: -%c: Opzione errata\n", optopt);
	else
	    printf("dplan: Formato scorretto del comando\n");
	exit(EXIT_FAILURE);
    }
    if (opt == 'u') {
	*optarg2 = argv[optind - 1];
	case_ = 'D';
    } else {
	*optarg1 = argv[optind - 1];
	case_ = toupper(opt);
    }

    if (opt == 'd' || opt == 'u') {
	if ((opt = getopt(argc, argv, "d:u:")) == -1 || opt == '?' || opt == ':') {
	    printf("dplan: Formato scorretto del comando\n");
	    exit(EXIT_FAILURE);
	}
	if (opt == 'd')
	    *optarg1 = argv[optind - 1];
	else
	    *optarg2 = argv[optind - 1];
    }

    if (getopt(argc, argv, OPT_STRING) != -1) {
	printf("dplan: Formato scorretto del comando\n");
	exit(EXIT_FAILURE);
    }
    if (opt != 'l' && optarg1 == NULL && optind >= argc) {
	printf("dplan: Formato scorretto del comando\n");
	exit(EXIT_FAILURE);
    }
    if (opt == 'c' || opt == 'q')
	*optarg1 = argv[argc - 1];
    else
	*agenda = argv[argc - 1];

    if (argc <= 3 && case_ == MSG_EGIORNO) {
	printf("dplan: Formato scorretto del comando\n");
	exit(EXIT_FAILURE);
    }
    return case_;
}

/**
   cerca di connetersi con un eventuale server in ascolto sulla socket
   di path SOCKET_PATH. effettua 5 tentativi di connessione a distanza di un secondo 
   l'uno dall'altro. in caso di fallimento termina il client
   \return il file descriptor del canale di connessione con il server
*/
int connectToServer()
{
    /* il file descriptor del canale */
    int server_fd;

    int i;

    for (i = 0; i < 5; i++) {
	if ((server_fd = openConnection(SOCKET_PATH)) > 0)
	    return server_fd;
	else
	    fprintf(stderr, "\n dplan: unable to openConnection(%s) on attemp %d", SOCKET_PATH, i + 1);
	(void) sleep(1);
    }
    fprintf(stderr, "\n dplan: unable to connect to the server ");
    exit(EXIT_FAILURE);
}

/* prepara il messaggio di richiesta da inviare al client 
   \param request il messaggio di richiesta
   \param agenda l'eventuale nome dell'agenda interessata nella richiesta 
   \param optarg1 l'argomento dell'eventuale prima opzione della richiesta
   \param optarg2 l'argomento dell'eventuale seconda opzione della richiesta
*/
void prepareRequestMessage(message_t * request, char *agenda, char *optarg1, char *optarg2)
{
    switch (request->type) {
    case MSG_MKAGENDA:
    case MSG_RMAGENDA:
	{
	    request->buffer = optarg1;
	    request->length = strlen(request->buffer);
	    break;
	}
    case MSG_RMPATTERN:
    case MSG_EGIORNO:
    case MSG_EMESE:
    case MSG_INSERT:
	{
	    char *buffer;
	    int i, j;
	    if (optarg2 == NULL)
		buffer = Malloc(sizeof(char) * (strlen(agenda) + strlen(optarg1) + 2));
	    else
		buffer = Malloc(sizeof(char) * (strlen(agenda) + strlen(optarg1) + strlen(optarg2) + 3));

	    for (i = 0; agenda[i]; i++)
		buffer[i] = agenda[i];
	    buffer[i] = '\0';
	    for (j = 0, i++; optarg1[j]; i++, j++)
		buffer[i] = optarg1[j];
	    buffer[i] = '\0';
	    if (optarg2 != NULL) {
		for (j = 0, i++; optarg2[j] != '#'; i++, j++)
		    buffer[i] = optarg2[j];
		buffer[i] = '\0';
		for (i++, j++; optarg2[j]; i++, j++)
		    buffer[i] = optarg2[j];
		buffer[i] = '\0';
	    }
	    request->buffer = buffer;
	    request->length = i;
	    break;
	}
    }

}

/** controlla che la richiesta sia corretta e si conformi al
    protocollo di comunicazione tra client e server
    \param option il codice della richiesta
    \param planner l'eventuale nome dell'agenda interessata nella richiesta
    \param optarg1 l'argomento dell'eventuale prima opzione della richiesta
    \param optarg2 l'argoemnto dell'eventuale seconda opzione della richiesta
    \return true in casi di fallimento; false altrimenti.
*/
bool checkArgs(int code, char *planner, char *optarg1, char *optarg2)
{
    while (isspace(*optarg1))
	optarg1++;
    if (code == MSG_MKAGENDA || code == MSG_RMAGENDA) {
	if (strlen(optarg1) > LAGENDA) {
	    printf("dplan: %s: Agenda max %d caratteri\n", optarg1, LAGENDA);
	    return false;
	}
	return true;
    }
    if (code == MSG_RMPATTERN)
	return true;
    if (strlen(planner) > LAGENDA) {
	printf("dplan: %s:  Formato evento non corretto (Agenda max %d caratteri) \n", planner, LAGENDA);
	return false;
    }
    switch (code) {
    case MSG_INSERT:
	{
	    char *hash_sign;

	    /* dplan agenda -d gg-mm-aaaa -u utente#descrizione -> inserisce in ‘‘agenda’’ un nuovo evento */
	    if (planner == NULL || optarg1 == NULL || optarg2 == NULL)
		return false;
	    while (isspace(*optarg2))
		optarg2++;
	    if ((hash_sign = strchr(optarg2, '#')) == NULL) {
		printf("dplan: %s: Formato evento non corretto (manca #)\n", optarg2);
		return false;
	    }
	    if (hash_sign - optarg2 > LUTENTE) {
		*hash_sign = '\0';
		printf("dplan: %s: Formato evento non corretto (Utente max %d caratteri)\n", optarg2, LUTENTE);
		return false;
	    }
	    hash_sign++;
	    if (strlen(hash_sign) > LDESCRIZIONE) {
		printf
		    ("dplan: %s: Formato evento non corretto (Descrizione max %d caratteri)\n", optarg2, LDESCRIZIONE);
		return false;
	    }
	}
    case MSG_EGIORNO:
	{
	    /* dplan agenda -g gg-mm-aaaa -> richiede gli eventi registrati per un certo giorno su ‘‘agenda’’ e li stampa sullo stdout */
	    if (planner == NULL || optarg1 == NULL)
		return false;
	    if (!checkDate(optarg1)) {
		printf("dplan: %s: Data non corretta\n", optarg1);
		return false;
	    }
	    return true;
	}
    case MSG_EMESE:
	{
	    /* dplan agenda -m mm-aaaa -> richiede gli eventi registrati su ‘‘agenda’’ per un certo mese e li stampa sullo stdout */
	    if (planner == NULL || optarg1 == NULL)
		return false;
	    if (strlen(optarg1) != LDATA - 3 || !isdigit(optarg1[0])
		|| !isdigit(optarg1[1]) || optarg1[2] != '-' || !isdigit(optarg1[3]) || !isdigit(optarg1[4])
		|| !isdigit(optarg1[5]) || !isdigit(optarg1[6])
		) {
		printf("dplan: %s Data non corretta\n", optarg1);
		return false;
	    }
	    return true;
	}
    }
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    /* l'argomento dell'eventuale prima opzione della richiesta */
    char *optarg1 = NULL;
    /* l'argomento dell'eventuale seconda opzione della richiesta */
    char *optarg2 = NULL;
    /* il nome dell'eventuale agenda interessata nella richiesta */
    char *agenda = NULL;
    /* il file descriptor del canale di comunicazione col server  */
    int server_fd = 0;
    /* il messaggio usato sia per inviare la richiesta che per
       ricevere la risposta */
    message_t msg;

    int i;

    mtrace();

    /* parsing degli argomenti */
    msg.type = fetch(argc, argv, &optarg1, &optarg2, &agenda);

    /* controllo della correttezza degli argomenti */
    if (checkArgs(msg.type, agenda, optarg1, optarg2) == false)
	return EXIT_FAILURE;

    /* preparazione del messaggio di richiesta da inviare al server */
    prepareRequestMessage(&msg, agenda, optarg1, optarg2);

    /* connessione al server */
    server_fd = connectToServer(SOCKET_PATH);

    /* invio messaggio di richiesta al server */
    if (sendMessage(server_fd, &msg) == -1) {
	(void) closeConnection(server_fd);
	fprintf(stderr, "\n dplan: unable to send message to server ");
	return EXIT_FAILURE;
    }

    /* ricezione messaggi di risposta dal server */
    if (msg.type != MSG_MKAGENDA && msg.type != MSG_RMAGENDA)
	free(msg.buffer);
    if (msg.type == MSG_EGIORNO || msg.type == MSG_EMESE) {
	int ret;
	while ((ret = receiveMessage(server_fd, &msg)) > 0) {
	    /* elmina gli spazi finali per far funzionare il test */
	    for (i = strlen(msg.buffer) - 1; i > 0 && isspace(msg.buffer[i]); i--);
	    msg.buffer[i + 1] = '\0';
	    printf("dplan: %s: %s\n", agenda, msg.buffer);
	    free(msg.buffer);
	}
	if (ret == -1) {
	    fprintf(stderr, "\n dplan: ERROR in receiving message from server ");
	    return EXIT_FAILURE;
	}
    } else {
	int type = msg.type;
	if (receiveMessage(server_fd, &msg) == -1) {
	    fprintf(stderr, "\n dplan: ERROR in receiving message from server ");
	    (void) closeConnection(server_fd);
	    return EXIT_FAILURE;
	}
	if (type == MSG_RMPATTERN && msg.type == MSG_ERROR)
	    printf("dplan: %s: %s\n", agenda, msg.buffer);
	else
	    printf("dplan: %s: %s\n", optarg1, msg.buffer);
    }

    /* cleanup */
    free(msg.buffer);
    if (closeConnection(server_fd) == -1) {
	fprintf(stderr, "\n dplan: ERROR in closing connection ");
	return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
