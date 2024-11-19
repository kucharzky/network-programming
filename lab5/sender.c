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
int snd_udp_socket(const char *serv, int port, SA **saptr, socklen_t *lenp)
{
	int sockfd, n;
	struct addrinfo	hints, *res, *ressave;
	struct sockaddr_in6 *pservaddrv6;
	struct sockaddr_in *pservaddrv4;

	*saptr = malloc( sizeof(struct sockaddr_in6));
	
	pservaddrv6 = (struct sockaddr_in6*)*saptr;

	bzero(pservaddrv6, sizeof(struct sockaddr_in6));

	if (inet_pton(AF_INET6, serv, &pservaddrv6->sin6_addr) <= 0){
	
		free(*saptr);
		*saptr = malloc( sizeof(struct sockaddr_in));
		pservaddrv4 = (struct sockaddr_in*)*saptr;
		bzero(pservaddrv4, sizeof(struct sockaddr_in));

		if (inet_pton(AF_INET, serv, &pservaddrv4->sin_addr) <= 0){
			fprintf(stderr,"AF_INET inet_pton error for %s : %s \n", serv, strerror(errno));
			return -1;
		}else{
			pservaddrv4->sin_family = AF_INET;
			pservaddrv4->sin_port   = htons(port);
			*lenp =  sizeof(struct sockaddr_in);
			if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
				fprintf(stderr,"AF_INET socket error : %s\n", strerror(errno));
				return -1;
			}
		}

	}else{
		pservaddrv6 = (struct sockaddr_in6*)*saptr;
		pservaddrv6->sin6_family = AF_INET6;
		pservaddrv6->sin6_port   = htons(port);	/* daytime server */
		*lenp =  sizeof(struct sockaddr_in6);

		if ( (sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0){
			fprintf(stderr,"AF_INET6 socket error : %s\n", strerror(errno));
			return -1;
		}

	}

	return(sockfd);
}

int mcast_join(int sockfd, const SA *grp, socklen_t grplen,
		   const char *ifname, u_int ifindex)
{
	struct group_req req;
	if (ifindex > 0) {
		req.gr_interface = ifindex;
	} else if (ifname != NULL) {
		if ( (req.gr_interface = if_nametoindex(ifname)) == 0) {
			errno = ENXIO;	/* if name not found */
			return(-1);
		}
	} else
		req.gr_interface = 0;
	if (grplen > sizeof(req.gr_group)) {
		errno = EINVAL;
		return -1;
	}
	memcpy(&req.gr_group, grp, grplen);
	return (setsockopt(sockfd, family_to_level(grp->sa_family),
			MCAST_JOIN_GROUP, &req, sizeof(req)));
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
