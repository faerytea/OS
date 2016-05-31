#define _XOPEN_SOURCE 600
#define _GNU_SOURCE
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/ip.h>
#include <signal.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#define MAXEVENTS 7
#define TIMEOUT 1000
#define BUF_SIZE 1024
#define MAX_FD 20000
#define ADDITIONAL_QUEUE_BUF_CAP 10

volatile char stopsig = 1;

struct queue_entry {
	int fd;
	char *buffer;
	unsigned offset;
	unsigned length;
	unsigned cap;
	struct queue_entry *next;
};

char *buffer;
struct queue_entry *fdtable[MAX_FD];

int sockfd = -1;
int pollfd = -1;
struct node {
	int ptyfd; // = -1;
	pid_t ptypid;
	struct node *next; // = NULL;
} *root = NULL, *last = NULL;

struct desc_pair {
	int from;
	int to;
	struct desc_pair *another;
};

void init_network(int port) {
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("socket creating fails");
		exit(-3);
	}
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);//atoi(argv[1]));
	if (bind(sockfd, (struct sockaddr*) &address, sizeof(address)) < 0) {
		perror("binding problems");
		close(sockfd);
		exit(-4);
	}
	listen(sockfd, 17);
}

void delete_node(int fdpty) {
	struct node *i = root;
	struct node *j = NULL;
	if (root->ptyfd == fdpty) {
		root = root->next;
		if (root == NULL) last = NULL;
		else last = i->next;
	}
	else {
		for (j = root; (j->next != NULL) && (j->next->ptyfd != fdpty); j = j->next);
		if (j->next == NULL) {
			perror("Zombie ran away! Hide, if you can!\n");
			return;
		}
		i = j->next;
	}
	kill(i->ptypid, SIGTERM);
	int status;
	do {
		waitpid(i->ptyfd, &status, 0);
	} while (WIFSIGNALED(status));
	if (j != NULL) j->next = i->next;
	free(i);
}

void close_evrything() {
	if (sockfd != -1) close(sockfd);
	if (pollfd != -1) close(pollfd);
	if (root != NULL)
		for (struct node *i = root; i->next; i = i->next)
			close(i->ptyfd); // add killing children
}

int term() {
	int ptyfd = posix_openpt(O_RDWR);
	if (ptyfd == -1) {
		printf("openpt fails");
		exit(-1);
	}
	if (!((grantpt(ptyfd) != -1) && (unlockpt(ptyfd) != -1))) {
		printf("something went wrong");
		exit(-2);
	}
	//printf("pty = %s\n", ptsname(ptyfd));
	return ptyfd;
}

struct node *shell() {
	int fdpty = term();

	pid_t f = fork();
	if (f == 0) {
		setsid();
		int fslave = open(ptsname(fdpty), O_RDWR | O_NONBLOCK);

		struct termios slave_orig_term_settings; // Saved terminal settings
		struct termios new_term_settings; // Current terminal settings
		tcgetattr(fslave, &slave_orig_term_settings);
		new_term_settings = slave_orig_term_settings;
		new_term_settings.c_lflag &= ~(ECHO | ECHONL | ICANON);
		tcsetattr(fslave, TCSANOW, &new_term_settings);

		dup2(fslave, STDIN_FILENO);
		dup2(fslave, STDOUT_FILENO);
		dup2(fslave, STDERR_FILENO);
		close(fslave);
		close(fdpty);
		close_evrything();
		execl("/bin/sh", "/bin/sh", NULL);
	}
	struct node *n = (struct node *)malloc(sizeof(struct node));

	n->ptyfd = fdpty;
	n->ptypid = f;
	n->next = NULL;
	if (last != NULL) {
		last->next = n;
	}
	else {
		root = n;
	}
	last = n;
	return n;
}

struct epoll_event *make_out_event(uint32_t what, struct desc_pair *pair) {
	struct epoll_event *event = (struct epoll_event *)malloc(sizeof(struct epoll_event));
	event->events = what;
	event->data.ptr = pair;
	return event;
}

//void delete_out_event(struct epoll_event *event) {
	//if ((close(event->data.fd) == -1) && (errno != EBADF)) {
		//// bad
	//}
//}

//struct epoll_event *mod_event(uint32_t what, struct epoll_event *e) {
	//e->events = what;
	//return e;
//}

struct epoll_event *make_events(uint32_t what, int from, int to) {
	struct desc_pair *pair0 = (struct desc_pair *)malloc(sizeof(struct desc_pair));
	struct desc_pair *pair1 = (struct desc_pair *)malloc(sizeof(struct desc_pair));
	pair0->from = from;
	pair1->from = to;
	pair0->to = to;
	pair1->to = from;
	pair0->another = pair1;
	pair1->another = pair0;
	struct epoll_event *event = (struct epoll_event *)malloc(sizeof(struct epoll_event) * 2);
	event[0].events = what;
	event[0].data.ptr = (void *)pair0;
	event[1].events = what;
	event[1].data.ptr = (void *)pair1;
	return event;
}

