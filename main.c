#include "smallsh.h"

char *prompt = "Command> ";
char *user_homedir;
int main() {
	get_homedir();
	
	struct sigaction act;
	act.sa_handler = child_handler;
	sigfillset(&act.sa_mask);
	act.sa_flags = SA_RESTART;
 	sigaction(SIGCHLD, &act, NULL);

	signal(SIGINT, SIG_IGN);	
	
	while(userin(prompt) != EOF) {
		procline();
	}
	return 0;
}

