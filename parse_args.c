#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse_args.h"

// usage: ./main source destination [-r] [-t time]

const short ONE_SECOND = 1;
const short ONE_MINUTE = 60 * ONE_SECOND;

struct Args* parse_args(int argc, char* argv[], struct Args* args) {
  if (argc < 3) {
    perror("Incorrect number of arguments. Usage: ./main source destination [-r] [-t time]");
    exit(EXIT_FAILURE);
  }

  args->source = argv[1];
  args->destination = argv[2];
  args->recursive = 0;
  args->every_seconds = 5 * ONE_MINUTE;

  for (int i=3; i<argc; i++) {
    char* option = argv[i];
    if (strcmp(option, "-r") == 0) {
      args->recursive = 1;
    } else if (strcmp(option, "-t") == 0) {
      args->every_seconds = atoi(argv[i+1]);
    }
  }

  return args;
};
