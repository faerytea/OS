#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
/*#include <errno.h>
//#include <stdio.h> */

const int INIT_BUF_SIZE = 1024;
const int INIT_PID_SIZE = 16;

int is_whitespace(char c) {
	return ((c == '\t') || (c <= ' ')) && (c != '\n');
}

void resize(void **target, unsigned size, unsigned new_size) {
	void *tmp = malloc(new_size);
	memcpy(tmp, *target, size);
	free(target);
	*target = tmp;
}

void expand(void **target, unsigned size) {
	resize(target, size, size*2);
}

/* scans command (and maybe begin of stdin) and returns array of commands
 * where each command represent as array of args (and last one - stdin) */
char ***scan(unsigned *result_size) {
	int commands = 0;
	int cnt;
	unsigned bufp = 0;
	char *buffer = (char *) malloc(INIT_BUF_SIZE * sizeof(char));
	int new_buf_size = INIT_BUF_SIZE;
	while ((cnt = read(0, buffer + bufp, new_buf_size - bufp)) > 0) {
		if (bufp == new_buf_size) {
			//char *new_buffer = (char *) malloc((new_buf_size *= 2) * sizeof(char));
			//memcpy(new_buffer, buffer, new_buf_size / 2);
			//free(buffer);
			//buffer = new_buffer;
			expand((void **) &buffer, new_buf_size * sizeof(char));
			new_buf_size *= 2;
		}
		unsigned i;
		for (i = bufp; i < bufp + cnt; ++i)
			switch (buffer[i]) {
				case '\n': {
					bufp += cnt;
					cnt = i;
					++commands;
					goto LDONE;
				}
				case '|': {
					++commands;
					break;
				}
			}
	}
	if (cnt <= 0) return NULL; // ERROR or ^D
	LDONE:;
	char ***result = (char ***) malloc(commands * sizeof(char **));
	{
		unsigned cmd_no = 0;
		unsigned one_cmd_size = 10;
		unsigned one_cmd_iter = 0;
		char **one_cmd_buffer = (char **) malloc(one_cmd_size * sizeof(char *));
		unsigned i = 0;
		while (i < cnt && is_whitespace(buffer[i++]));
		unsigned mark = --i;
		char shild = 0;
		for (; i < cnt + 1; ++i) {
			if (shild == 0) {
				if (buffer[i] == ' ' || ((buffer[i] == '|' || buffer[i] == '\n') && (i != 0) && !((buffer[i-1] == ' ') || (buffer[i-1] == '\'') || (buffer[i-1] == '\"')))) {
					#include "savestr.c"
				}
				if (buffer[i] == '|' || buffer[i] == '\n') {
					char **done_cmd = (char **) malloc(one_cmd_iter + 1);
					//memcpy(done_cmd, one_cmd_buffer, one_cmd_iter);
					{
						int j; // I know about memcpy, but it don't work here
						for (j = 0; j < one_cmd_iter; ++j) done_cmd[j] = one_cmd_buffer[j];
					}
					done_cmd[one_cmd_iter] = NULL;
					one_cmd_iter = 0;
					result[cmd_no++] = done_cmd;
					--i;
					while (i < cnt && is_whitespace(buffer[++i + 1]));
					mark = i + 1;
				}
				if (buffer[i] == '"' || buffer[i] == '\'') {
					shild = buffer[i];
					mark = i + 1;
				}
				if (buffer[i] == '\\') ++i;
			}
			else {
				if (shild == buffer[i]) {
					shild = 0;
					#include "savestr.c"
					//goto SAVESTRL;
				}
			}
		}
		//result[commands] = (char **) malloc(sizeof(char *) * 10);
		//result[commands][0] = (char *) malloc((bufp - cnt) * sizeof(char));
		//memcpy(result[commands][0], buffer + cnt + 1, bufp - cnt - 1);
		//result[commands][0][bufp - cnt - 1] = '\0';
	}
	*result_size = commands;
	return result;
} // so, it works

