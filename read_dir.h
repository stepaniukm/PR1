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
  struct FileOrDirectoryInfo* info;
  struct PathInfo* next_path;
} PathInfo;

struct PathInfo* read_dir(char* path, int recursive);
void write_dir(char* source_base, char* dest_base, struct PathInfo* current_path_info);
int get_file_or_directory_info(char* path, struct FileOrDirectoryInfo* info);
void print_path_info(struct PathInfo* path_info);
void free_path_info(struct PathInfo *path_info);
struct PathInfo* changed_files(struct PathInfo* old_path_info, struct PathInfo* new_path_info);
struct PathInfo* added_files(struct PathInfo* old_path_info, struct PathInfo* new_path_info);
struct PathInfo* deleted_files(struct PathInfo* old_path_info, struct PathInfo* new_path_info);