void delete_event(struct epoll_event *event) {
	struct desc_pair *p = (struct desc_pair *)event->data.ptr;
	if ((close(p->from) == -1) && (errno != EBADF)) {
		// bad
	}
	if ((close(p->to) == -1) && (errno != EBADF)) {
		// bad
	}
	//if ((close(p->another->from) == -1) && (errno != EBADF)) {
		//// bad
	//}
	//if ((close(p[1].to) == -1) && (errno != EBADF)) {
		//// bad
	//}
	free(p);
}

struct queue_entry *create_queue(int fd, char *initial_buffer, unsigned length) {
	struct queue_entry *res = (struct queue_entry *) malloc(sizeof(struct queue_entry));
	res->buffer = (char *) malloc(sizeof(char) * (length + ADDITIONAL_QUEUE_BUF_CAP));
	memcpy(res->buffer, initial_buffer, length * sizeof(char));
	res->cap = length + ADDITIONAL_QUEUE_BUF_CAP;
	res->fd = fd;
	res->length = length;
	res->next = NULL;
	res->offset = 0;
	return res;
}

struct queue_entry *find_queue(int fd) {
	struct queue_entry *res;
	for (res = fdtable[fd % MAX_FD]; (res != NULL) && (res->fd != fd); res = res->next);
	return res;
}

void add_queue_to_fdtable(struct queue_entry *queue) {
	queue->next = fdtable[queue->fd % MAX_FD];
	fdtable[queue->fd % MAX_FD] = queue;
}

void enqueue(struct queue_entry *queue, char *buf, unsigned length) {
	if ((queue->cap - queue->length) < length) {
		char *tmp = (char *) malloc((queue->length * 2 + length) * sizeof(char));
		memcpy(tmp, (queue->buffer + queue->offset), queue->length - queue->offset);
		queue->cap = (queue->length * 2 + length);
		queue->length -= queue->offset;
		queue->offset = 0;
		free(queue->buffer);
		queue->buffer = tmp;
	}
	memcpy((queue->buffer + queue->length), buf, length);
	queue->length += length;
}

void clear_queue(struct queue_entry *queue) {
	queue->offset = 0;
	queue->length = 0;
}

struct queue_entry *delete_queue(struct queue_entry *queue) {
	free(queue->buffer);
	//queue->cap = queue->length = queue->offset = 0;
	struct queue_entry *next = queue->next;
	free(queue);
	return next;
}

void delete_queue_by_fd(int fd) {
	struct queue_entry *i = fdtable[fd % MAX_FD];
	if ((i != NULL) && (i->fd == fd)) {
		fdtable[fd % MAX_FD] = delete_queue(i);
		return;
	}
	while ((i->next != NULL) && (i->next->fd != fd)) i = i->next;
	if (i->next == NULL) return;
	i->next = delete_queue(i->next);
}

void handler(int signum, siginfo_t * siginfo, void * useless) {
	stopsig = 0;
}

void demonize() {
	int file = open("/tmp/rshd.pid", O_RDONLY);
	if (file > 0) {
		unsigned short w = 0, dw = 0;
		do {
			dw = read(file, buffer + w, BUF_SIZE);
			w += dw;
		} while (dw > 0);
		buffer[w] = 0;
		if (!kill(atoi(buffer), 0)) {
			perror("Already running: ");
			perror(buffer);
			perror("\n");
			close_evrything();
			close(file);
			exit(0);
		}
	}
	close(file);
	int f1 = fork();
	if (f1 < 0) {
		perror("cannot fork =( \n");
		exit(-23);
	}
	if (f1 != 0) {
		exit(0);
	}
	setsid();
	int f2 = fork();
	if (f2 < 0) {
		perror("cannot fork =( \n");
		exit(-23);
	}
	if (f2 != 0) {
		file = open("/tmp/rshd.pid", O_WRONLY | O_CREAT | O_TRUNC, 01664);
		if (file < 0) {
			perror("cannot create a file\n");
			exit(-17);
		}
		char *tbuff = (char *) malloc(15 * sizeof(char));
		memset(tbuff, 0, 15 * sizeof(char));
		sprintf(tbuff, "%d", f2);
		write(file, tbuff, strlen(tbuff));
		close(file);
		exit(0);
	}
	// !!
}

