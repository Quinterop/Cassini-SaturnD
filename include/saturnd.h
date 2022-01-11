#ifndef SATURND_H
#define SATURND_H

#include "common.h"

typedef struct run {
    uint16_t exit_code;
    uint64_t time;
    run *next;
} run;

typedef struct task {
    uint64_t taskid;
    struct timing *timing;
    commandline *cmd;
    run *runs;
    string last_stdout;
    string last_stderr;
    task *next;
} task;


void get_stdout(char *tasks_directory, int fd_request, char * path_request, char * path_reply);

#endif // SATURND