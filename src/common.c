#include "../include/common.h"

char * init_path() {
    char *username = getpwuid(getuid())->pw_name;
    size_t length = strlen("/tmp/") + strlen(username) + strlen("/saturnd/pipes");
    char *pipes_directory = malloc(length + 1);
    if (pipes_directory == NULL) { exit(EXIT_FAILURE); }
    //strcat(strcat(strcpy(pipes_directory, "/tmp/"), username), "/saturnd/pipes");
    strcpy(pipes_directory, "/tmp/");
    strcat(pipes_directory, username);
    strcat(pipes_directory, "/saturnd/pipes");
    return pipes_directory;
}

char * init_path_request(char *pipes_directory) {
    char *path_request = malloc(strlen(pipes_directory) + strlen("/saturnd-request-pipe") + 1);
    if (path_request == NULL) { exit(EXIT_FAILURE); }
    strcat(strcpy(path_request, pipes_directory), "/saturnd-request-pipe");
    return path_request;
}

char * init_path_reply(char *pipes_directory) {
    char *path_reply = malloc(strlen(pipes_directory) + strlen("/saturnd-reply-pipe") + 1);
    if (path_reply == NULL) { exit(EXIT_FAILURE); }
    strcat(strcpy(path_reply, pipes_directory), "/saturnd-reply-pipe");
    return path_reply;
}

//free les espaces memoire alloues et return EXIT_FAILURE
int free_and_exit(char *path_request, char *path_reply) {
  free(path_request);
  free(path_reply);
  path_request = NULL;
  path_reply = NULL;
  exit( EXIT_FAILURE );
}
