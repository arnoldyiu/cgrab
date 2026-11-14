#include <ctype.h>
#include <dirent.h>
#include <errno.h>
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

// set whenever a path can't be read, so main() can exit with an error code
static int path_error = 0;

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
  size_t n_comments = 0;
  const char *prefix = comment_prefix_for(path);
  if (!prefix)
    return;

  FILE *fp = fopen(path, "r");
  if (!fp) {
    perror(path);
    path_error = 1;
    return;
  }

  const char *name = path;
  if (name[0] == '.' && name[1] == '/')
    name += 2;

  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  size_t current_line = 1;
  while ((read = getline(&line, &len, fp)) != -1) {
    const char *stripped_line = lstrip(line);
    if (has_prefix(stripped_line, prefix)) {
      // only include files with valid comments
      if (n_comments == 0) {
        if (written_files > 0) {
          fprintf(output_file, "\n");
        }
        written_files++;

        if (markdown_output)
          fprintf(output_file, "### %s\n```\n", name);
        else
          fprintf(output_file, "====== %s ======\n", name);
      }

      // don't allow files to end with '\n'
      const char *newline = line[read - 1] == '\n' ? "" : "\n";
      fprintf(output_file, "[%zu] %s%s", current_line, stripped_line, newline);
      n_comments++;
    }
    current_line++;
  }

  if (n_comments > 0) {
    if (markdown_output)
      fprintf(output_file, "```\n");
    printf("Extracted %zu comment%s: %s\n", n_comments, n_comments == 1 ? "" : "s", name);
  } else {
    printf("No comments found: %s\n", name);
  }

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
    path_error = 1;
    return;
  }

  if (!S_ISDIR(path_stat.st_mode)) {
    struct stat target_stat;
    if (stat(input_path, &target_stat) == 0 && S_ISREG(target_stat.st_mode))
      cat_comment_helper(input_path, output_file);
    return;
  }

  // ! scandir(): store file/folder names in a sorted array
  // !! entries is an array of pointers automatically allocated by scandir
  struct dirent **entries;
  int n = scandir(input_path, &entries, NULL, alphasort);
  if (n < 0) {
    perror(input_path);
    path_error = 1;
    return;
  }

  // avoid "test//file.py" if the path is inputted as "test/"
  const char *sep = input_path[strlen(input_path) - 1] == '/' ? "" : "/";

  // recursion over subfolders
  for (int i = 0; i < n; i++) {
    // ! (*entries[i]).d_name
    const char *name = entries[i]->d_name;
    if (!starts_with_period(name)) {
      size_t len = strlen(input_path) + strlen(sep) + strlen(name) + 1;
      char sub_path[len];

      // ! snprintf: string buffer
      snprintf(sub_path, len, "%s%s%s", input_path, sep, name);

      // cat_comments checks for itself whether sub_path is a file or folder
      cat_comments(sub_path, output_file);
    }
    // free each entry BEFORE freeing the array itself
    free(entries[i]);
  }
  free(entries);
}

int main(int argc, char *argv[]) {
  char input[256];
  char *output_path;
  const char *out_ext;
  FILE *output_file;

  // keep asking until a valid file name is given and the file is created
  while (1) {
    printf("Save to (.txt OR .md): ");
    fflush(stdout);

    // ! fgets(): read one line (including '\n'), NULL on EOF (Ctrl+D)
    if (fgets(input, sizeof(input), stdin) == NULL) {
      printf("\nError: no file name given\n");
      return 1;
    }

    // ! strcspn(): index of the first '\n'
    size_t len = strcspn(input, "\n");
    // ! feof(): is it file EOF?
    if (input[len] != '\n' && !feof(stdin)) {
      int c;
      while ((c = getchar()) != '\n' && c != EOF)
        ; // discard the rest of the line
      printf("Error: file name cannot exceed %zu characters\n", sizeof(input) - 2);
      continue;
    }
    input[len] = '\0';

    output_path = lstrip(input);
    char *end = output_path + strlen(output_path);
    while (end > output_path && isspace((unsigned char)end[-1]))
      // decrement before using end variable
      *--end = '\0';

    if (*output_path == '\0') {
      printf("Please provide a file name\n");
      continue;
    }

    out_ext = strrchr(basename_of(output_path), '.');

    // check strrchr returning NULL if there's no '.'
    // ! strcasecmp(): 0 = match
    if (!out_ext || (strcasecmp(out_ext, ".txt") != 0 &&
                     strcasecmp(out_ext, ".md") != 0)) {
      printf("Error: Invalid file extension\n");
      continue;
    }

    // ! "wx": create for writing only if the target file doesn't already exist (C11)
    output_file = fopen(output_path, "wx");
    if (!output_file) {
      if (errno == EEXIST)
        printf("Error: Output file '%s' already exists\n", output_path);
      else
        perror(output_path);
      continue;
    }

    break; // valid input
  }

  markdown_output = strcasecmp(out_ext, ".md") == 0;

  printf("\n");
  if (argc == 1)
    cat_comments(".", output_file);

  for (int i = 1; i < argc; i++) {
    const char *path = argv[i];
    cat_comments(path, output_file);
  }

  fclose(output_file);
  printf("================\n");

  if (written_files == 0) {
    remove(output_path);
    printf("No comments found. Output file was not saved.\n");
  } else {
    printf("Successfully saved output to %s\n", output_path);
  }

  if (path_error) {
    printf("Finished with errors (see above)\n");
    return 1;
  }
  return 0;
}
