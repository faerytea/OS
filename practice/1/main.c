#include <unistd.h>
#include <sys/wait.h>

extern char **environ;

int main(int argc, char ** argv) {
	int tube[2];
	pipe(tube);
	pid_t cat = fork();
	if (cat != 0) {
		pid_t grep = fork();
		if (grep != 0) {
			dup2(tube[0], STDIN_FILENO);
			close(tube[0]);
			close(tube[1]);
			execlp("grep", "grep", "int", NULL);
		}
		else {
			close(tube[0]);
			close(tube[1]);
			int status;
			waitpid(cat, &status, 0);
			waitpid(grep, &status, 0);
			return 0;
		}
	}
	else {
		dup2(tube[1], STDOUT_FILENO);
		close(tube[0]);
		close(tube[1]);
		execlp("cat", "cat", argv[1], NULL);
	}
	return -1;
}
