#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 1024*4 // 4kb

char buf[BUF_SIZE];

int redirection(int fd_from, int fd_to) {
	int received;
	do {
		received = read(fd_from, buf, BUF_SIZE);
		if (received < 0) {
			return -1;
		}
		else {
			int written = 0;
			do {
				written = write(fd_to, buf + written, received - written);
				if (written < 0) return -2;
			} while (received != written);
		}
	} while (received != 0);
	return 0;
}
