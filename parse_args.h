struct Args {
  char* source;
  char* destination;
  int recursive;
  int every_seconds;
};

struct Args* parse_args(int argc, char* argv[], struct Args* args);
