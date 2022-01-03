#include "../include/cassini.h"


int create_daemon() {
    int pid = fork();
    if (pid == -1) {
        //échec
        exit(1);
    } else if (pid == 0) {

        //fils
        printf("filspid %d\n",getpid());
        sleep(1);
        if(getppid()==1){
            printf("daemon\n");
            setsid(); //todo check erreur
            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);
            //todo log
        }else{
            printf("error %d\n",getppid());
        }
        return 0;
    } else{

        //pere a tuer
        printf("pere %d\n",getpid());
        exit(0);
        //envoyer un signal a catch avec le fils pour confirmer ?
    }
}

int main() {
    printf("print3 %d\n",create_daemon());
    printf("\n%d\n",getppid());
    exit(0);
}