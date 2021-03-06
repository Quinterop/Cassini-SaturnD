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
#include <sys/types.h>
#include <pwd.h>
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
}commandline;

#endif // CASSINI
