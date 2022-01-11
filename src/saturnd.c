#include "../include/cassini.h"



void create_daemon() {
    int pid = fork();
    if (pid == -1) { exit(EXIT_FAILURE); }
    else if (pid != 0) { exit(EXIT_SUCCESS); } 
    else { 
        pid = fork();
        if (pid == -1) { exit(EXIT_FAILURE); }
        else if (pid != 0) { exit(EXIT_SUCCESS); }
        else {
            if (setsid() < 0) { exit(EXIT_FAILURE); }
            printf("PID : %d Ce terminal peut etre fermé\n", getpid());
            printf("PPID : %d Ce terminal peut etre fermé\n", getppid());
            printf("demon cree !");
        }
    }
}




/*
int send_reply_bool(int reptype){//0 pour ok et autre pour erreur
    int fd_reply = open(path_reply,O_WRONLY);
    if (fd_reply == -1) return 1;//todo
    uint16_t response;
    if (reptype == 0) {
        response = htobe16(SERVER_REPLY_OK);
    } else { response = htobe16(SERVER_REPLY_ERROR); }
    int errW = write(fd_reply, &response, sizeof(uint16_t));
    if (errW == -1) return 1;//todo
    close(fd_reply);
    return 0;
}
 */

/*
void read_from_pipes() {
    while (1) { //boucle nécéssaire ?
        int fd_request = open(path_request, O_RDONLY); //ou mettre le close ?
        //test err
        uint16_t request;
        int rd = read(fd_request, &request, sizeof(uint16_t));
        //test err
        request = be16toh(request);
        switch (request) {
            case CLIENT_REQUEST_TERMINATE:
                send_reply_bool(1);
                goto exit_loop;
            case CLIENT_REQUEST_LIST_TASKS:
                list_tasks();
                break;
            case CLIENT_REQUEST_CREATE_TASK:
                create_task(timing,commandline);
                break;
            case CLIENT_REQUEST_REMOVE_TASK:
                remove_task(taskid);
                break;
            case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES:
                get_times_and_exitcodes(taskid);
                break;
            case CLIENT_REQUEST_GET_STDOUT:
                get_stdout(taskid);
                break;
            case CLIENT_REQUEST_GET_STDERR:
                get_stderr(taskid);
                break;
        }
    }
    exit_loop : //spaghetti ?
}
*/
int main() {
    char *path_request = init_path_request(init_path());
    char *path_reply = init_path_reply(init_path());
    if (mkfifo(path_request,0666) == -1) { exit(EXIT_FAILURE); }
    if (mkfifo(path_reply,0666) == -1) { exit(EXIT_FAILURE); }

    create_daemon();
    while (1){ }
    //read_from_pipes();
    printf("zzzz\n");
    exit(0);
}