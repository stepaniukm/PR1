#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/_types/_mode_t.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include "parse_args.h"
#include "read_dir.h"
#include <errno.h>

#define PATH_MAX 300
#define CURRENT_DIRECTORY "."
#define HIGHER_DIRECTORY ".."

void append_to_path_info(struct PathInfo *root, struct PathInfo *appended);

int get_file_or_directory_info(char* path, struct FileOrDirectoryInfo* info) {
  struct stat st;
  lstat(path, &st);

  if (S_ISDIR(st.st_mode)) {
    info->type = DIRECTORY;
  } else if (S_ISREG(st.st_mode)) {
    info->type = REGULAR_FILE;
  } else {
    return 0;
  }

  info->modified_time = st.st_mtime;
  info->size_in_bytes = st.st_size;

  return 1;
}

void prepend(char* s, const char* t)
{
    size_t len = strlen(t);
    memmove(s + len, s, strlen(s) + 1);
    memcpy(s, t, len);
}

int mkdir_p(const char *path) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, S_IRWXU) != 0 && errno != EEXIST) {
                return -1;
            }
            *p = '/';
        }
    }
    if (mkdir(tmp, S_IRWXU) != 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

int open_file_in_nested_dir(const char *path, int flags) {
    char *dir_path = strdup(path);
    if (!dir_path) {
        perror("strdup");
        return -1;
    }

    char *last_slash = strrchr(dir_path, '/');
    if (last_slash != NULL) {
        *last_slash = '\0';
        if (mkdir_p(dir_path) == -1) {
            perror("mkdir_p");
            free(dir_path);
            return -1;
        }
    }

    free(dir_path);
    return open(path, flags);
}

void write_dir(char* source_base, char* dest_base, struct PathInfo* current_path_info) {
  struct PathInfo* current = current_path_info;

  while(current!=NULL) {
    char* dest_path = strdup(current->path);
    dest_path += strlen(source_base);

    prepend(dest_path, dest_base);
    printf("CURRENT PATH: %s\n",  strdup(current->path));
    printf("CURRENT PATH DEST: %s\n", dest_path);

    int source = open_file_in_nested_dir(strdup(current->path), O_RDONLY);

    int dest = open_file_in_nested_dir(dest_path, O_WRONLY | O_CREAT);
    chmod(dest_path, 0x777);

    off_t* bytes_to_send = (off_t*) current->info->size_in_bytes;
    off_t offset = 0;

    if (sendfile(source, dest, offset, bytes_to_send, NULL, 0) == -1) {
        perror("sendfile");
    }


    close(source);
    close(dest);

    current = current->next_path;
  }
}

struct PathInfo* read_dir(char* path, int recursive) {
  struct PathInfo* local_path_info = NULL;

  struct dirent* entry;
  char entry_path[PATH_MAX + 1];
  char* dir_path = strdup(path);
  strcpy(entry_path, dir_path);
  size_t path_len = strlen(dir_path);

  if (entry_path[path_len - 1] != '/') {
      entry_path[path_len] = '/';
      entry_path[path_len + 1] = '\0';
      ++path_len;
  }

  DIR* dir = opendir(dir_path);

  while ((entry = readdir(dir)) != NULL) {
      char* name = strdup(entry->d_name);
      if (strcmp(name, CURRENT_DIRECTORY) == 0 || strcmp(name, HIGHER_DIRECTORY) == 0) {
        continue;
      }
      strncpy(entry_path + path_len, entry->d_name, sizeof (entry_path) - path_len);

      struct FileOrDirectoryInfo* info = (struct FileOrDirectoryInfo*)malloc(sizeof(struct FileOrDirectoryInfo));
      int result = get_file_or_directory_info(entry_path, info);

      if (!result) {
          free(info);
          continue;
      }

      if (info->type == REGULAR_FILE) {
          struct PathInfo* current_path_info = (struct PathInfo*)malloc(sizeof(struct PathInfo));
          current_path_info->path = strdup(entry_path);
          current_path_info->info = info;
          current_path_info->next_path = NULL;

          if (local_path_info == NULL) {
            local_path_info = current_path_info;
          } else {
            append_to_path_info(local_path_info, current_path_info);
          }
      }

      if (recursive == 1 && info->type == DIRECTORY) {
        struct PathInfo* inner_path_info = read_dir(strdup(entry_path), recursive);
        if (local_path_info == NULL) {
          local_path_info = inner_path_info;
        } else {
          append_to_path_info(local_path_info, inner_path_info);
        }
      }
  }

  closedir(dir);

  return local_path_info;
}

void print_path_info(struct PathInfo *path_info) {
    if (path_info == NULL) {
        return;
    }

    struct PathInfo *current = path_info;

    while (current != NULL) {
        printf("-------------------------\n");
        printf("Path: %s\n", current->path);
        printf("Modified Time: %ld\n", current->info->modified_time);
        printf("Size in Bytes: %ld\n", current->info->size_in_bytes);
        printf("-------------------------\n");
        current = current->next_path;
    }
}

void append_to_path_info(struct PathInfo *root, struct PathInfo *appended) {
    if (root == NULL || appended == NULL) {
        return;
    }

    struct PathInfo *current = root;
    while (current->next_path != NULL) {
        current = current->next_path;
    }

    current->next_path = appended;
}

struct PathInfo* changed_files(struct PathInfo* old_path_info, struct PathInfo* new_path_info) {
    struct PathInfo* changed_files = NULL;

    struct PathInfo* old_current = old_path_info;
    while (old_current != NULL) {
      struct PathInfo* new_current = new_path_info;
      while (new_current != NULL) {
        if (strcmp(old_current->path, new_current->path) == 0) {
          if (old_current->info->modified_time != new_current->info->modified_time || old_current->info->size_in_bytes != new_current->info->size_in_bytes) {
            struct PathInfo* current_path_info = (struct PathInfo*)malloc(sizeof(struct PathInfo));
            current_path_info->path = strdup(new_current->path);
            current_path_info->info = new_current->info;
            current_path_info->next_path = NULL;

            if (changed_files == NULL) {
              changed_files = current_path_info;
            } else {
              append_to_path_info(changed_files, current_path_info);
            }
          }
        }
        new_current = new_current->next_path;
      }
      old_current = old_current->next_path;
    }

    return changed_files;
}

struct PathInfo* added_files(struct PathInfo* old_path_info, struct PathInfo* new_path_info) {
    struct PathInfo* added_files = NULL;

    struct PathInfo* new_current = new_path_info;
    while (new_current != NULL) {
      struct PathInfo* old_current = old_path_info;
      int found = 0;
      while (old_current != NULL) {
        if (strcmp(old_current->path, new_current->path) == 0) {
          found = 1;
          break;
        }
        old_current = old_current->next_path;
      }

      if (!found) {
        struct PathInfo* current_path_info = (struct PathInfo*)malloc(sizeof(struct PathInfo));
        current_path_info->path = strdup(new_current->path);
        current_path_info->info = new_current->info;
        current_path_info->next_path = NULL;

        if (added_files == NULL) {
          added_files = current_path_info;
        } else {
          append_to_path_info(added_files, current_path_info);
        }
      }

      new_current = new_current->next_path;
    }

    return added_files;
}

struct PathInfo* deleted_files(struct PathInfo* old_path_info, struct PathInfo* new_path_info) {
  // Deleted files are the files that are in the old path info but not in the new path info
  struct PathInfo* deleted_files = NULL;

  struct PathInfo* old_current = old_path_info;

  while (old_current != NULL) {
    struct PathInfo* new_current = new_path_info;
    int found = 0;
    while (new_current != NULL) {
      if (strcmp(old_current->path, new_current->path) == 0) {
        found = 1;
        break;
      }
      new_current = new_current->next_path;
    }

    if (!found) {
      struct PathInfo* current_path_info = (struct PathInfo*)malloc(sizeof(struct PathInfo));
      current_path_info->path = strdup(old_current->path);
      current_path_info->info = old_current->info;
      current_path_info->next_path = NULL;

      if (deleted_files == NULL) {
        deleted_files = current_path_info;
      } else {
        append_to_path_info(deleted_files, current_path_info);
      }
    }

    old_current = old_current->next_path;
  }

  return deleted_files;
}