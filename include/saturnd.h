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


int send_reply_bool(int reptype, char *path_reply);

void get_stdout(char *tasks_directory, int fd_request, char * path_request, char * path_reply);

uint16_t remove_task(char * tasks_directory, int fd_request, char * path_request, char * path_reply);

int list_tasks();
int create_task(timing timing,commandline commandline);
int get_times_and_exitcodes(uint64_t taskid);
int  get_stderr(uint64_t taskid);

#endif // SATURND