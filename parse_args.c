#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <unistd.h>
#include "parse_args.h"

// usage: ./main source destination [[-r] [-t time]]

const short ONE_SECOND = 1;
const short ONE_MINUTE = 60 * ONE_SECOND;

struct Args* parse_args(int argc, char* argv[], struct Args* args) {
  char cwr[100];
  getcwd(cwr, sizeof(cwr));

  if (argc < 3) {
    syslog(LOG_ERR, "Incorrect number of arguments. Usage: ./main /source/ /destination/ [-r] [-t time]");
    exit(EXIT_FAILURE);
  }

  args->source = strdup(argv[1]);
  args->destination = strdup(argv[2]);

  char* temp = strdup(cwr);
  strcat(temp, args->source);
  args->source = temp;

  char* temp2 = strdup(cwr);
  strcat(temp2, args->destination);
  args->destination = temp2;

  args->recursive = 0;
  args->every_seconds = 5 * ONE_MINUTE;

  for (int i=3; i<argc; i++) {
    char* option = argv[i];
    if (strcmp(option, "-r") == 0) {
      args->recursive = 1;
    } else if (strcmp(option, "-t") == 0) {
      args->every_seconds = atoi(argv[i+1]);
      if (args->every_seconds <= 0) {
        syslog(LOG_WARNING, "Incorrect number provided as time. Default value of 5 minutes will be used.");
        args->every_seconds = 5 * ONE_MINUTE;
      }
    }
  }

  return args;
};
