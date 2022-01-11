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

//prend un operation et l'ecrit dans le pipe request
void write_op_to_pipe(uint16_t operation, char *path_request, char *path_reply) {
  int errW;
  int fd_request = open(path_request, O_WRONLY);
  if (fd_request == -1) { free_and_exit(path_request, path_reply); }
  uint16_t opcode = htobe16(operation);
  errW = write(fd_request, &opcode, sizeof(uint16_t));
  if (errW == -1) { free_and_exit(path_request, path_reply); }
  close(fd_request);
}


//prend une operation et un taskid et les ecrit dans le pipe request
void write_op_taskid_to_pipe(uint16_t operation, uint64_t taskid, char* path_request, char *path_reply) {
  int errW;
  int fd_request = open(path_request, O_WRONLY);
  if (fd_request == -1) { free_and_exit(path_request, path_reply); }
  uint16_t requete = htobe16(operation);
  taskid = htobe64(taskid);

  int buffer_size = sizeof(uint16_t) + sizeof(uint64_t);
  char buffer[buffer_size];
  memmove(buffer,&requete,sizeof(uint16_t));
  memmove(buffer+sizeof(uint16_t),&taskid,sizeof(uint64_t));
  errW = write(fd_request, buffer, buffer_size);
  if (errW == -1) { free_and_exit(path_request, path_reply); }
  close(fd_request);
}


//ecriture pour la commande CREATE
void write_create(uint16_t operation, int argc, char * argv[], char * minutes_str, char * hours_str, char * daysofweek_str, char *path_request, char *path_reply) {
  int fd_request = open(path_request, O_WRONLY);
  if (fd_request == -1) { free_and_exit(path_request, path_reply); }
  char buffer[1024];
  
  //ecriture de l'opcode
  int buffer_size = sizeof(uint16_t);
  uint16_t opcode = htobe16(operation);
  memmove(buffer,&opcode,sizeof(uint16_t));
  
  //ecriture du timing (MINUTES <uint64>, HOURS <uint32>, DAYSOFWEEK <uint8>)
  struct timing t;
  int n = timing_from_strings(&t, minutes_str, hours_str, daysofweek_str);
  if (n==-1) { free_and_exit(path_request, path_reply); }
  t.minutes = htobe64(t.minutes);
  t.hours = htobe32(t.hours);
  memmove(buffer+buffer_size, &t, sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t));
  buffer_size += sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t);
  
  //ecriture de la commandline (ARGC <uint32>, ARGV[0] <string>, ..., ARGV[ARGC-1] <string>)
  if(argc<1) { free_and_exit(path_request, path_reply); }
  if(argv[0]==NULL) { free_and_exit(path_request, path_reply); }
  //ecriture argc
  uint32_t cmdArgc = htobe32(argc-optind);
  memmove(buffer+buffer_size, &cmdArgc, sizeof(uint32_t));
  buffer_size += sizeof(uint32_t);
  //ecriture argv
  for (int i = optind; i < argc; i++) {
    uint32_t length = strlen(argv[i]);
    uint32_t h = htobe32(length);
    memmove(buffer+buffer_size, &h, sizeof(uint32_t));
    char *bufferCmd = argv[i];
    memmove(buffer+buffer_size+sizeof(uint32_t), bufferCmd, length);
    buffer_size += sizeof(uint32_t)+length;
  }
  int err = write(fd_request, buffer, buffer_size);
  if (err == -1) { free_and_exit(path_request, path_reply); }
  close(fd_request);
}


//renvoie le reptype lu, ou -1
uint16_t read_reptype(int fd_reply, char *path_reply, char *path_request) {
  uint16_t reptype;
  int err = read(fd_reply, &reptype, sizeof(uint16_t));
  if (err == -1) { free_and_exit(path_request, path_reply); }
  reptype = be16toh(reptype);
  return reptype;
}


//renvoie l'errcode lu, ou -1
uint16_t read_errcode(int fd_reply, char *path_reply, char *path_request) {
  uint16_t errcode;
  int err = read(fd_reply, &errcode, sizeof(uint16_t));
  if (err == -1) { free_and_exit(path_request, path_reply); }
  close(fd_reply);
  errcode = be16toh(errcode);
  return errcode;
}


