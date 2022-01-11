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

int list_tasks();
int create_task(timing timing,commandline commandline);
int remove_task(uint64_t taskid);
int get_times_and_exitcodes(uint64_t taskid);
int  get_stdout(uint64_t taskid);
int  get_stderr(uint64_t taskid);
#endif // SATURND