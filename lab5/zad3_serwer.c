#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h> // Dodano dla struktury utsname

#define MAXLINE 1024
#define SA struct sockaddr

void day_time_br(int sockfd, SA *pcliaddr, socklen_t clilen) {
    int n;
    socklen_t len = clilen;
    char mesg[MAXLINE];
    char str[INET6_ADDRSTRLEN + 1];
    time_t ticks;
    struct utsname sys_info; // Struktura do przechowywania informacji o systemie

    // Wypełnij strukturę sys_info danymi systemowymi
    if (uname(&sys_info) < 0) {
        fprintf(stderr, "uname error: %s\n", strerror(errno));
        return;
    }

    fprintf(stderr, "Sending day time for clients ... \n");

    for (;;) {
        ticks = time(NULL);
        // Tworzenie wiadomości z czasem i informacjami o systemie
        snprintf(mesg, sizeof(mesg),
                 "PS daytime server: %.24s\r\n"
                 "System Name: %s\r\n"
                 "Node Name: %s\r\n",
                 ctime(&ticks), sys_info.sysname, sys_info.nodename);
        fprintf(stdout, "Sending day time for clients: %s\n", mesg);

        if (sendto(sockfd, mesg, strlen(mesg), 0, pcliaddr, len) < 0) {
            fprintf(stderr, "sendto error: %s\n", strerror(errno));
            continue;
        }
        sleep(2);
    }
}

int main(int argc, char **argv) {
    int sockfd, on;
    int len;
    char buff[MAXLINE], str[INET6_ADDRSTRLEN + 1];
    time_t ticks;
    struct sockaddr_in servaddr; /* Service address */

    if (argc != 2) {
        fprintf(stderr, "usage: %s <IPv4 address> \n", argv[0]);
        return 1;
    }

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        fprintf(stderr, "socket error : %s\n", strerror(errno));
        return 1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(13); /* PS daytime service */

    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "inet_pton error for %s : %s \n", argv[1], strerror(errno));
        return 1;
    }

    len = sizeof(on);
    on = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, len) == -1) {
        fprintf(stderr, "SO_BROADCAST setsockopt error : %s\n", strerror(errno));
        return 1;
    }

    day_time_br(sockfd, (SA *)&servaddr, sizeof(servaddr));

    return 0;
}
