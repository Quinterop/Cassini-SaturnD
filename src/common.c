#include "../include/common.h"

int write_uint16(uint16_t message,char* path){
    int fd_reply = open(path,O_WRONLY);
    if (fd_reply == -1) return 1;//todo
    message = htobe16(message);
    int errW = write(fd_reply, &message, sizeof(uint16_t));
    if (errW == -1) return 1;//todo
    close(fd_reply);
    return 0;
}

//renvoie le reptype lu, ou -1
uint16_t read_uint16(int fd_reply, char *path_reply, char *path_request) {
    uint16_t reptype;
    int err = read(fd_reply, &reptype, sizeof(uint16_t));
    if (err == -1) { free_and_exit(path_request, path_reply); }
    reptype = be16toh(reptype);
    return reptype;
}

int write_uint64(uint64_t message,char* path){
    int fd_reply = open(path,O_WRONLY);
    if (fd_reply == -1) return 1;//todo
    message = htobe64(message);
    int errW = write(fd_reply, &message, sizeof(uint64_t));
    if (errW == -1) return 1;//todo
    close(fd_reply);
    return 0;
}

