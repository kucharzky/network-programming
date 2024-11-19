#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/utsname.h>

#define MAXLINE 1024
#define SA struct sockaddr
int snd_udp_socket(const char *serv, int port, SA **saptr, socklen_t *lenp);
int mcast_join(int sockfd, const SA *grp, socklen_t grplen, const char *ifname, u_int ifindex);

void send_all(int sendfd, SA *sadest, socklen_t salen) {
    char line[MAXLINE];
    struct utsname myname;

    if (uname(&myname) < 0)
        perror("uname error");

    for (;;) {
        printf("Enter message: ");
        if (fgets(line, sizeof(line), stdin) == NULL)
            break;
        char message[MAXLINE];
        snprintf(message, sizeof(message), "%s: %s", myname.nodename, line);

        if (sendto(sendfd, message, strlen(message), 0, sadest, salen) < 0)
            fprintf(stderr, "sendto() error : %s\n", strerror(errno));
    }
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <IP-multicast-address> <port#> <if name>\n", argv[0]);
        return 1;
    }

    int sendfd;
    socklen_t salen;
    struct sockaddr *sasend;

    sendfd = snd_udp_socket(argv[1], atoi(argv[2]), &sasend, &salen);

    if (setsockopt(sendfd, SOL_SOCKET, SO_BINDTODEVICE, argv[3], strlen(argv[3])) < 0) {
        perror("setsockopt SO_BINDTODEVICE error");
        return 1;
    }

    send_all(sendfd, sasend, salen);

    free(sasend);
    close(sendfd);
    return 0;
}
