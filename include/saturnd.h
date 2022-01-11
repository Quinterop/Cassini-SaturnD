#ifndef SATURND_H
#define SATURND_H

#include "common.h"

typedef struct run {
    string stdout;
    struct timing *timing;
    run *next;
} run;

typedef struct task {
    int id;
    struct timing *timing;
    commandline *cmd;
    run *runs;
    string last_stdout;
    string last_stderr;
    task *next;
} task;

#endif // SATURND