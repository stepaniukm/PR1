enum FileType {
  REGULAR_FILE,
  DIRECTORY
};

struct FileOrDirectoryInfo {
  long modified_time;
  long size_in_bytes;
  enum FileType type;
} FileInfo;

struct PathInfo {
  char* path;
  char* dest_path;
  struct FileOrDirectoryInfo* info;
  struct PathInfo* next_path;
} PathInfo;

struct PathInfo* read_dir(char* path, char* dest_path, int recursive);
void write_dir(struct PathInfo* current_path_info);
void delete_dir(struct PathInfo* deleted_path_info);
int get_file_or_directory_info(char* path, struct FileOrDirectoryInfo* info);
void print_path_info(struct PathInfo* path_info);
void free_path_info(struct PathInfo *path_info);
struct PathInfo* changed_files(struct PathInfo* old_path_info, struct PathInfo* new_path_info);
struct PathInfo* diff_files(struct PathInfo* old_path_info, struct PathInfo* new_path_info);
