#include <ctype.h>
#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

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

// set in main() when the output file ends in .md
static int markdown_output = 0;

// how many files have had comments written so far; used to put a blank line
// between files without leaving an extra one at the end of the output
static int written_files = 0;

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

void cat_comment_helper(const char *path, FILE *output_file) {
  const char *prefix = comment_prefix_for(path);
  if (!prefix)
    return;

  FILE *fp = fopen(path, "r");
  if (!fp) {
    perror(path);
    return;
  }

  const char *name = path;
  if (name[0] == '.' && name[1] == '/')
    name += 2;

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  size_t current_line = 1;
  int found = 0;
  while ((read = getline(&line, &len, fp)) != -1) {
    const char *stripped_line = lstrip(line);
    if (has_prefix(stripped_line, prefix)) {
      // only include files with valid comments
      if (!found) {
        if (written_files > 0) {
          printf("\n");
          fprintf(output_file, "\n");
        }
        written_files++;

        printf("====== %s ======\n", name);
        if (markdown_output)
          fprintf(output_file, "### %s\n```\n", name);
        else
          fprintf(output_file, "====== %s ======\n", name);
        found = 1;
      }

      // don't allow files to end with '\n'
      const char *newline = line[read - 1] == '\n' ? "" : "\n";
      printf("[%zu] %s%s", current_line, stripped_line, newline);
      fprintf(output_file, "[%zu] %s%s", current_line, stripped_line, newline);
    }
    current_line++;
  }

  if (found && markdown_output)
    fprintf(output_file, "```\n");

  free(line);
  fclose(fp);
}

void cat_comments(const char *input_path, FILE *output_file) {
  // skip hidden files/folders but allow "." and ".." (e.g. cgrab ../..)
  const char *base = basename_of(input_path);
  if (starts_with_period(base) && strcmp(base, ".") != 0 &&
      strcmp(base, "..") != 0)
    return;

  struct stat path_stat;
  if (lstat(input_path, &path_stat)) {
    perror(input_path);
    return;
  }

  if (!S_ISDIR(path_stat.st_mode)) {
    cat_comment_helper(input_path, output_file);
    return;
  }

  // ! scandir(): store file/folder names in a sorted array
  // !! entries is an array of pointers automatically allocated by scandir
  struct dirent **entries;
  int n = scandir(input_path, &entries, NULL, alphasort);
  if (n < 0) {
    perror(input_path);
    return;
  }

  // recursion over subfolders
  for (int i = 0; i < n; i++) {
    // ! (*entries[i]).d_name
    const char *name = entries[i]->d_name;
    if (!starts_with_period(name)) {
      size_t len = strlen(input_path) + strlen(name) + 2;
      char sub_path[len];

      // ! snprintf: string buffer
      snprintf(sub_path, len, "%s/%s", input_path, name);

      // cat_comments checks for itself whether sub_path is a file or folder
      cat_comments(sub_path, output_file);
    }
    // free each entry BEFORE freeing the array itself
    free(entries[i]);
  }
  free(entries);
}

int main(int argc, char *argv[]) {
  char output_path[256];
  printf("Save to (.txt OR .md): ");

  // ! scanf(): write stdin to variable
  if (scanf("%255s", output_path) != 1) {
    printf("Error: no file name given\n");
    return 1;
  }

  // ! scanf stops after 255 chars
  int next = getchar();
  if (next != EOF && !isspace(next)) {
    printf("Error: file name cannot exceed 255 characters\n");
    return 1;
  }
  
  const char *out_ext = strrchr(basename_of(output_path), '.');

  // check strrchr returning NULL if there's no '.'
  // ! strcasecmp: 0 = match
  if (!out_ext || (strcasecmp(out_ext, ".txt") != 0 && strcasecmp(out_ext, ".md") != 0)) {
    printf("Error: Invalid file extension\n");
    return 1;
  }

  FILE *output_file = fopen(output_path, "r");
  if (output_file) {
    printf("Error: Output file '%s' already exists\n", output_path);
    fclose(output_file);
    return 1;
  }

  // ! reopen in append mode
  output_file = fopen(output_path, "a");
  if (!output_file) {
    perror(output_path);
    return 1;
  }

  markdown_output = strcasecmp(out_ext, ".md") == 0;

  printf("\n");
  if (argc == 1)
    cat_comments(".", output_file);

  for (int i = 1; i < argc; i++) {
    const char *path = argv[i];
    cat_comments(path, output_file);
  }

  printf("\n================\n");
  printf("Successfully saved to %s\n", output_path);

  fclose(output_file);
  return 0;
}