/**
 * \file lcscom.c 
 * \author Federico Viscomi 412006
 * 
 * Si dichiara che il contenuto di questo file e' in ogni sua parte opera
 * originale dell' autore.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#define UNIX_PATH_MAX 108

#include "lcscom.h"
#include "error_check.h"

serverChannel_t createServerChannel(const char *path)
{
    /** il canale di ascolto */
    serverChannel_t server_channel;
    /** l'indirizzo da associare alla socket */
    struct sockaddr_un sa;

    if (strlen(path) > UNIX_PATH_MAX)
	return SOCKNAMETOOLONG;

    (void) unlink(path);
    strncpy(sa.sun_path, path, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    if ((server_channel = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
	perror("\n lcscom: createServerChannel(): ERROR in socket()");
	return -1;
    }
    if (bind(server_channel, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
	perror("\n lcscom: createServerChannel(): ERROR in bind()");
	(void) close(server_channel);
	return -1;
    }
    if (listen(server_channel, SOMAXCONN) == -1) {
	perror("\n lcscom: createServerChannel(): ERROR in listen()");
	(void) close(server_channel);
	return -1;
    }
    return server_channel;
}

int closeSocket(serverChannel_t s)
{
    if (close(s) == -1) {
	perror("\n lcscom: closeSocket: unable to close socket");
	return -1;
    }
    return 0;
}

channel_t acceptConnection(serverChannel_t s)
{
    /** il file descriptor del canale di comunicazione tra client e server */
    channel_t fd_client;

    if ((fd_client = accept(s, NULL, 0)) == -1) {
	perror("\n lcscom: acceptConnection: unable to accept connection ");
	return -1;
    }
    return fd_client;
}

int receiveMessage(channel_t sc, message_t * msg)
{
    /** numero di byte letti del buffer del messaggio */
    int read_bytes;

    if (read(sc, msg, sizeof(message_t)) == -1) {
	perror("\n lcscom: receiveMessage: unable to read message template ");
	return -1;
    }
    if ((msg->buffer = (char *) malloc(sizeof(char) * (msg->length + 1))) == NULL) {
	perror("\n lcscom: receiveMessage: memory error");
	return -1;
    }
    if ((read_bytes = read(sc, msg->buffer, msg->length)) == -1 || read_bytes != msg->length) {
	if (errno <= 0)
	    return SEOF;
	perror("\n lcscom: receiveMessage: unable to read message ");
	free(msg->buffer);
	return -1;
    }
    msg->buffer[msg->length] = '\0';
    return msg->length;
}

int sendMessage(channel_t sc, message_t * msg)
{
    /** numero di byte inviati del buffer del messagio */
    int sent_bytes;

    if (write(sc, msg, sizeof(message_t)) != sizeof(message_t)) {
	perror("\n lcscom: sendMessage: unable to send message template ");
	return -1;
    }
    if ((sent_bytes = write(sc, msg->buffer, msg->length)) == -1) {
	perror("\n lcscom: sendMessage: unable to send message ");
	return -1;
    }
    return sent_bytes;
}

int closeConnection(channel_t sc)
{
    if (close(sc) == -1) {
	perror("\n lcscom: closeConnection: unable to close connection ");
	return -1;
    }
    return 0;
}

channel_t openConnection(const char *path)
{
    /** il file descriptor del canale di comunicazione tra client e server */
    channel_t fd_skt;
    /** l'indirizzo della socket tramite la quale creare il canale */
    struct sockaddr_un sa;

    if (strlen(path) > UNIX_PATH_MAX)
	return SOCKNAMETOOLONG;

    if ((fd_skt = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
	perror("\n lcscom: openConnection(): socket(AF_UNIX, SOCK_STREAM, 0): ERROR in creating socket ");
	return -1;
    }

    strncpy(sa.sun_path, path, UNIX_PATH_MAX);
    sa.sun_family = AF_UNIX;
    errno = 0;
    while (connect(fd_skt, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
	if (errno == ENOENT) {
	    sleep(1);
	    errno = 0;
	} else {
	    perror("\n lcscom: openConnection: unable to connect ");
	    (void) close(fd_skt);
	    return -1;
	}
    }
    return fd_skt;
}
