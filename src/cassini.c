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

//prend une operation et un taskid et les ecrit dans pipe_request
int write_to_pipe(uint16_t operation, uint64_t taskid, char* path) {
    int errW;;
    int pipein = open(path, O_WRONLY);
    if (pipein == -1) { return -1; }
    uint16_t requete = htobe16(operation);
    taskid = htobe64(taskid);
    errW = write(pipein, &requete, sizeof(uint16_t));
    if (errW == -1) { return -1; }
    errW = write(pipein, &taskid, sizeof(uint64_t));
    if (errW == -1) { return -1; }
    close(pipein);
    return 0;
    /* TODO : utiliser qu'un seul write
    char *all[]
    write(path_request,all,sizeof(all));
    */
}

int main(int argc, char * argv[]) {
  errno = 0;

  int fd_request;
  int fd_reply;
  int errWorR;

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
  uint16_t errcode;

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


  //Commande LIST (-l)
  if(operation == CLIENT_REQUEST_LIST_TASKS) {
    //ECRITURE
    fd_request = open(path_request, O_WRONLY);
    if (fd_request == -1) { goto error; }
    opcode = htobe16(operation);
    errWorR = write(fd_request, &opcode, sizeof(uint16_t));
    if (errWorR == -1) { goto error; }
    close(fd_request);

    //LECTURE
    fd_reply = open(path_reply, O_RDONLY);
    if (fd_reply == -1) { goto error; } 
    //lecture du reptype
    errWorR = read(fd_reply, &reptype, sizeof(uint16_t));
    if (errWorR == -1) { goto error; }
    reptype = be16toh(reptype);
    if (reptype != SERVER_REPLY_OK) { goto error; }
    //lecture du nombre de tasks
    errWorR = read(fd_reply, &nbtasks, sizeof(uint32_t));
    if (errWorR == -1) { goto error; }
    nbtasks = be32toh(nbtasks);
    if (nbtasks != 0) {
      //pour chaque task
      for (int i = 0; i < nbtasks; i++) {
        //lecture du taskid
        errWorR = read(fd_reply, &taskid, sizeof(uint64_t));
        if (errWorR == -1) { goto error; }
        taskid = be64toh(taskid);
        printf("%" PRId64 ": ", taskid);

        //lecture du timing
        timing *t = malloc(sizeof(timing));
        if (t == NULL) { goto error; }
        errWorR = read(fd_reply, t, sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t));
        if (errWorR == -1) { goto error; }
        t->minutes = be64toh(t->minutes);
        t->hours = be32toh(t->hours);
        char *bufferTiming = malloc(TIMING_TEXT_MIN_BUFFERSIZE);
        if (bufferTiming == NULL) { goto error; }
        timing_string_from_timing(bufferTiming, t);
        printf("%s ", bufferTiming);

        //lecture de la commandline
        errWorR = read(fd_reply, &cmdArgc, sizeof(uint32_t)); //lecture du nombre d'argv pour la task[i]
        if (errWorR == -1) { goto error; }
        cmdArgc = be32toh(cmdArgc);
        for (int j = 0; j < cmdArgc; j++) {
          uint32_t stringL;
          errWorR = read(fd_reply, &stringL, sizeof(uint32_t)); //lecture de la longueur de l'argv[i]
          if (errWorR == -1) { goto error; }
          stringL = be32toh(stringL);
          char bufferCmd[stringL+1];
          errWorR = read(fd_reply, bufferCmd, stringL); //lecture du contenu de l'argv[i]
          if (errWorR == -1) { goto error; }
          bufferCmd[stringL] = '\0';
          printf("%s ", bufferCmd);
        }
        free(t);
        free(bufferTiming);
        printf("\n");
      }    
    }
    close(fd_reply);
  } //Fin commande LIST (-l)


  //Commande CREATE (-c)
  else if (operation == CLIENT_REQUEST_CREATE_TASK){
    //ECRITURE
    fd_request = open(path_request, O_WRONLY);
    if (fd_request == -1) { goto error; }
    
    //ecriture de l'opcode
    opcode = htobe16(operation);
    errWorR = write(fd_request, &opcode, sizeof(uint16_t));
    if (errWorR == -1) { goto error; }
    
    //ecriture du timing (MINUTES <uint64>, HOURS <uint32>, DAYSOFWEEK <uint8>)
    struct timing t;
    int n = timing_from_strings(&t, minutes_str, hours_str, daysofweek_str);
    if (n==-1) { goto error; }
    t.minutes = htobe64(t.minutes);
    t.hours = htobe32(t.hours);
    errWorR = write(fd_request, &t, sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t));
    if (errWorR == -1) { goto error; }
    
    //ecriture de la commandline (ARGC <uint32>, ARGV[0] <string>, ..., ARGV[ARGC-1] <string>)
    if(argc<1) goto error;
    if(argv[0]==NULL) goto error;
    //ecriture argc
    cmdArgc = htobe32(argc-optind);
    errWorR = write(fd_request, &cmdArgc, sizeof(uint32_t));
    if (errWorR == -1) { goto error; }
    //ecriture argv
    for (int i = optind; i < argc; i++) {
      uint32_t length = strlen(argv[i]);
      uint32_t h = htobe32(length);
      errWorR = write(fd_request, &h, sizeof(uint32_t));
      if (errWorR == -1) { goto error; }
      char *bufferCmd = argv[i];
      errWorR = write(fd_request, bufferCmd, length);
      if (errWorR == -1) { goto error; }
    }
    close(fd_request);

    //LECTURE
    fd_reply = open(path_reply, O_RDONLY);
    if (fd_reply == -1) { goto error; }
    //lecture du reptype
    errWorR = read(fd_reply, &reptype, sizeof(uint16_t));
    if (errWorR == -1) { goto error; }
    reptype = be16toh(reptype);
    if (reptype != SERVER_REPLY_OK) { goto error; }
    //lecture du taskid
    errWorR = read(fd_reply, &taskid, sizeof(uint64_t));
    if (errWorR == -1) { goto error; }
    taskid = be64toh(taskid);
    printf("%" PRId64 ": ", taskid);
    close(fd_reply);
  } //Fin commande CREATE (-c)


  //Commande STDERR (-e)
  else if (operation == CLIENT_REQUEST_GET_STDERR) {
    //ECRITURE
    int n = write_to_pipe(operation, taskid, path_request);
    if (n == -1) { goto error; }
    
    //LECTURE
    fd_reply = open(path_reply, O_RDONLY);
    if (fd_reply == -1) {goto error;}
    //lecture du reptype
    errWorR = read(fd_reply, &reptype, sizeof(uint16_t));
    if (errWorR == -1) { goto error; }
    reptype = be16toh(reptype);
    //lecture reptype ERROR
    if (reptype == SERVER_REPLY_ERROR){
      errWorR = read(fd_reply, &errcode, sizeof (uint16_t));
      if (errWorR == -1) { goto error; }
      close(fd_reply);
      if (errcode == SERVER_REPLY_ERROR_NEVER_RUN) {
        printf("il n'existe aucune tâche avec cet identifiant");
        goto error;
      } else if (errcode == SERVER_REPLY_ERROR_NOT_FOUND) {
          printf("la tâche n'a pas encore été exécutée au moins une fois");
          goto error;
      }
    }
    //lecture reptype OK
    else if (reptype == SERVER_REPLY_OK) {
      //lecture de la longueur de l'output
      uint32_t stringL; 
      errWorR = read(fd_reply, &stringL, sizeof(uint32_t));
      if (errWorR == -1) { goto error; }
      stringL = be32toh(stringL);
      char bufferOutput[stringL+1];
      //lecture du contenu de l'output
      errWorR = read(fd_reply, bufferOutput, stringL);
      if (errWorR == -1) { goto error; }
      bufferOutput[stringL] = '\0';
      printf("%s", bufferOutput);
    }
    close(fd_reply);
  } //Fin commande STDERR (-e)


  //Commande STDOUT (-o)
  else if (operation == CLIENT_REQUEST_GET_STDOUT){
    //ECRITURE
    int n = write_to_pipe(operation, taskid, path_request);
    if (n == -1) { goto error; }
    
    //LECTURE
    fd_reply = open(path_reply, O_RDONLY);
    if (fd_reply == -1) {goto error;}
    errWorR = read(fd_reply, &reptype, sizeof(uint16_t));
    if (errWorR == -1) { goto error; }
    reptype= be16toh(reptype); 
    //lecture reptype ERROR
    if(reptype == SERVER_REPLY_ERROR){
      read(fd_reply, &errcode, sizeof(uint16_t));
      if (errWorR == -1) { goto error; }
      close(fd_reply);
      errcode = be16toh(errcode);
      if (errcode == SERVER_REPLY_ERROR_NEVER_RUN) {
        printf("il n'existe aucune tâche avec cet identifiant");
        goto error;
      } else if (errcode == SERVER_REPLY_ERROR_NOT_FOUND) {
          printf("la tâche n'a pas encore été exécutée au moins une fois");
          goto error;
      }
    }
    //lecture reptype OK 
    else if (reptype == SERVER_REPLY_OK) {
      //lecture de la longueur de l'output
      uint32_t stringL; 
      errWorR = read(fd_reply, &stringL, sizeof(uint32_t));
      if (errWorR == -1) { goto error; }
      stringL = be32toh(stringL);
      char bufferOutput[stringL+1];
      //lecture du contenu de l'output
      errWorR = read(fd_reply, bufferOutput, stringL);
      if (errWorR == -1) { goto error; }
      bufferOutput[stringL] = '\0';
      printf("%s", bufferOutput);
    }
    close(fd_reply);
  } //Fin commande STDOUT (-o)


  //Commande REMOVE (-r)
  else if (operation == CLIENT_REQUEST_REMOVE_TASK) {
    //ECRITURE de l'opcode et du taskid
    int n = write_to_pipe(operation, taskid, path_request);
    if (n == -1) { goto error; }

    //LECTURE
    fd_reply = open(path_reply, O_RDONLY);
    if (fd_reply == -1) { goto error; }
    //lecture du reptype
    errWorR = read(fd_reply, &reptype, sizeof(uint16_t));
    if (errWorR == -1) { goto error; }
    reptype = be16toh(reptype);
    if (reptype == SERVER_REPLY_ERROR) {
      //lecture de l'errcode
      errWorR = read(fd_reply, &errcode, sizeof(uint16_t));
      if (errWorR == -1) { goto error; }
      close(fd_reply);
      errcode = be16toh(errcode);
      if (errcode == SERVER_REPLY_ERROR_NOT_FOUND) {
        printf("il n'existe aucune tâche avec cet identifiant");
        goto error;
      }
    } else if (reptype != SERVER_REPLY_OK) { //Si ce n'est ni ERROR ni OK alors erreur
        close(fd_reply);
        goto error; 
    }
    close(fd_reply);
  } //Fin commande REMOVE (-r)
  

  //Commande TIMES_EXITCODE (-x)
  else if (operation == CLIENT_REQUEST_GET_TIMES_AND_EXITCODES){
    //ECRITURE de l'opcode et du taskid
    int n = write_to_pipe(operation, taskid, path_request);
    if (n == -1) { goto error; }
    
    //LECTURE
    fd_reply = open(path_reply, O_RDONLY);
    if (fd_reply == -1) { goto error; }
    //lecture du reptype
    errWorR = read(fd_reply, &reptype, sizeof(uint16_t));
    if (errWorR == -1) { goto error; }
    reptype = be16toh(reptype);
    //lecture reptype OK
    if (reptype == SERVER_REPLY_OK) {
      //lecture du nombre de runs
      uint32_t nbruns;
      errWorR = read(fd_reply, &nbruns, sizeof(uint32_t));
      if (errWorR == -1) { goto error; }
      nbruns = be32toh(nbruns);
      for (int i=0; i<nbruns; i++) { //pour chaque run
        //lecture de la date et heure du run
        int64_t time;
        errWorR = read(fd_reply, &time, sizeof(int64_t));
        if (errWorR == -1) { goto error; }
        time = be64toh((uint64_t)time);
        struct tm * t = localtime(&time);
        char strBuffer[80];
        strftime(strBuffer, 80, "%F %T", t);
        printf("%s ", strBuffer);
        //lecture de l'exitcode du run
        uint16_t exitcode;
        errWorR = read(fd_reply, &exitcode, sizeof(uint16_t));
        if (errWorR == -1) { goto error; }
        exitcode = be16toh(exitcode);
        printf("%u\n", (unsigned int)exitcode);
      }
    //lecture reptype ERROR
    } else if (reptype == SERVER_REPLY_ERROR) {
      //lecture de l'errcode
      errWorR = read(fd_reply, &errcode, sizeof(uint16_t));
      if (errWorR == -1) { goto error; }
      close(fd_reply);
      errcode = be16toh(errcode);
      if (errcode == SERVER_REPLY_ERROR_NOT_FOUND) {
        printf("il n'existe aucune tâche avec cet identifiant");
        goto error;
      }
    }
    close(fd_reply);
  } 
  
  //Option TERMINATE (-q)
  else if(operation == CLIENT_REQUEST_TERMINATE) {
    //ECRITURE
    fd_request = open(path_request, O_WRONLY);
    if (fd_request == -1) { goto error; }
    opcode = htobe16(operation);
    errWorR = write(fd_request, &opcode, sizeof(uint16_t));
    if (errWorR == -1) { goto error; }
    close(fd_request);
    
    //LECTURE
    fd_reply = open(path_reply, O_RDONLY);
    if (fd_reply == -1) { goto error; }
    errWorR = read(fd_reply, &reptype, sizeof(uint16_t));
    if (errWorR == -1) { goto error; }
    close(fd_reply);
    reptype = be16toh(reptype);
    if (reptype != SERVER_REPLY_OK) { goto error; }
  } //Fin option TERMINATE (-q)


  free(path_request);
  free(path_reply);
  free(pipes_directory);
  return EXIT_SUCCESS;

  error:
    if (errno != 0) perror("main");
    if(pipes_directory) { free(pipes_directory); }
    if(path_request) { free(path_request); }
    if(path_reply) { free(path_reply); }
    pipes_directory = NULL;
    return EXIT_FAILURE;
}
