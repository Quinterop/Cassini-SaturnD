#include "../include/cassini.h"
/*
//assignation du répertoire par défaut des pipes
char *username = getlogin();
char *pipes_directory = "/tmp/<USERNAME>/saturnd/pipes";
size_t length = strlen("/tmp/") + strlen(username) + strlen("/saturnd/pipes");
char* pipes_directory = malloc(length + 1);
strcpy(pipes_directory, "/tmp/");
strcat(pipes_directory, username);
strcat(pipes_directory, "/saturnd/pipes");
char *path_request = malloc(strlen(pipes_directory) + strlen("/saturnd-request-pipe") + 1);
if (path_request == NULL) goto error;
strcat(strcpy(path_request, pipes_directory), "/saturnd-request-pipe");
char *path_reply = malloc(strlen(pipes_directory) + strlen("/saturnd-reply-pipe") + 1);
if (path_reply == NULL) goto error;
strcat(strcpy(path_reply, pipes_directory), "/saturnd-reply-pipe");*/

int create_daemon() {
    int pid = fork();
    if (pid == -1) {
        exit(EXIT_FAILURE);
    } 
    if(pid!=0) exit(EXIT_SUCCESS);
    pid = fork();
    if (pid == -1) {
        exit(EXIT_FAILURE);
    } 
    //if(pid!=0) exit(EXIT_SUCCESS);
    else if (pid == 0) {
        sleep(1); //le temps que le père meure*/
        if (getppid() !=0) {
            printf("daemon crée \n");
            if (setsid() == -1) {
                exit(1);
            }
            printf("PID : %d\nce terminal peut etre fermé\n", getpid());
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
            //nécessaire ?
            //todo log
        } else {
            printf("error %d\n", getppid());
        }
        return 0;
    } else {
        //pere a tuer
        printf("pid pere = %d\n",getpid());
        exit(0);
        printf("pere mort \n");
        //envoyer un signal a catch avec le fils pour confirmer ?
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
    create_daemon() ;
    while (1){}
    //read_from_pipes();
    printf("zzzz\n");
    exit(0);

    //error:
}