int main(int argc, char ** argv) {
	if (argc < 2) return 0;
	{
		unsigned i;
		for (i = 0; i < MAX_FD; fdtable[i++] = NULL);
	}
	buffer = (char *) malloc(BUF_SIZE * sizeof(char));

	// demonize

	demonize();

	init_network(atoi(argv[1]));

	pollfd = epoll_create(17);

	struct epoll_event sse;
	sse.events = EPOLLIN;
	sse.data.ptr = NULL;
	epoll_ctl(pollfd, EPOLL_CTL_ADD, sockfd, &sse);

	// set signals

	struct sigaction sa;
	sa.sa_sigaction = &handler;
	sa.sa_flags = SA_SIGINFO;
	//char i_sig;
	//for (i_sig = 1; i_sig < 32; ++i_sig) {
		sigaddset(&sa.sa_mask, SIGTERM);
		sigaddset(&sa.sa_mask, SIGINT);
	//}
	//for (i_sig = 1; i_sig < 32; ++i_sig) {
		sigaction(SIGTERM, &sa, NULL);
		sigaction(SIGINT, &sa, NULL);
	//}

	struct epoll_event *events = (struct epoll_event *) malloc (MAXEVENTS * sizeof(struct epoll_event));

	while (stopsig) {
		int descs = epoll_wait(pollfd, events, MAXEVENTS, TIMEOUT), i;
		for (i = 0; i < descs; ++i) {
			if (events[i].data.ptr == NULL) {
				// new connection
				int sd = accept(sockfd, NULL, NULL); //, O_NONBLOCK | O_CLOEXEC); // errors // accept4
				printf("Accepted socket: %d\n", sd);
				struct node *sh = shell();
				struct epoll_event *es = make_events(EPOLLIN, sd, sh->ptyfd);
				epoll_ctl(pollfd, EPOLL_CTL_ADD, sd, &es[0]);//make_event(EPOLLIN, sd, sh->ptyfd));
				epoll_ctl(pollfd, EPOLL_CTL_ADD, sh->ptyfd, &es[1]);//make_event(EPOLLIN, sh->ptyfd, sd));
				free(es);
				fcntl(sd, F_SETFL, fcntl(sd, F_GETFL) | O_NONBLOCK | O_CLOEXEC); // accept4 is same
				fcntl(sh->ptyfd, F_SETFL, fcntl(sh->ptyfd, F_GETFL) | O_NONBLOCK); // accept4 is same
				// add hup
			}
			else {
				uint32_t ev = events[i].events;
				// transmit or disconnect
				if (ev & EPOLLIN) {
					struct desc_pair * pair = ((struct desc_pair *)events[i].data.ptr);
					int r = read(pair->from, buffer, BUF_SIZE); // !!! BLOCKS !!! why?
					printf("read()-> %d\n", r);
					if (r < 0) {
						perror(errno == EAGAIN ? "again" : "else");
						perror(strerror(errno));
						//exit(-1);
						continue;
					}
					struct queue_entry *que = find_queue(pair->to);
					if (r == 0) {
						epoll_ctl(pollfd, EPOLL_CTL_DEL, pair->from, NULL);
						if (que == NULL) {
							printf("empty shell killed\n");
							delete_node(pair->to);
						}
						if ((que != NULL) && (que->length == que->offset)) {
							epoll_ctl(pollfd, EPOLL_CTL_DEL, pair->to, NULL);
							printf("%p %d %d\n", pair->another, pair->another->from, pair->another->to);
							printf("%p %d %d\n", pair, pair->from, pair->to);
							delete_node(pair->to);
							free((struct desc_pair *)pair->another);
							delete_queue_by_fd(que->fd);
						}
						delete_event(&events[i]);
					}
					else {
						if (que == NULL) {
							add_queue_to_fdtable(create_queue(pair->to, buffer, r));
						}
						else {
							enqueue(que, buffer, r);
						}
						struct epoll_event *tmpe = make_out_event(EPOLLOUT | EPOLLIN, pair->another);
						epoll_ctl(pollfd, EPOLL_CTL_MOD, pair->to, tmpe);//make_event(EPOLLOUT | EPOLLIN, pair->to, pair->from));//mod_event(EPOLLOUT | EPOLLIN, &events[i]));
						free(tmpe);
					}
				}
				else if (ev & EPOLLOUT) {
					int tfd = ((struct desc_pair *)(events[i].data.ptr))->from;
					struct queue_entry *que = find_queue(tfd);
					if (que == NULL) {
						// unregister, delete
						printf("WTF %d \n", tfd);
						epoll_ctl(pollfd, EPOLL_CTL_DEL, tfd, NULL);
						delete_event(&events[i]);
						delete_queue_by_fd(tfd);
						delete_node(tfd);
					}
					else {
						int w = write(tfd, (que->buffer + que->offset), que->length - que->offset);
						if (w < 0) {
							perror(strerror(errno));
							continue;
						}
						que->offset += w;
						if (que->offset == que->length) {
							clear_queue(que);
							struct epoll_event *tmpe = make_out_event(EPOLLIN, events[i].data.ptr);
							epoll_ctl(pollfd, EPOLL_CTL_MOD, tfd, tmpe);//make_event(EPOLLOUT | EPOLLIN, tfd, ((struct desc_pair *)(events[i].data.ptr))->from));//mod_event(EPOLLIN, &events[i]));
							free(tmpe);
						}
					}
				}
			}
		}
	}

	close_evrything();

	while (root != NULL) {
		delete_node(root->ptyfd);
	}

	printf(" exit\n");

	//int clientfd;
	//if ((clientfd = accept(sockfd, NULL, NULL)) == -1) {
		//printf("accepting problems");
		//close(sockfd);
		//return -5;
	//}

	//char c;
	//for (int t = 1;
		//(t > 0) &&
		//((t = read(ptyfd, &c, 1)) > 0);
		//t = write(ptyfd, &c, 1)) {
		////
	//}

	return 0;
}
