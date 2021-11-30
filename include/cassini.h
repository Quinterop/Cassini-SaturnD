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
#include <pwd.h>

#include "client-request.h"
#include "server-reply.h"

struct string{
  uint32_t L;
  char *contenu;
};

struct commandline{
  uint32_t argc;
  struct string argv[];
};

#endif // CASSINI
