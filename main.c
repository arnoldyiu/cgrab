#include <ctype.h>
#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#define MH_MAGIC_32 "\xce\xfa\xed\xfe"
#define MH_MAGIC_64 "\xcf\xfa\xed\xfe"

char *lstrip(const char *str) {
  if (str == NULL)
    return NULL;
  while (*str && isspace((unsigned char)*str))
    str++;
  return (char *)str;
}

int has_prefix(const char *a, const char *b) {
  // ! strncmp(): compare the first n characters of strings a and b
  return strncmp(a, b, strlen(b)) == 0;
}

static const char *basename_of(const char *path) {
  // ! strrchr() to find the last occurrence of a character
  // ! strchr() for first occurence
  const char *slash = strrchr(path, '/');
  return slash ? slash + 1 : path;
}

int starts_with_period(const char *path) {
  const char *name = basename_of(path);

  if (name[0] == '.')
    return 1;
  return 0;
}

int is_binary_executable(FILE *fp) {
  unsigned char header_bytes[4];

  // ! fread(<memory block to store read data>, <no. of bytes/element>, <no. of elements>, <pointer to FILE object>)
  // !! basically move a "cursor" (up to) 4 bytes to the right, once
  // !! and copy said 4 bytes into header_bytes
  size_t header_bytes_read = fread(header_bytes, 1, 4, fp);

  // ! rewind cursor back to the start; clear EOF/error flags
  rewind(fp);

  if (header_bytes_read < 4)
    return 0;
  if (header_bytes[0] == 0x7f && header_bytes[1] == 'E' &&
      header_bytes[2] == 'L' && header_bytes[3] == 'F') // Linux/ELF
    return 1;
  // ! memcmp(<pointer to block A>, <pointer to block B>, <no. of bytes for comparison>)
  // !! C handles the * for functions like this
  if (memcmp(header_bytes, MH_MAGIC_32, 4) == 0 ||
      memcmp(header_bytes, MH_MAGIC_64, 4) == 0) // macOS
    return 1;
  return 0;
}

typedef struct CommentStyle {
  const char *comment_marker;
  const char **exts;
} CommentStyle;

// ! NULL to terminate arrays because C array lengths are not stored
static const char *hash_exts[] = {".py",   ".rb",   ".pl",  ".r",
                                  ".sh",   ".bash", ".zsh", ".yml",
                                  ".yaml", ".toml", NULL};
static const char *slash_exts[] = {".c",    ".h",   ".cpp",   ".hpp",   ".cc",
                                   ".cxx",  ".js",  ".jsx",   ".ts",    ".tsx",
                                   ".java", ".go",  ".rs",    ".swift", ".kt",
                                   ".cs",   ".php", ".scala", NULL};

// ! {key, value} kind of like a Python list of tuples?
static const CommentStyle styles[] = {
    {"# !", hash_exts},
    {"// !", slash_exts},
};

// ! sizeof <total array size in bytes> / sizeof <size of one item in bytes>
static const size_t n_styles = sizeof styles / sizeof styles[0];

static const char *comment_prefix_for(const char *path) {
  const char *dot = strrchr(basename_of(path), '.');
  if (!dot)
    return NULL;
  for (size_t s = 0; s < n_styles; s++)
    // ! ** pointer to a pointer
    for (const char **ext = styles[s].exts; *ext; ext++)
      if (strcmp(dot, *ext) == 0)
        return styles[s].comment_marker;
  return NULL;
}

void cat_comment_helper(const char *path) {
  const char *prefix = comment_prefix_for(path);
  if (!prefix)
    return;

  FILE *fp;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  fp = fopen(path, "r");
  if (!fp) {
    perror(path);
    return;
  }
  if (is_binary_executable(fp)) {
    fclose(fp);
    return;
  }

  const char *name = path;
  if (name[0] == '.' && name[1] == '/')
    name += 2;

  printf("\n====== %s ======\n", name);

  size_t current_line = 1;
  while ((read = getline(&line, &len, fp)) != -1) {
    const char *stripped_line = lstrip(line);
    if (has_prefix(stripped_line, prefix))
      printf("[%zu] %s", current_line, stripped_line);
    current_line++;
  }
  printf("\n");
  fclose(fp);
}

void cat_comments(const char *path) {
  if (strcmp(path, ".") != 0 && starts_with_period(path))
    return;

  struct stat path_stat;
  if (lstat(path, &path_stat)) {
    perror(path);
    return;
  }

  if (!S_ISDIR(path_stat.st_mode)) {
    cat_comment_helper(path);
    return;
  }

  DIR *dir = opendir(path);
  if (!dir) {
    perror(path);
    return;
  }

  // recursion over subfolders
  struct dirent *dirpath;
  while ((dirpath = readdir(dir)) != NULL) {
    // ! (*dirpath).d_name
    const char *name = dirpath->d_name;
    if (starts_with_period(name))
      continue;

    size_t len = strlen(path) + strlen(name) + 2;
    char sub_path[len];

    // ! snprintf: string buffer
    snprintf(sub_path, len, "%s/%s", path, name);

    if (dirpath->d_type == DT_DIR)
      cat_comments(sub_path);
    else
      cat_comment_helper(sub_path);
  }
  closedir(dir);
}

int main(int argc, char *argv[]) {
  if (argc == 1)
    cat_comments(".");

  for (int i = 1; i < argc; i++) {
    const char *path = argv[i];
    cat_comments(path);
  }
  return 0;
}