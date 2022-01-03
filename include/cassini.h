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


//free les espaces memoire alloues et return EXIT_FAILURE
int free_and_exit(char *path_request, char *path_reply);

//prend une operation et l'ecrit dans le pipe request, renvoie 0 si erreur, 1 sinon
void write_op_to_pipe(uint16_t operation, char* path_request, char *path_reply);

//prend une operation et un taskid et les ecrit dans le pipe request, renvoie 0 si erreur, 1 sinon
void write_op_taskid_to_pipe(uint16_t operation, uint64_t taskid, char* path_request, char *path_reply);

//lecture pour les options e et o (reptype OK ou ERROR), renvoie 0 si errue, 1 sinon
int read_reptype_e_o(char *path_reply, char *path_request);

//renvoie le reptype lu, ou -1
uint16_t read_reptype(int fd_reply, char *path_reply, char *path_request);

//renvoie l'errcode lu, ou -1
uint16_t read_errcode(int fd_reply, char *path_reply, char *path_request);


#endif // CASSINI