void free_tree(char ***tree, unsigned size) {
	unsigned it = size;
	while (--it > -1) {
		int it2 = -1;
		while (tree[it][++it2] != NULL) {
			free(tree[it][it2]);
			write(0, "free?\n", 6);
		}
		free(tree[it]);
		write(0, "free\n", 5);
	}
	free(tree);
	write(0, "free!\n", 6);
}

volatile pid_t *pid_array;
volatile unsigned pid_array_size;
volatile unsigned pid_array_iter;

void run(char **cmd, int from[2], int to[2]) {
	write(0, cmd[0], strlen(cmd[0]));
	write(0, "!\n", 2);
	pid_t pid = fork();
	if (pid == 0) {
		if (from != NULL) {
			dup2(from[0], STDIN_FILENO);
			close(from[0]);
			close(from[1]);
		}
		if (to != NULL) {
			dup2(to[1], STDOUT_FILENO);
			close(to[0]);
			close(to[1]);
		}
		write(0, "Im'a ", 5);
		write(0, cmd[0], strlen(cmd[0]));
		write(0, "!\n", 2);
		execvp(cmd[0], cmd);
	}
	else {
		if (pid_array_iter == pid_array_size) {
			expand((void **) &pid_array, pid_array_size);
			pid_array_size *=2;
		}
		if (from != NULL) {
			dup2(from[0], STDIN_FILENO);
			close(from[0]);
			close(from[1]);
		}
		if (to != NULL) {
			dup2(to[1], STDOUT_FILENO);
			close(to[0]);
			close(to[1]);
		}
		pid_array[pid_array_iter++] = pid;
	}
}

void run_chain(char ***tree, int size) {
	unsigned it = 0;
	write(0, "s1\n", 3);
	int **pipes = (int **) malloc((size - 2) * sizeof(int[2]));
	write(0, "s2\n", 3);
	if (size < 2) {
		write(0, "s3a\n", 4);
		run(tree[0], NULL, NULL);
	}
	else {
		write(0, "s3b\n", 4);
		pipe(pipes[0]);
		run(tree[0], NULL, pipes[0]);
	}
	while (++it < size-1) {
		write(0, "yet another pipe\n", 17);
		pipe(pipes[it]);
		run(tree[it], pipes[it-1], pipes[it]);
	}
	if (size > 1) {
		write(0, "last one\n", 9);
		run(tree[size-1], pipes[it-1], NULL);
	}
	while (pid_array_iter > 0) {
		int status;
		waitpid(pid_array[pid_array_iter], &status, 0);
		if (WIFEXITED(status) || WIFSIGNALED(status)) pid_array_iter--;
	}
	free(pipes);
}

void handler(int signum, siginfo_t * siginfo, void * useless) {
	int i;
	for (i = 0; i < pid_array_iter; ++i) kill(pid_array[i], SIGINT);
}

int main() {

	struct sigaction sa;
	sa.sa_sigaction = &handler;
	sa.sa_flags = SA_SIGINFO;
	sigaddset(&sa.sa_mask, SIGINT);
	sigaction(SIGINT, &sa, NULL);

	pid_array_size = INIT_PID_SIZE;
	pid_array_iter = 0;
	pid_array = (pid_t *) malloc(pid_array_size * sizeof(pid_t));
	char ***line;
	unsigned size;
	goto LGO;
	do {
		unsigned it = -1;
		while (++it < size) {
			int it2 = -1;
			while (line[it][++it2] != NULL) {
				write(0, line[it][it2], strlen(line[it][it2]));
				write(0, "'", 1);
			}
			write(0, "|\n", 2);
		}
		run_chain(line, size);
		free_tree(line, size);
		LGO:
		write(0, "\nftsh\\ ", 7);
	} while ((line = scan(&size)) != NULL);
	return 0;
}
