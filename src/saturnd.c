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


int send_reply_bool(int reptype, char *path_reply){//0 pour ok et autre pour erreur
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
 
int sameTaskid(char *name, uint64_t taskid) {
    uint64_t nameInt = ToUInt64(name);
    return (nameInt == taskid);
}

void get_stdout(char *tasks_directory, int fd_request, char * path_request, char * path_reply) {
    uint64_t taskid;
    if (read(fd_request, &taskid, sizeof(uint64_t)) == -1) {
        free_and_exit(path_request, path_reply); 
    }
    taskid = be64toh(taskid);
    DIR * dirp = opendir(tasks_directory);
    struct dirent *entry;
    while ((entry = readdir(dirp)) != NULL) {
        char *name = entry->d_name;
        if (entry->d_type == DT_DIR) {
            if (sameTaskid(name,taskid)) {
                char * path = malloc(1024);
                sprintf(path, "%s/%s" ,tasks_directory,name);
                char * filePathSO = malloc(1024);
                sprintf(filePathSO, "%s/stdout", path);
                int fd = open(filePathSO, O_RDONLY);
                if (fd == -1) { free_and_exit(path_request,path_reply); }
                
                //lecture dans le fichier
                uint32_t length; 
                if (read(fd, &length, sizeof(uint32_t)) == -1) {
                    free_and_exit(path_request, path_reply);
                }
                char bufferOutput[length];
                if (read(fd, &bufferOutput, length) == -1) {
                    free_and_exit(path_request, path_reply);
                }
                close(fd);
                close(fd_request);
                free(path);
                free(filePathSO);
                
                if (length == 0) {
                    //ecriture dans le pipe reply pour le reptype OK
                    uint16_t reptype = htobe16(SERVER_REPLY_ERROR);
                    uint16_t errcode = htobe16(SERVER_REPLY_ERROR_NEVER_RUN);
                    int buffer_size = sizeof(uint16_t) + sizeof(uint16_t);
                    char bufferWrite[buffer_size];
                    memmove(bufferWrite,&reptype,sizeof(uint16_t));
                    memmove(bufferWrite+sizeof(uint16_t),&errcode,sizeof(uint16_t));
                    int fd_reply = open(path_reply,O_WRONLY);
                    if (fd_reply == -1) { free_and_exit(path_request,path_reply); }
                    if (write(fd_reply, bufferWrite, buffer_size) == -1) { free_and_exit(path_request,path_reply); }
                    close(fd_reply);
                }
                else {
                    //ecriture dans le pipe reply pour le reptype ERROR
                    uint16_t reptype = htobe16(SERVER_REPLY_OK);
                    int buffer_size = sizeof(uint16_t) + sizeof(uint32_t) + length;
                    char bufferWrite[buffer_size];
                    uint32_t stringL = htobe32(length);
                    memmove(bufferWrite,&reptype,sizeof(uint16_t));
                    memmove(bufferWrite+sizeof(uint16_t),&stringL,sizeof(uint32_t));
                    memmove(bufferWrite+sizeof(uint16_t)+sizeof(uint32_t),bufferOutput,length);
                    int fd_reply = open(path_reply,O_WRONLY);
                    if (fd_reply == -1) { free_and_exit(path_request,path_reply); }
                    if (write(fd_reply, bufferWrite, buffer_size) == -1) { free_and_exit(path_request,path_reply); }
                    close(fd_reply);
                }
            }
        }
    }
    closedir(dirp);
}


uint16_t remove_task(char * tasks_directory, int fd_request, char * path_request, char * path_reply){ 
    uint64_t taskid;
    if (read(fd_request, &taskid, sizeof(uint64_t)) == -1) {
        free_and_exit(path_request, path_reply); 
    }
    taskid = be64toh(taskid);
    uint16_t reptype = SERVER_REPLY_ERROR;
    char output[50];
    sprintf(output,"%d",taskid);
    struct dirent *dir;
    DIR *d = opendir(tasks_directory); 
    if (d) {
        while ((dir = readdir(d)) != NULL){
            if (strcmp(dir->d_name,output)==0){
                unlink(strcat("/tmp/<username>/saturnd/taches/",dir->d_name));
                reptype = SERVER_REPLY_OK;
            }
        }
    }
    int fd_reply = open (path_reply, O_WRONLY);
    reptype = hto16(reptype);
    int err = write(fd_reply, &reptype, sizeof(uint16_t));
    if (err == -1) { free(path_reply); }
    close(fd_reply);
    return reptype;  
}



void read_from_pipes(char * path_request, char * path_reply, char * path_tasks) {
    int fd_request = open(path_request, O_RDONLY); 
    if (fd_request == -1) { free_and_exit(path_request,path_reply); }
    struct pollfd pfd[1];
    pfd[0].fd = fd_request;
    pfd[0].events=POLLIN;

    while (1) {
        poll(pfd,1,60000);
        if (pfd[0].revents & POLLIN){

            uint16_t opcode_req;
            if (read(fd_request, &opcode_req, sizeof(uint16_t)) == -1) { 
                free_and_exit(path_request, path_reply);
            }
            opcode_req = be16toh(opcode_req);
            switch (opcode_req) {
                case CLIENT_REQUEST_TERMINATE:
                    send_reply_bool(1,path_reply);
                    break;
                // case CLIENT_REQUEST_LIST_TASKS:
                //     list_tasks();
                //     break;
                // case CLIENT_REQUEST_CREATE_TASK:
                //     create_task(timing,commandline);
                //     break;
                case CLIENT_REQUEST_REMOVE_TASK:
                    remove_task(path_tasks, fd_request, path_request, path_reply);
                    break;
                // case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES:
                //     get_times_and_exitcodes(taskid);
                //     break;
                case CLIENT_REQUEST_GET_STDOUT:
                    get_stdout(path_tasks,fd_request,path_request, path_reply);
                    break;
                // case CLIENT_REQUEST_GET_STDERR:
                //     get_stderr(taskid);
                //     break;
            }
        }
    }
}

int main() {
    char *path_request = init_path_request(init_path());
    char *path_reply = init_path_reply(init_path());
    char *path_tasks = init_path_tasks(init_path());
    if (mkfifo(path_request,0666) == -1) { goto error; }
    if (mkfifo(path_reply,0666) == -1) { goto error; }

    create_daemon();
    read_from_pipes(path_request,path_reply,path_tasks);

    free(path_request);
    free(path_reply);
    return EXIT_SUCCESS;

    error:
      if (errno != 0) perror("main");
      free(path_request);
      free(path_reply);
      return EXIT_FAILURE;

}
