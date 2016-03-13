#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <redir.h>

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
			switch (redirection(fd, 1)) {
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
	if ((a == 1) && (redirection(0, 1) != 0)) {
		printerr("Error in stdin! ");
	}
	return 0;
}

