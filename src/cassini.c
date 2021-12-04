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

  int fd_request;
  int fd_reply;

  char * minutes_str = "*";
  char * hours_str = "*";
  char * daysofweek_str = "*";
  char * pipes_directory = NULL;

  uint16_t operation = CLIENT_REQUEST_LIST_TASKS;
  uint16_t opcode;
  uint16_t reptype;
  uint32_t nbtasks;
  uint32_t cmdArgc;
  uint64_t taskid;

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

  //Chemin par defaut du dossier des pipes
  if(pipes_directory==NULL){
    char *username = getlogin();
    size_t length = strlen("/tmp/") + strlen(username) + strlen("/saturnd/pipes");
    char* pipes_directory = malloc(length + 1);
    if (pipes_directory == NULL) goto error;
    //strcat(strcat(strcpy(pipes_directory, "/tmp/"), username), "/saturnd/pipes");
    strcpy(pipes_directory, "/tmp/");
    strcat(pipes_directory, username);
    strcat(pipes_directory, "/saturnd/pipes");
  }
  
  //Chemins des request_pipe et reply_pipe
  char *path_request = malloc(strlen(pipes_directory) + strlen("/saturnd-request-pipe") + 1);
  if (path_request == NULL) goto error;
  strcat(strcpy(path_request, pipes_directory), "/saturnd-request-pipe");
  char *path_reply = malloc(strlen(pipes_directory) + strlen("/saturnd-reply-pipe") + 1);
  if (path_reply == NULL) goto error;
  strcat(strcpy(path_reply, pipes_directory), "/saturnd-reply-pipe");
  free(pipes_directory);


  //Option LIST TASKS (-l)
  if(operation == CLIENT_REQUEST_LIST_TASKS) {
    //ECRITURE
    fd_request = open(path_request, O_WRONLY);
    if (fd_request == -1) { goto error; }
    opcode = htobe16(operation);
    write(fd_request, &opcode, sizeof(uint16_t));
    close(fd_request);

    //LECTURE
    fd_reply = open(path_reply, O_RDONLY);
    if (fd_reply == -1) { goto error; } 
    read(fd_reply, &reptype, sizeof(uint16_t));
    read(fd_reply, &nbtasks, sizeof(uint32_t));
    reptype = be16toh(reptype);
    nbtasks = be32toh(nbtasks);
    if(nbtasks != 0) {
      for (int i = 0; i < nbtasks; i++) { //pour chaque task
        //read du taskid
        read(fd_reply, &taskid, sizeof(uint64_t));
        taskid = be64toh(taskid);
        printf("%" PRId64 ": ", taskid);

        //read du timing
        timing *t = malloc(sizeof(timing));
        if (t == NULL) { goto error; }
        read(fd_reply, t, sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t));
        t->minutes = be64toh(t->minutes);
        t->hours = be32toh(t->hours);
        char *bufferTiming = malloc(TIMING_TEXT_MIN_BUFFERSIZE);
        if (bufferTiming == NULL) { goto error; }
        timing_string_from_timing(bufferTiming, t);
        printf("%s ", bufferTiming);

        //read de la commandline
        read(fd_reply, &cmdArgc, sizeof(uint32_t)); //read du nombre d'argv pour la task[i]
        cmdArgc = be32toh(cmdArgc);
        for (int j = 0; j < cmdArgc; j++) {
          uint32_t stringL;
          read(fd_reply, &stringL, sizeof(uint32_t)); //read de la longueur de l'argv[i]
          stringL = be32toh(stringL);
          char bufferCmd[stringL+1];
          read(fd_reply, bufferCmd, stringL); //read du contenu de l'argv[i]
          bufferCmd[stringL] = '\0';
          printf("%s ", bufferCmd);
        }
        free(t);
        free(bufferTiming);
        printf("\n");
      }    
    }
    close(fd_reply);
  } //Fin option LIST TASKS (-l)


  //COMMANDE REQUETE CREATE -c:
  else if (operation == CLIENT_REQUEST_CREATE_TASK){
    //ECRITURE
    fd_request = open(path_request, O_WRONLY);
    if (fd_request == -1) { goto error; }
    
    //Ecriture de l'OPCODE (OPCODE='CR' <uint16>) :
    opcode = htobe16(operation);
    write(fd_request, &opcode, sizeof(uint16_t));
    
    //Ecriture du TIMING (MINUTES <uint64>, HOURS <uint32>, DAYSOFWEEK <uint8>) :
    struct timing t; //ecrire le resultat sous forme d'une struct timing
    int n = timing_from_strings(&t, minutes_str, hours_str, daysofweek_str);
    if (n==-1){ goto error; }
    t.minutes = htobe64(t.minutes);
    t.hours = htobe32(t.hours);
    write(fd_request, &t, sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t));
    
    //Ecriture de la COMMANDLINE (ARGC <uint32>, ARGV[0] <string>, ..., ARGV[ARGC-1] <string>) :
    if(argc<1) goto error;
    if(argv[0]==NULL) goto error;
    //Ecriture argc
    cmdArgc = htobe32(argc-optind);
    write(fd_request, &cmdArgc, sizeof(uint32_t));
    //Ecriture argv
    for (int i = optind; i < argc; i++) {
      uint32_t length = strlen(argv[i]);
      uint32_t h = htobe32(length);
      write(fd_request, &h, sizeof(uint32_t));
      char *bufferCmd = argv[i];
      write(fd_request, bufferCmd, length);
    }
    close(fd_request);

    //LECTURE
    fd_reply = open(path_reply, O_RDONLY);
    if (fd_reply == -1) { goto error; }
    read(fd_reply, &reptype, sizeof(uint16_t));
    reptype = be16toh(reptype);
    read(fd_reply, &taskid, sizeof(uint64_t));
    taskid = be64toh(taskid);
    printf("%" PRId64 ": ", taskid);
    close(fd_reply);
  } //Fin option CREATE (-c)


  //Option TERMINATE (-q)
  else if(operation == CLIENT_REQUEST_TERMINATE) {
    //ECRITURE
    fd_request = open(path_request, O_WRONLY);
    if (fd_request == -1) { goto error; }
    opcode = htobe16(operation);
    write(fd_request, &opcode, sizeof(uint16_t));
    close(fd_request);
    
    //LECTURE
    fd_reply = open(path_reply, O_RDONLY);
    if (fd_reply == -1) { goto error; }
    read(fd_reply, &reptype, sizeof(uint16_t));
    reptype = be16toh(reptype);
    close(fd_reply);
  } //Fin option TERMINATE (-q)


  free(path_request);
  free(path_reply);

  return EXIT_SUCCESS;

 error:
  if (errno != 0) perror("main");
  free(pipes_directory);
  free(path_request);
  free(path_reply);
  pipes_directory = NULL;
  return EXIT_FAILURE;
}
