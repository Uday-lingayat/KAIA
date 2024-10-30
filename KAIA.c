#include <stdio.h>  // for printf fprintf perror
#include <string.h> //for strcmp strtok strcat
#include <stdlib.h> // malloc realloc free
#include <stdbool.h>
#include <process.h> //for _spawn functions in  Windows
#include <windows.h> // For WaitForSingleObject for  using CreateProcess
#include <unistd.h>  //for chdir and getcwd
#include <direct.h>  //for _getcwd
#include <errno.h>   //for error handling
#include <limits.h>  //for MAX_PATH
#define getcwd _getcwd
#define TOKEN_BUFFER_SIZE 64 
#define TOKEN_DELIMITER "\r \a \n \t \e"

int KAIA_cd(char **argument);
int KAIA_help(char **argument);
int KAIA_exit(char **argument)
{
  return 0;
}
int KAIA_echo(char **argument);
int KAIA_clear(char **argument);
int KAIA_cwd(char **argument);
// array to store all the builtin function's commands
char *builtin_str[] = {
    "cd",
    "help",
    "exit",
    "echo",
    "clear",
    "cwd"};

int (*builtin_func[])(char **) = {
    &KAIA_cd,
    &KAIA_help,
    &KAIA_exit,
    &KAIA_echo,
    &KAIA_clear,
    &KAIA_cwd};

// number of number built in functions
int KAIA_num_builtins()
{
  return sizeof(builtin_str) / sizeof(char *);
}

// function to get current working directory
int KAIA_cwd(char **argument)
{
  char *cwd;
  char *temp;
  size_t size = MAX_PATH; // Windows maximum path length
  cwd = (char *)malloc(size);

  // Ensure memory allocation was successful
  if (cwd == NULL)
  {
    fprintf(stderr, "KAIA: Allocation error\n");
    return 1;
  }

  // Attempt to get the current working directory
  while (_getcwd(cwd, size) == NULL)
  {
    if (errno == ERANGE)
    {
      size *= 2; // Double the buffer size
      char *temp = (char *)realloc(cwd, size);
      if (temp == NULL)
      {
        free(cwd);
        fprintf(stderr, "KAIA: Memory allocation failed\n");
        return 1;
      }
    }
    else
    {
      perror("KAIA: _getcwd error"); // Handle errors other than ERANGE
      free(cwd);
      return 1;
    }
    perror("KAIA: _getcwd error");
    cwd = temp;
  }

  // Print the current working directory
  printf("%s\n", cwd);
  free(cwd);
  return 1;
}

// function to change current working directory
int KAIA_cd(char **argument)
{
  char *home = NULL;
#ifdef _WIN32
  home = getenv("USERPROFILE");
  if (home == NULL)
  {
    char *drive = getenv("HOMEDRIVE");
    char *path = getenv("HOMEPATH");
    if (drive != NULL && path != NULL)
    {
      static char win_home[512];
      snprintf(win_home, sizeof(win_home), "%s%s", drive, path);
      home = win_home;
    }
  }
#else

#endif

  if (argument[1] == NULL)
  {
    // No directory specified; use home directory if available
    if (home != NULL && chdir(home) == 0)
    {
      printf("Changed to home directory: %s\n", home);
    }
    else
    {
      fprintf(stderr, "KAIA: Could not find or change to home directory.\n");
    }
  }
  else
  {
    // Attempt to change to the specified directory
    if (chdir(argument[1]) != 0)
    {
      perror("KAIA");
    }
  }

  return 1;
}

// function to clear the terminal screen
int KAIA_clear(char **argument)
{
  system("cls");
  return 1;
}

// function to print all tha available builtin functions
int KAIA_help(char **argument)
{
  int i;
  printf("UDAY LINGAYAT's KAIA\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");

  for (i = 0; i < KAIA_num_builtins(); i++)
  {
    printf("  %s\n", builtin_str[i]);
  }
  return 1;
}

