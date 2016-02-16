#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 1024*4 // 4kb

char buf[BUF_SIZE];

int redirection(int fd) {
	int received = read(fd, buf, BUF_SIZE);
	if (received < 0) {
		return -1;
	}
	else {
		int written = 0;
		do {
			written = write(1, buf + written, received - written);
			if (written < 0) return -2;
		} while (received != written);
	}
	return 0;
}
