#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define BUF_SIZE 1024*4 // 4kb

char buf[BUF_SIZE];

int redirection(int fd) {
	int received;
	do {
		received = read(fd, buf, BUF_SIZE);
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
	} while (received != 0);
	return 0;
}

void printerr(const char *prestr) {
	int err = errno;
	write(2, prestr, strlen(prestr));
	char *strerr = strerror(err);
	write(2, strerr, strlen(strerr));
}

int main(int argc, char **argv) {
	int a; // Счётчик аргументов
	for (a = 1; a < argc; ++a) {
		int fd = open(argv[a], O_RDONLY);
		if (fd < 0) {
			printerr(argv[a]);
		}
		else {
			switch (redirection(fd)) {
				case -1:
					printerr("Read error: ");
					break;
				case -2:
					printerr("Write error: ");
					break;
				case 0:
				default:
					break;
			}
			fd = close(fd);
			if (fd < 0) {
				printerr(argv[a]);
			}
		}
	}
	if ((a == 1) && (redirection(0) != 0)) {
		printerr("Error in stdin! ");
	}
	return 0;
}

