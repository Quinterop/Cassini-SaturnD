#include "../include/cassini.h"

char *pipes_directory = "/tmp/<USERNAME>/saturnd/pipes";

int create_daemon() {
    int pid = fork();
    if (pid == -1) {
        exit(1);
    } else if (pid == 0) {
        sleep(1); //le temps que le père meure
        if (getppid() == 1) {
            printf("daemon crée \n");
            int er = setsid(); //todo check erreur
            if (er == -1) {
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
        exit(0);
        //envoyer un signal a catch avec le fils pour confirmer ?
    }
}

void read_from_pipes() {
    while (1) { //boucle nécéssaire ?
        int fd_request = open(pipes_directory, O_RDONLY);//test err
        uint16_t request;
        int rd = read(fd_request, &request, sizeof(uint16_t));//test err
        request = be16toh(request);
        switch (request) {
            //TODO
        }
    }
}

int main() {
    create_daemon();
    read_from_pipes();
    printf("zzzz\n");
    exit(0);
}