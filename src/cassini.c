#include "../include/cassini.h"

const char usage_info[] = "\
   usage: cassini [OPTIONS] -l -> list all tasks\n\
      or: cassini [OPTIONS]    -> same\n\
      or: cassini [OPTIONS] -q -> terminate the daemon\n\
      or: cassini [OPTIONS] -c [-m MINUTES] [-H HOURS] [-d DAYSOFWEEK] COMMAND_NAME [ARG_1] ... [ARG_N]\n\
          -> add a new task and print its TASKID\n\
             format & semantics of the \"timing\" fields defined here:\n\
             https://pubs.opengroup.org/onlinepubs/9699919799/utilities/crontab.html\n\
             default value for each field is \"*\"\n\
      or: cassini [OPTIONS] -r TASKID -> remove a task\n\
      or: cassini [OPTIONS] -x TASKID -> get info (time + exit code) on all the past runs of a task\n\
      or: cassini [OPTIONS] -o TASKID -> get the standard output of the last run of a task\n\
      or: cassini [OPTIONS] -e TASKID -> get the standard error\n\
      or: cassini -h -> display this message\n\
\n\
   options:\n\
     -p PIPES_DIR -> look for the pipes in PIPES_DIR (default: /tmp/<USERNAME>/saturnd/pipes)\n\
";

int main(int argc, char * argv[]) {
  errno = 0;

  char * minutes_str = "*";
  char * hours_str = "*";
  char * daysofweek_str = "*";
  char * pipes_directory = NULL;

  uint16_t operation = CLIENT_REQUEST_LIST_TASKS;
  uint64_t taskid;

  char *username; 
  struct passwd *pass; 
  pass = getpwuid(getuid()); 
  username = pass->pw_name;

  int opt;
  char * strtoull_endp;
  while ((opt = getopt(argc, argv, "hlcqm:H:d:p:r:x:o:e:")) != -1) {
    switch (opt) {
    case 'm':
      minutes_str = optarg;
      break;
    case 'H':
      hours_str = optarg;
      break;
    case 'd':
      daysofweek_str = optarg;
      break;
    case 'p':
      pipes_directory = strdup(optarg);
      if (pipes_directory == NULL) goto error;
      break;
    case 'l':
      operation = CLIENT_REQUEST_LIST_TASKS;
      break;
    case 'c':
      operation = CLIENT_REQUEST_CREATE_TASK;
      break;
    case 'q':
      operation = CLIENT_REQUEST_TERMINATE;
      break;
    case 'r':
      operation = CLIENT_REQUEST_REMOVE_TASK;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'x':
      operation = CLIENT_REQUEST_GET_TIMES_AND_EXITCODES;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'o':
      operation = CLIENT_REQUEST_GET_STDOUT;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'e':
      operation = CLIENT_REQUEST_GET_STDERR;
      taskid = strtoull(optarg, &strtoull_endp, 10);
      if (strtoull_endp == optarg || strtoull_endp[0] != '\0') goto error;
      break;
    case 'h':
      printf("%s", usage_info);
      return 0;
    case '?':
      fprintf(stderr, "%s", usage_info);
      goto error;
    }
  }

  //Chemin par defaut des pipes
  if(pipes_directory==NULL){
    size_t length = strlen("/tmp/") + strlen(username) + strlen("/saturnd/pipes");
    char* pipes_directory = malloc(length + 1);
    if (pipes_directory == NULL) goto error;
    strcat(strcat(strcpy(pipes_directory, "/tmp/"), username), "/saturnd/pipes");
    // ou dessous
    // strcpy(pipes_directory, "/tmp/");
    // strcat(pipes_directory, username);
    // strcat(pipes_directory, "/saturnd/pipes");
  }
  
  char *path_request = malloc(strlen(pipes_directory) + strlen("/saturnd-request-pipe") + 1);
  if (path_request == NULL) goto error;
  strcat(strcpy(path_request, pipes_directory), "/saturnd-request-pipe");
  char *path_reply = malloc(strlen(pipes_directory) + strlen("/saturnd-reply-pipe") + 1);
  if (path_reply == NULL) goto error;
  strcat(strcpy(path_reply, pipes_directory), "/saturnd-reply-pipe");

  //Option LIST TASKS (-l)
  if(operation == CLIENT_REQUEST_LIST_TASKS) {
    int fd_request = open(path_request, O_WRONLY);
    if (fd_request == -1) {
      close(fd_request);
      goto error;
    }
    uint16_t opcode = htobe16(operation);
    write(fd_request,&opcode,sizeof(uint16_t));
    close(fd_request);

    int fd_reply = open(path_reply, O_RDONLY);
    if (fd_reply == -1) {
      close(fd_reply);
      goto error;
    }
    uint16_t reptype;
    uint32_t nbtasks;
    read(fd_reply, &reptype, sizeof(uint16_t));
    read(fd_reply, &nbtasks, sizeof(uint32_t));
    reptype = be16toh(reptype);
    nbtasks = be32toh(nbtasks);
    if(nbtasks != 0) {
      // TODO L'affichage de la liste des tasks
    }
    close(fd_reply);
  }

  return EXIT_SUCCESS;

 error:
  if (errno != 0) perror("main");
  free(pipes_directory);
  free(path_request);
  free(path_reply);
  pipes_directory = NULL;
  return EXIT_FAILURE;
}
