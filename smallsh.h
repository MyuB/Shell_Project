#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/types.h>
#include <errno.h>
#define EOL		1
#define ARG		2
#define AMPERSAND	3
#define SEMICOLON	4

#define MAXARG		512
#define MAXBUF		512
#define FOREGROUND	0
#define	BACKGROUND	1

int userin(char *p);
void procline();
int gettok(char** outptr);
int inarg(char c);
int runcommand(char **cline, int where);
char *user_homedir;
void get_homedir();
void cd_implementation(char **args);
int check_redir(char **commands);
void child_handler();
int check_pipe(char ** args);
int args_grp(char *s, char **argvp, char* delimit);
void exec_cmd(char *cmd);