// echo function to print
int KAIA_echo(char **argument)
{

  if (argument[1] == NULL)
  {
    printf("No arguments provided after echo.\n"); // printing  if no argument after echo
    return 1;
  }
  else
  {
    // printf("Arguments received:\n"); debugging line
    for (int i = 1; argument[i] != NULL; i++)
    {
      printf("%s", argument[i]); // Print each argument
      if (argument[i + 1] != NULL)
      {
        printf(" ");
      }
    }
    printf("\n");
    return 1;
  }
}

// procedural functions below
// reading the commands user entered and storing in array "line"
char *KAIA_read_line()
{
  char *line = NULL;       // pointer to hold the command that will KAIA
  size_t BufferLength = 0; // variable to store the length of the command
  int check;
  check = getline(&line, &BufferLength, stdin);

  if (check == -1)
  {
    if (feof(stdin))
    {
      exit(EXIT_SUCCESS);
    }
    else
    {
      perror("readline");
      exit(EXIT_FAILURE);
    }
  }

  return line;
}

// storing the commands and agruments in array of pointers
char **KAIA_split_line(char *line)
{
  int buffer_size = TOKEN_BUFFER_SIZE, index = 0;

  char **tokens_array = malloc(buffer_size * sizeof(char *));
  char *token;

  // checking for error from malloc size
  if (tokens_array == NULL)
  {
    fprintf(stderr, "KAIA: allocation error\n");
    exit(EXIT_FAILURE);
  }

  // tokenizing the line

  token = strtok(line, TOKEN_DELIMITER);
  while (token != NULL)
  {
    tokens_array[index] = token;
    index++;

    if (index >= buffer_size)
    {
      // resizing the token_Array if number of tokens exceeds inbuilt limit
      buffer_size += TOKEN_BUFFER_SIZE;
      tokens_array = realloc(tokens_array, buffer_size * sizeof(char *));
      if (tokens_array == NULL)
      {
        fprintf(stderr, "KAIA: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
    token = strtok(NULL, TOKEN_DELIMITER);
  }
  tokens_array[index] = NULL;
  return tokens_array;
}

// deciding the status and storing the return value in will
//this function is something I myself am trying to figure out completely 
//but only understand it partially 
int KAIA_launch(char **argument)
{
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));

  // Convert argument array into a single command line string
  char cmd[1024] = {0};
  int i = 0;
  while (argument[i] != NULL)
  {
    strcat(cmd, argument[i]);
    strcat(cmd, " ");
    i++;
  }

  // Attempt to create a new process to run the command
  if (!CreateProcess(
          NULL,  // No module name (use command line)
          cmd,   // Command line
          NULL,  // Process handle not inheritable
          NULL,  // Thread handle not inheritable
          FALSE, // Set handle inheritance to FALSE
          0,     // No creation flags
          NULL,  // Use parent's environment block
          NULL,  // Use parent's starting directory
          &si,   // Pointer to STARTUPINFO structure
          &pi))  // Pointer to PROCESS_INFORMATION structure
  {
    // If CreateProcess fails, print an error
    printf("KAIA: command not found\n");
    return 1;
  }

  // Wait until child process exits
  WaitForSingleObject(pi.hProcess, INFINITE);

  // Close process and thread handles
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return 1;
}

// function that would call the required builtin function
int KAIA_execute(char **argument)
{
  int i;

  if (argument[0] == NULL)
  {
    // An empty command was entered.
    printf("Empty command entered.\n");
    return 1;
  }

  for (i = 0; i < KAIA_num_builtins(); i++)
  {
    if (strcmp(argument[0], builtin_str[i]) == 0)
    {
      return (*builtin_func[i])(argument);
    }
  }

  return KAIA_launch(argument);
}

// the main looping function
void KAIA_loop()
{

  char *line;
  char **argument;
  int will;

  do
  {
    // KAIA_cwd(NULL);
    printf(">$");
    line = KAIA_read_line();
    argument = KAIA_split_line(line);
    will = KAIA_execute(argument);

    free(line);
    free(argument);
  } while (will);
}

int main()
{

  // Wait for user input
  KAIA_loop();
  return 0;
}