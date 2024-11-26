#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <errno.h>
#include <net/if.h>

#define MAXLINE 1024
#define SA struct sockaddr

int snd_udp_socket(const char *serv, int port, SA **saptr, socklen_t *lenp) {
    int sockfd;
    struct sockaddr_in6 *pservaddrv6;

    *saptr = malloc(sizeof(struct sockaddr_in6));
    pservaddrv6 = (struct sockaddr_in6 *)*saptr;

    bzero(pservaddrv6, sizeof(struct sockaddr_in6));
    if (inet_pton(AF_INET6, serv, &pservaddrv6->sin6_addr) <= 0) {
        fprintf(stderr, "inet_pton error for %s : %s\n", serv, strerror(errno));
        return -1;
    }

    pservaddrv6->sin6_family = AF_INET6;
    pservaddrv6->sin6_port = htons(port);
    *lenp = sizeof(struct sockaddr_in6);

    if ((sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "socket error : %s\n", strerror(errno));
        return -1;
    }

    return sockfd;
}

void send_all(int sendfd, SA *sadest, socklen_t salen) {
    char line[MAXLINE];
    struct utsname myname;

    if (uname(&myname) < 0)
        perror("uname error");

    for (;;) {
        printf("Enter message: ");
        if (fgets(line, sizeof(line), stdin) == NULL)
            break;

        char msg[MAXLINE];
        snprintf(msg, sizeof(msg), "%s: %s", myname.nodename, line);

        if (sendto(sendfd, msg, strlen(msg), 0, sadest, salen) < 0)
            fprintf(stderr, "sendto() error : %s\n", strerror(errno));
    }
}

int main(int argc, char **argv) {
    int sendfd;
    socklen_t salen;
    struct sockaddr *sasend;

    if (argc != 4) {
        fprintf(stderr, "usage: %s <IPv6-multicast-address> <port#> <if name>\n", argv[0]);
        return 1;
    }

    sendfd = snd_udp_socket(argv[1], atoi(argv[2]), &sasend, &salen);

    unsigned int ifindex = if_nametoindex(argv[3]);
    if (setsockopt(sendfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0) {
        perror("setting local interface");
        return 1;
    }

    send_all(sendfd, sasend, salen);

    free(sasend);
    close(sendfd);
    return 0;
}
