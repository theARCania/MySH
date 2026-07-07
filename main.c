#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>


#define MYSH_RL_BUFSIZE 1024
#define _BUFSIZE 64
#define _DELIM " \t\r\n\a"

int MYSH_cd(char **args);
int MYSH_help(char **args);
int MYSH_exit(char **args);
int MYSH_pwd(char **args);

char *builtin_str[] = {
  "cd",
  "help",
  "exit",
  "pwd"
};

int (*builtin_func[]) (char **) = {
  &MYSH_cd,
  &MYSH_help,
  &MYSH_exit,
  &MYSH_pwd
};

int MYSH_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}

int MYSH_cd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "mysh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("mysh");
    }
  }
  return 1;
}

int MYSH_help(char **args)
{
  int i;
  printf("MySH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < MYSH_num_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }

  //printf("Use the man command for information on other programs.\n");
  return 1;
}

int MYSH_exit(char **args)
{
  return 0;
}

int MYSH_pwd(char **args) {
  char cwd[MYSH_RL_BUFSIZE];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    printf("%s\n", cwd);
  } else {
    perror("mysh");
  }

  return 1;
}


int MYSH_launch(char** args) {
  pid_t pid, wpid;
  int status;
  // i/o redirection
  char *in_file = NULL;
  char *out_file = NULL;
  int i = 0;

  while (args[i] != NULL) {
    if (strcmp(args[i], ">") == 0) {
      out_file = args[i + 1];
      args[i] = NULL;
      break;
    }
    if (strcmp(args[i], "<") == 0) {
      in_file = args[i + 1];
      args[i] = NULL;
      break;
    }
    i++;
  }


  pid = fork();

  if (pid == 0) {
    // signal handling
    struct sigaction sa_default;
    sa_default.sa_handler = SIG_DFL;
    sigemptyset(&sa_default.sa_mask);
    sa_default.sa_flags = 0;

    sigaction(SIGINT, &sa_default, NULL);
    sigaction(SIGTSTP, &sa_default, NULL);
    // i/o redirection
    if (in_file != NULL) {
      int fd_in = open(in_file, O_RDONLY);
      if (fd_in == -1) {
        perror("mysh");
        exit(EXIT_FAILURE);
      }
      dup2(fd_in, STDIN_FILENO);
      close(fd_in);
    }
    if (out_file != NULL) {
      int fd_out = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd_out == -1) {
        perror("mysh");
        exit(EXIT_FAILURE);
      }
      dup2(fd_out, STDOUT_FILENO);
      close(fd_out);
    }



    if (execvp(args[0], args) == -1) {
      perror("mysh");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
      perror("mysh");
  } else {
      do {
        wpid = waitpid(pid, &status, WUNTRACED);
      } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
  return 1;
}

int MYSH_execute(char **args)
{
  int i;

  if (args[0] == NULL) {
    return 1;
  }

  for (i = 0; i < MYSH_num_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return MYSH_launch(args);
}

char **MYSH_split_line(char *line)
{
  int bufsize = _BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token;

  if (!tokens) {
    fprintf(stderr, "mysh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, _DELIM);
  while (token != NULL) {
    tokens[position++] = token;
    
    if (position >= bufsize) {
      bufsize  += _BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        fprintf(stderr, "mysh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
    token = strtok(NULL, _DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

char *MYSH_read_line(void)
{
  int bufsize = MYSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "mysh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    c = getchar();
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
        buffer[position] = c;
    }
    position++;

    if (position >= bufsize) {
      bufsize += MYSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "mysh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

void MYSH_loop(void) {
  char *line;
  char **args;
  int status;

  do {
    printf(">> ");
    line = MYSH_read_line();
    args = MYSH_split_line(line);
    status = MYSH_execute(args);
    
    free(line);
    free(args);
  } while (status);
}





// ---------------------
// MAIN LOOP
// ---------------------

void print_welcome() {
    printf("  __  __       _____ _    _ \n");
    printf(" |  \\/  |     / ____| |  | |\n");
    printf(" | \\  / |  _ | (___ | |__| |\n");
    printf(" | |\\/| | | | \\___ \\|  __  |\n");
    printf(" | |  | | |_| |___) | |  | |\n");
    printf(" |_|  |_|\\__, |_____/|_|  |_|\n");
    printf("          __/ |             \n");
    printf("         |___/              \n\n");
}

int main(int argc, char *argv[])
{
  struct sigaction sa_ignore;
  sa_ignore.sa_handler = SIG_IGN;
  sigemptyset(&sa_ignore.sa_mask);
  sa_ignore.sa_flags = 0;
  sigaction(SIGINT, &sa_ignore, NULL);
  sigaction(SIGTSTP, &sa_ignore, NULL);
  print_welcome();
  MYSH_loop();
  return EXIT_SUCCESS;
}
