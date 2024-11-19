#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <net/if.h>

#define MAXLINE 1024
#define SA struct sockaddr

int mcast_join(int sockfd, const SA *grp, socklen_t grplen, const char *ifname) {
    struct ipv6_mreq mreq6;
    memcpy(&mreq6.ipv6mr_multiaddr, &((struct sockaddr_in6 *)grp)->sin6_addr, sizeof(struct in6_addr));
    mreq6.ipv6mr_interface = if_nametoindex(ifname);

    if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq6, sizeof(mreq6)) < 0) {
        perror("setsockopt IPV6_JOIN_GROUP");
        return -1;
    }
    return 0;
}

void recv_all(int recvfd, socklen_t salen) {
    char line[MAXLINE + 1];
    socklen_t len;
    struct sockaddr *safrom;
    safrom = malloc(salen);

    for (;;) {
        len = salen;
        int n = recvfrom(recvfd, line, MAXLINE, 0, safrom, &len);
        if (n < 0)
            perror("recvfrom error");

        line[n] = 0;
        printf("Received: %s\n", line);
        fflush(stdout);
    }
}

int main(int argc, char **argv) {
    int recvfd;
    const int on = 1;
    socklen_t salen;
    struct sockaddr_in6 sarecv;

    if (argc != 4) {
        fprintf(stderr, "usage: %s <IPv6-multicast-address> <port#> <if name>\n", argv[0]);
        return 1;
    }

    recvfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (recvfd < 0) {
        perror("socket error");
        return 1;
    }

    if (setsockopt(recvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        return 1;
    }

    bzero(&sarecv, sizeof(sarecv));
    sarecv.sin6_family = AF_INET6;
    sarecv.sin6_addr = in6addr_any;
    sarecv.sin6_port = htons(atoi(argv[2]));
    salen = sizeof(sarecv);

    if (bind(recvfd, (SA *)&sarecv, salen) < 0) {
        perror("bind error");
        return 1;
    }

    struct sockaddr_in6 group;
    inet_pton(AF_INET6, argv[1], &group.sin6_addr);
    if (mcast_join(recvfd, (SA *)&group, salen, argv[3]) < 0)
        return 1;

    recv_all(recvfd, salen);

    close(recvfd);
    return 0;
}
