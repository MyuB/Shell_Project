#include "smallsh.h"

static char inpbuf[MAXBUF];
static char tokbuf[2*MAXBUF];
static char *ptr = inpbuf;
static char *tok = tokbuf;

static char special[] = {' ', '\t', '&', ';', '\n', '\0' };

extern char *user_homedir;
int narg;
int homedir_len;
char* cmdgrp[MAXBUF];
char* cmdpipegrp[MAXBUF];


void child_handler() {
	waitpid(-1, NULL, WNOHANG);
}

void get_homedir() {
	struct passwd *pwd;
    	char   buf[1024];
    	size_t bufsize;
    	int    ret;

    	errno = 0;
    
    	if((pwd = getpwuid(getuid())) == NULL) {
        	if(errno == 0 || errno == ENOENT || errno == ESRCH || errno == EBADF || errno == EPERM) {
            		perror("getpwuid");
            		return;
        	} 
		else {
            		printf("error: %s\n", "something wrong");
            		return;
        	}
    	}

	user_homedir = pwd->pw_dir;
	homedir_len = strlen(user_homedir);
}

int userin(char *p) {
	int c, count;
	ptr = inpbuf;
	tok = tokbuf;

	char *cwd;
	cwd = getcwd(NULL, MAXBUF);
	int hdir_len = strlen(user_homedir);
	char temp[MAXBUF] = "~";
	if (strstr(cwd, user_homedir) != NULL) {
		for (int i = homedir_len; i < strlen(cwd); i++) 
			temp[i - homedir_len + 1] = cwd[i];
		
		//strcat(temp, temp_cwd);
		p = temp;
	}
	else p = cwd;

	printf("%s$ ", p);
	count = 0;

	while(1) {
		if ((c = getchar()) == EOF)
			return EOF;
		if (count < MAXBUF)
			inpbuf[count++] = c;
		if (c == '\n' && count < MAXBUF) {
			inpbuf[count] = '\0';
			return count;
		}
		if (c == '\n' || count >= MAXBUF) {
			printf("smallsh: input line too long\n");
			count = 0;
			printf("%s", p);
		}
	}
}

int gettok(char** outptr) {
	int type;
	*outptr = tok;
	while(*ptr == ' ' || *ptr == '\t')
		ptr++;

	*tok++ = *ptr;
	switch(*ptr++) {
		case '\n':
			type = EOL;
			break;
		case '&':
			type = AMPERSAND;
			break;
		case ';':
			type = SEMICOLON;
			break;
		default:
			type = ARG;
			while(inarg(*ptr))
				*tok++ = *ptr++;
	}
	*tok++ = '\0';
	return type;
}

int inarg(char c) {
	char *wrk;

	for (wrk = special; *wrk; wrk++) {
		if (c == *wrk)
			return 0;
	}
	return 1;
}

void procline() {
	char *arg[MAXARG + 1];
	int toktype, type;
	narg = 0;
	for (;;) {
		switch (toktype = gettok(&arg[narg])) {
			case ARG:
				if (narg < MAXARG) 
					narg++;
				break;
			case EOL:
			case SEMICOLON:
			case AMPERSAND:
				if (toktype == AMPERSAND) 	type = BACKGROUND;
				else 				type = FOREGROUND;
				if (narg != 0) {
					arg[narg] = NULL;
					if (strcmp(arg[0], "exit") == 0) exit(0);
					runcommand(arg, type);
				}
				if (toktype == EOL) return;
				narg = 0;
				break;
		}
	}
}