//lecture pour les options e et o (reptype OK ou ERROR)
void read_reptype_e_o(char *path_reply, char *path_request) {
  int fd_reply = open(path_reply, O_RDONLY);
  if (fd_reply == -1) { free_and_exit(path_request, path_reply); }
  //lecture du reptype
  uint16_t reptype = read_reptype(fd_reply, path_reply, path_request);
  //lecture pour reptype ERROR
  if (reptype == SERVER_REPLY_ERROR){
    uint16_t errcode = read_errcode(fd_reply, path_reply, path_request);
    if (errcode == SERVER_REPLY_ERROR_NEVER_RUN) {
      printf("il n'existe aucune tâche avec cet identifiant");
      free_and_exit(path_request, path_reply);
    } else if (errcode == SERVER_REPLY_ERROR_NOT_FOUND) {
      printf("la tâche n'a pas encore été exécutée au moins une fois");
      free_and_exit(path_request, path_reply);
    } else { free_and_exit(path_request, path_reply); }
  }
  //lecture pour reptype OK
  else if (reptype == SERVER_REPLY_OK) {
    //lecture de la longueur de l'output
    uint32_t stringL; 
    int err = read(fd_reply, &stringL, sizeof(uint32_t));
    if (err == -1) { free_and_exit(path_request, path_reply); }
    stringL = be32toh(stringL);
    char bufferOutput[stringL+1];
    //lecture du contenu de l'output
    err = read(fd_reply, bufferOutput, stringL);
    if (err == -1) { free_and_exit(path_request, path_reply); }
    bufferOutput[stringL] = '\0';
    printf("%s", bufferOutput);
  }
  close(fd_reply);
}


//lecture pour la commande LIST
void read_list(char *path_reply, char *path_request) {
  uint32_t nbtasks;
  uint32_t cmdArgc;
  uint64_t taskid;

  int fd_reply = open(path_reply, O_RDONLY);
  if (fd_reply == -1) { free_and_exit(path_request, path_reply); } 
  //lecture du reptype
  uint16_t reptype = read_reptype(fd_reply, path_reply, path_request);
  if (reptype != SERVER_REPLY_OK) { free_and_exit(path_request, path_reply); }
  //lecture du nombre de tasks
  int err = read(fd_reply, &nbtasks, sizeof(uint32_t));
  if (err == -1) { free_and_exit(path_request, path_reply); }
  nbtasks = be32toh(nbtasks);
  if (nbtasks != 0) {
    //pour chaque task
    for (int i = 0; i < nbtasks; i++) {
      //lecture du taskid
      err = read(fd_reply, &taskid, sizeof(uint64_t));
      if (err == -1) { free_and_exit(path_request, path_reply); }
      taskid = be64toh(taskid);
      printf("%" PRId64 ": ", taskid);

      //lecture du timing
      timing *t = malloc(sizeof(timing));
      if (t == NULL) { free_and_exit(path_request, path_reply); }
      err = read(fd_reply, t, sizeof(uint64_t)+sizeof(uint32_t)+sizeof(uint8_t));
      if (err == -1) { free_and_exit(path_request, path_reply); }
      t->minutes = be64toh(t->minutes);
      t->hours = be32toh(t->hours);
      char *bufferTiming = malloc(TIMING_TEXT_MIN_BUFFERSIZE);
      if (bufferTiming == NULL) { free_and_exit(path_request, path_reply); }
      timing_string_from_timing(bufferTiming, t);
      printf("%s ", bufferTiming);

      //lecture de la commandline
      err = read(fd_reply, &cmdArgc, sizeof(uint32_t)); //lecture du nombre d'argv pour la task[i]
      if (err == -1) { free_and_exit(path_request, path_reply); }
      cmdArgc = be32toh(cmdArgc);
      for (int j = 0; j < cmdArgc; j++) {
        uint32_t stringL;
        err = read(fd_reply, &stringL, sizeof(uint32_t)); //lecture de la longueur de l'argv[i]
        if (err == -1) { free_and_exit(path_request, path_reply); }
        stringL = be32toh(stringL);
        char bufferCmd[stringL+1];
        err = read(fd_reply, bufferCmd, stringL); //lecture du contenu de l'argv[i]
        if (err == -1) { free_and_exit(path_request, path_reply); }
        bufferCmd[stringL] = '\0';
        printf("%s ", bufferCmd);
      }
      free(t);
      free(bufferTiming);
      printf("\n");
    }    
  }
  close(fd_reply);
}


//lecture pour la commande TIMES_EXITCODE
void read_times_exitcode(char *path_reply, char *path_request) {
  int fd_reply = open(path_reply, O_RDONLY);
  if (fd_reply == -1) { free_and_exit(path_request, path_reply); }
  //lecture du reptype
  uint16_t reptype = read_reptype(fd_reply, path_reply, path_request);
  //lecture pour reptype OK
  if (reptype == SERVER_REPLY_OK) {
    //lecture du nombre de runs
    uint32_t nbruns;
    int err = read(fd_reply, &nbruns, sizeof(uint32_t));
    if (err == -1) { free_and_exit(path_request, path_reply); }
    nbruns = be32toh(nbruns);
    for (int i=0; i<nbruns; i++) { //pour chaque run
      //lecture de la date et heure du run
      int64_t time;
      err = read(fd_reply, &time, sizeof(int64_t));
      if (err == -1) { free_and_exit(path_request, path_reply); }
      time = be64toh((uint64_t)time);
      struct tm * t = localtime(&time);
      char strBuffer[80];
      strftime(strBuffer, 80, "%F %T", t);
      printf("%s ", strBuffer);
      //lecture de l'exitcode du run
      uint16_t exitcode;
      err = read(fd_reply, &exitcode, sizeof(uint16_t));
      if (err == -1) { free_and_exit(path_request, path_reply); }
      exitcode = be16toh(exitcode);
      printf("%u\n", (unsigned int)exitcode);
    }
  //lecture pour reptype ERROR
  } else if (reptype == SERVER_REPLY_ERROR) {
    //lecture de l'errcode
    uint16_t errcode = read_errcode(fd_reply, path_reply, path_request);
    if (errcode == SERVER_REPLY_ERROR_NOT_FOUND) {
      printf("il n'existe aucune tâche avec cet identifiant");
      free_and_exit(path_request, path_reply);
    }
  }
  close(fd_reply);
}



