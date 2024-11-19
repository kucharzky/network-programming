#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#define MAXLINE 500

int main(int argc, char *argv[]) {
    struct timeval start_time, current_time, delay;
    int sfd, n, s;
    socklen_t len;
    char recvline[MAXLINE], str[INET6_ADDRSTRLEN + 1];
    struct sockaddr_in servaddr, peer_addr;
    char host[NI_MAXHOST], service[NI_MAXSERV];

    if ((sfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "socket error: %s\n", strerror(errno));
        return 1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(13); /* daytime server */

    if (bind(sfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        fprintf(stderr, "bind error: %s\n", strerror(errno));
        return 1;
    }

    delay.tv_sec = 6; // Total listening time
    delay.tv_usec = 0;
    gettimeofday(&start_time, NULL);

    printf("Listening for UDP packets for 6 seconds...\n");

    while (1) {
        len = sizeof(peer_addr);
        gettimeofday(&current_time, NULL);

        // Check if 6 seconds have passed
        if ((current_time.tv_sec - start_time.tv_sec) >= 6) {
            printf("Timeout reached. Exiting.\n");
            break;
        }

        n = recvfrom(sfd, recvline, MAXLINE, MSG_DONTWAIT, (struct sockaddr *)&peer_addr, &len);

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue; // Non-blocking wait
            } else {
                perror("recvfrom error");
                exit(1);
            }
        }

        s = getnameinfo((struct sockaddr *)&peer_addr, len, host, NI_MAXHOST, service, NI_MAXSERV,
                        NI_NUMERICSERV | NI_NUMERICHOST);
        if (s == 0) {
            printf("Received %ld bytes from %s:%s\n", (long)n, host, service);
        } else {
            fprintf(stderr, "getnameinfo: %s\n", gai_strerror(s));
        }

        recvline[n] = '\0'; /* Null-terminate the string */
        if (fputs(recvline, stdout) == EOF) {
            fprintf(stderr, "fputs error: %s\n", strerror(errno));
            exit(1);
        }
        printf("\n");
    }

    close(sfd);
    return 0;
}
