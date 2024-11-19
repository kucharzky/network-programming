#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define SA struct sockaddr
int snd_udp_socket(const char *serv, int port, SA **saptr, socklen_t *lenp);
int mcast_join(int sockfd, const SA *grp, socklen_t grplen, const char *ifname, u_int ifindex);



#define MAXLINE 1024

void recv_all(int recvfd, socklen_t salen) {
    char line[MAXLINE + 1];
    char addr_str[INET6_ADDRSTRLEN + 1];
    struct sockaddr_storage safrom;
    socklen_t len;
    int n;

    for (;;) {
        len = salen;
        if ((n = recvfrom(recvfd, line, MAXLINE, 0, (struct sockaddr *)&safrom, &len)) < 0)
            perror("recvfrom() error");

        line[n] = 0;

        if (safrom.ss_family == AF_INET6) {
            struct sockaddr_in6 *cliaddr = (struct sockaddr_in6 *)&safrom;
            inet_ntop(AF_INET6, &cliaddr->sin6_addr, addr_str, sizeof(addr_str));
        } else {
            struct sockaddr_in *cliaddr = (struct sockaddr_in *)&safrom;
            inet_ntop(AF_INET, &cliaddr->sin_addr, addr_str, sizeof(addr_str));
        }

        printf("Datagram from %s: %s (%d bytes)\n", addr_str, line, n);
        fflush(stdout);
    }
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <IP-multicast-address> <port#> <if name>\n", argv[0]);
        return 1;
    }

    int recvfd;
    const int on = 1;
    socklen_t salen;
    struct sockaddr *sasend, *sarecv;

    recvfd = snd_udp_socket(argv[1], atoi(argv[2]), &sasend, &salen);

    sarecv = malloc(salen);
    memcpy(sarecv, sasend, salen);

    if (setsockopt(recvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        perror("setsockopt SO_REUSEADDR error");
        return 1;
    }

    if (bind(recvfd, sarecv, salen) < 0) {
        perror("bind error");
        return 1;
    }

    if (mcast_join(recvfd, sasend, salen, argv[3], 0) < 0) {
        perror("mcast_join() error");
        return 1;
    }

    recv_all(recvfd, salen);

    free(sasend);
    free(sarecv);
    close(recvfd);
    return 0;
}