int runcommand(char **cline, int where) {
	pid_t pid;
	int status;
	int idx = -1;
	int redir = check_redir(cline);
	if (strcmp("exit", cline[0]) == 0) exit(0);
	if (strcmp("cd", cline[0]) == 0) {
		cd_implementation(cline);
		return 0;
	}
	for (int i = 0; i < narg; i++) printf("%s \t", cline[i]);
	printf("\n");
	switch(pid = fork()) {
		case -1:
			perror("smallsh");
			return -1;
		case 0:
			;
			if (where == FOREGROUND) signal(SIGINT, SIG_DFL);
			int idx = -1;
			int redir = check_redir(cline);
			int pipe_check = check_pipe(cline);
			if (pipe_check) {
				//now i have to delimit by command groups
				int a = args_grp(inpbuf, cmdpipegrp, "|");
				int pfd[2];
				int i, status;
				if (pipe(pfd) == -1) perror("pipe wrong");
				
				char *right_cmd[MAXARG + 1];
				for (int i = pipe_check; i < MAXARG + 1; i++) {
					if (cline[i] == NULL) {
						right_cmd[i] = NULL;
						break;
					}
					right_cmd[i - pipe_check] = cline[i];

					if (i == pipe_check) cline[i] = NULL;
				}
				i = 0;
				for (i = 1; i < MAXARG + 1; i++) {
					right_cmd[i - 1] = right_cmd[i];
				}	
				switch(fork()) {
					case -1:
						perror("fork wrong");
						return -1;
					case 0:
						dup2(pfd[1], 1);
						close(pfd[0]);
						close(pfd[1]);
						execvp(*cline, cline);
						perror("something wrong");
						return -1;
					default:
						dup2(pfd[0], 0);
						close(pfd[0]);
						close(pfd[1]);
						execvp(*right_cmd, right_cmd);
						perror("something wrong");
						return -1;
				}
				return 0;
			}
			else if(redir == 0) { //case of no redirection
				execvp(*cline, cline);
			}
			else if (redir == 2) { //case of output redirection
				int idx = 0;
				for (int i = 0; i < narg; i++) {
					if (strcmp(">", cline[i]) == 0) {
						idx = i;
						break;
					}
				}			
				int out = open(cline[idx + 1], O_CREAT| O_WRONLY | O_TRUNC, 0644);
				dup2(out, 1);
				close(out);
				if (narg > 3) {
					cline[idx] = NULL; cline[idx + 1] = NULL;
				}
				else {
					cline[1] = NULL; cline[2] = NULL;
				}
				narg -= 2;
				execvp(*cline, cline);
			}
			perror(*cline);
			exit(1);
	}			
	if (where == BACKGROUND) {
		printf("[Process id] %d\n", pid);
		return 0;
	}
	if (waitpid(pid, &status, 0) == -1) 
		return -1;
	else
		return status;
}

void cd_implementation(char **line) {
	char *path = (char*)calloc(MAXBUF, sizeof(char));
	if (narg > 1) {
		if (line[1][0] == '~') {
			memset(path, '\0', sizeof(path));
			for (int i = 0; i < homedir_len; i++) {
				path[i] = user_homedir[i];
			}
			if (strlen(line[1]) > 1) {
				strcat(path, &line[1][1]);	
			}
			if(chdir(path)< 0) perror("chdir wrong");
			free(path);
		}
		else {
			if(chdir(line[1]) < 0) perror("chdir wrong");
			free(path);
		}
	}	
	else if (narg == 1) {
		if(chdir(user_homedir) < 0) perror("chdir wrong");
		free(path);
	}
	if (narg > 2) {
		free(path);
		printf("too many argumenrs please run 'man cd'\n");
		return;
	}
}

int check_redir(char **cline) {
	char **commands = cline;
	int in = 0;
	int out = 0;
	for (int i = 0; i < narg; i++) {
		if (strcmp(cline[i], "<") == 0) in++;
		if (strcmp(cline[i], ">") == 0) out++;
	}

	if (in && out) return 3;
	if (in && !out) return 1;
	if (!in && out) return 2;
	return 0;
}

int check_pipe(char **cline) {
	char **commands = cline;
	int p = 0;
	int i;
	for (i = 0; i < narg; i++) {
		if (strcmp(cline[i], "|") == 0) { p++; break; }
	}
	if (p != 0) return i;
	else return 0;
}

int args_grp(char *s, char** argvp, char* delimeter) {
       char *new = NULL;
	int i = 0;
	new = s + strspn(s, delimeter);

	argvp[i] = strtok(new, delimeter);
	if (argvp[i] != NULL) {
		for (i = 1; (argvp[i] = strtok(NULL, delimeter)) != NULL; i++) {
			if (i == (MAXBUF - 1)) return -1;
		}
	}
	if (i > MAXBUF) return -1;
	return i;

}

void exec_cmd(char *cmd) {
	if (args_grp(cmd, cmdgrp, " ") <= 0) perror("parsing wrong");
	printf("here is command %s\n", cmdgrp[0]);
	execvp(cmdgrp[0], cmdgrp);
	
	perror("exec_cmd wrong");
	exit(1);
}


