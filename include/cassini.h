#ifndef CASSINI_H
#define CASSINI_H

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <endian.h>
#include <inttypes.h>
#include <time.h>

#include "client-request.h"
#include "server-reply.h"
#include "timing-text-io.h"

typedef struct string{
    uint32_t L;
    char *contenu;
} string;

typedef struct commandline{
    uint32_t argc;
    string argv[];
} commandline;


//free les espaces memoire alloues et exit(EXIT_FAILURE)
int free_and_exit(char *path_request, char *path_reply);

//prend une operation et l'ecrit dans le pipe request
void write_op_to_pipe(uint16_t operation, char* path_request, char *path_reply);

//prend une operation et un taskid et les ecrit dans le pipe request
void write_op_taskid_to_pipe(uint16_t operation, uint64_t taskid, char* path_request, char *path_reply);

//ecriture pour la commande CREATE
void write_create(uint16_t operation, int argc, char * argv[], char * minutes_str, char * hours_str, char * daysofweek_str, char *path_request, char *path_reply);
  
//lecture pour les options e et o (reptype OK ou ERROR)
void read_reptype_e_o(char *path_reply, char *path_request);

//lecture pour la commande LIST
void read_list(char *path_reply, char *path_request);

//lecture pour la commande TIMES_EXITCODE
void read_times_exitcode(char *path_reply, char *path_request);

//renvoie le reptype lu
uint16_t read_reptype(int fd_reply, char *path_reply, char *path_request);

//renvoie l'errcode lu
uint16_t read_errcode(int fd_reply, char *path_reply, char *path_request);


#endif // CASSINI
