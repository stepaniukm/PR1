#include <signal.h>
#include <sys/signal.h>
#include <sys/syslog.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "parse_args.h"
#include "read_dir.h"

#define PATH_MAX 100
#define REGULAR_FILE "regular file"
#define DIRECTORY "directory"
#define CURRENT_DIRECTORY "."
#define HIGHER_DIRECTORY ".."

volatile sig_atomic_t keep_going = 1;
volatile sig_atomic_t should_wake = 0;

struct PathInfo* whole_path_info;

void sigint_handler(int sig) {
  printf("\nSIGINT\n");
  keep_going = 0;
}

void sigusr1_handler(int sig) {
  printf("\nSIGUSR1\n");
  should_wake = 1;
}

void sigusr2_handler(int sig) {
  printf("\nSIGUSR2\n");
  print_path_info(whole_path_info);
}

int main(int argc, char *argv[]) {
  struct Args *args = malloc(sizeof(struct Args));
  parse_args(argc, argv, args);

  openlog("ProjectWatcher", LOG_CONS | LOG_PID, LOG_DAEMON);
  syslog(LOG_ERR, "Project watcher started!");
  pid_t pid, sid;
  pid = fork();
  if (pid < 0) {
    syslog(LOG_ERR, "Could not fork!");
    exit(EXIT_FAILURE);
  }

  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  umask(0);

  sid = setsid();
  if (sid < 0) {
    syslog(LOG_ERR, "Couldn't create sid for child");
    exit(EXIT_FAILURE);
  }

  // if ((chdir("/")) < 0) {
  //   syslog(LOG_ERR, "Couldn't change directory to /");
  //   exit(EXIT_FAILURE);
  // }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  signal(SIGINT, sigint_handler);
  signal(SIGUSR1, sigusr1_handler);
  signal(SIGUSR2, sigusr2_handler);

  while (keep_going == 1) {
    struct PathInfo* previous_path_info = whole_path_info;

    whole_path_info = read_dir(args->source, args->destination,  args->recursive);
    struct PathInfo* changed = changed_files(previous_path_info, whole_path_info);
    struct PathInfo* added = diff_files(previous_path_info, whole_path_info);
    struct PathInfo* deleted = diff_files(whole_path_info, previous_path_info);
    syslog(LOG_INFO, "READ DIR");

    if (changed != NULL) {
      syslog(LOG_INFO, "Changed files:\n");
      print_path_info(changed);
      write_dir(changed);
    }

    if (added != NULL) {
      syslog(LOG_INFO, "Added files:\n");
      print_path_info(added);
    }

    if (deleted != NULL) {
      syslog(LOG_INFO, "Deleted files:\n");
      print_path_info(deleted);
    }


    for (int i=0; i<args->every_seconds; i++) {
      if (should_wake == 1 || keep_going == 0) {
        break;
      }
      sleep(1);
    }
    should_wake = 0;
  }

  closelog();
  exit(EXIT_SUCCESS);
}