int main(int argc, char * argv[]) {
  errno = 0;

  int fd_reply;
  int err;

  char * minutes_str = "*";
  char * hours_str = "*";
  char * daysofweek_str = "*";
  char * pipes_directory = NULL;

  uint16_t operation = CLIENT_REQUEST_LIST_TASKS;
  uint16_t reptype;
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
    pipes_directory = init_path();
  }
  
  //Chemins des request_pipe et reply_pipe
  char *path_request = init_path_request(pipes_directory);
  char *path_reply = init_path_reply(pipes_directory);
  
  free(pipes_directory);


  //Commande LIST (-l)
  if(operation == CLIENT_REQUEST_LIST_TASKS) {
    //ECRITURE
    write_op_to_pipe(operation, path_request, path_reply);

    //LECTURE
    read_list(path_reply, path_request);
  } //Fin commande LIST (-l)


  //Commande CREATE (-c)
  else if (operation == CLIENT_REQUEST_CREATE_TASK){
    //ECRITURE
    write_create(operation, argc, argv, minutes_str, hours_str, daysofweek_str, path_request, path_reply);
  
    //LECTURE
    fd_reply = open(path_reply, O_RDONLY);
    if (fd_reply == -1) { goto error; }
    //lecture du reptype
    reptype = read_reptype(fd_reply, path_reply, path_request);
    if (reptype != SERVER_REPLY_OK) { goto error; }
    //lecture du taskid
    err = read(fd_reply, &taskid, sizeof(uint64_t));
    if (err == -1) { goto error; }
    taskid = be64toh(taskid);
    printf("%" PRId64 ": ", taskid);
    close(fd_reply);
  } //Fin commande CREATE (-c)


  //Commande STDERR (-e)
  else if (operation == CLIENT_REQUEST_GET_STDERR) {
    //ECRITURE
    write_op_taskid_to_pipe(operation, taskid, path_request, path_reply);
    
    //LECTURE
    read_reptype_e_o(path_reply, path_request);
  } //Fin commande STDERR (-e)


  //Commande STDOUT (-o)
  else if (operation == CLIENT_REQUEST_GET_STDOUT){
    //ECRITURE
    write_op_taskid_to_pipe(operation, taskid, path_request, path_reply);
   
    //LECTURE
    read_reptype_e_o(path_reply, path_request);
  } //Fin commande STDOUT (-o)


  //Commande REMOVE (-r)
  else if (operation == CLIENT_REQUEST_REMOVE_TASK) {
    //ECRITURE de l'opcode et du taskid
    write_op_taskid_to_pipe(operation, taskid, path_request, path_reply);

    //LECTURE
    fd_reply = open(path_reply, O_RDONLY);
    if (fd_reply == -1) { goto error; }
    //lecture du reptype
    reptype = read_reptype(fd_reply, path_reply, path_request);
    if (reptype == SERVER_REPLY_ERROR) {
      //lecture de l'errcode
      errcode = read_errcode(fd_reply, path_reply, path_request);
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
    write_op_taskid_to_pipe(operation, taskid, path_request, path_reply);
    
    //LECTURE
    read_times_exitcode(path_reply, path_request);
  } //Fin commande TIMES_EXITCODE (-x)
  

  //Option TERMINATE (-q)
  else if(operation == CLIENT_REQUEST_TERMINATE) {
    //ECRITURE
    write_op_to_pipe(operation, path_request, path_reply);
    
    //LECTURE
    fd_reply = open(path_reply, O_RDONLY);
    if (fd_reply == -1) { goto error; }
    //lecture du reptype
    reptype = read_reptype(fd_reply, path_reply, path_request);
    close(fd_reply);
    if (reptype != SERVER_REPLY_OK) { goto error; }
  } //Fin option TERMINATE (-q)


  free(path_request);
  free(path_reply);
  return EXIT_SUCCESS;

  error:
    if (errno != 0) perror("main");
    free(path_request);
    free(path_reply);
    return EXIT_FAILURE;
}
