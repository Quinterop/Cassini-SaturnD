#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <stdint.h>
#include <fcntl.h>
#include <endian.h>
#include <inttypes.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <dirent.h>

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


char * init_path();

char * init_path_request(char *pipes_directory);

char * init_path_reply(char *pipes_directory);

int free_and_exit(char *path_request, char *path_reply);

#endif // COMMON