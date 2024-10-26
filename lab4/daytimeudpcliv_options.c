#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include        <sys/time.h>    /* timeval{} for select() */
#include        <time.h>                /* timespec{} for pselect() */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <fcntl.h>               /* for nonblocking */
#include        <netdb.h>
#include        <signal.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
//#include 		<linux/ipv6.h>
#include		<netinet/in.h>

#define MAXLINE 1024
#define SA      struct sockaddr

struct in6_pktinfo {
        struct in6_addr ipi6_addr;
        int             ipi6_ifindex;
};


int
dt_cli(int sockfd, const SA *pservaddr, socklen_t servlen, struct sockaddr	*preply_addr, 
		socklen_t *t, char *recvline, int *data_len, struct sockaddr *src_addr)
{
	int				n, i;
	char			sendline[MAXLINE]; 
	
	socklen_t		len;
	//struct sockaddr	*preply_addr=NULL;
	
	struct sockaddr_in6	 cliaddrv6;
	struct sockaddr_in	 cliaddrv4;
	struct sockaddr_in  *pcliaddrv4;
	struct sockaddr_in6	 *pcliaddrv6;
	struct sockaddr_in  *pdstaddrv4;
	struct sockaddr_in6	 *pdstaddrv6;

	struct timeval delay;
	
	struct  msghdr msg;
	struct iovec	iov[1];
//	ssize_t			t;
	struct cmsghdr	*cmptr;
	struct in_pktinfo pktinfov4;
	struct in6_pktinfo pktinfov6;

	union {
	  struct cmsghdr	cm;
	  char				control[CMSG_SPACE(sizeof(struct in6_pktinfo))];
	} control_un;

	pdstaddrv4 = (struct sockaddr_in *) src_addr;
	pdstaddrv6= (struct sockaddr_in6 *) src_addr;

	delay.tv_sec =2;  //opóŸnienie na gnieŸdzie
	delay.tv_usec = 1; 
	len = sizeof(delay);
	if( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &delay, len) == -1 ){
		fprintf(stderr,"SO_RCVTIMEO setsockopt error : %s\n", strerror(errno));
		return -1;
	}

	
	bzero(&msg, sizeof(msg));

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	msg.msg_flags = 0;

	msg.msg_name = preply_addr;
	msg.msg_namelen = *t;
	iov[0].iov_base = recvline;
	iov[0].iov_len = MAXLINE;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;


//	if( (preply_addr = malloc(servlen)) == NULL ){
//	    perror("malloc error");
//	    exit(1);
//	}
	bzero( sendline, sizeof(sendline));

	len = servlen;
	for(i=0; i < 3 ; i++ ){

		if( sendto(sockfd, sendline, 0, 0, pservaddr, servlen) <0 ){
	    	perror("sendto error");
	    	return -1;
		}


		if( (n = recvmsg(sockfd, &msg, 0) ) < 0 ){
			perror("recvmsg() error");
			if( (errno == (EAGAIN | EWOULDBLOCK)) && (i < 2) ){

				delay.tv_sec = delay.tv_sec << 1;  //opóŸnienie na gnieŸdzie
				len = sizeof(delay);
				if( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &delay, len) == -1 ){
					fprintf(stderr,"SO_RCVTIMEO setsockopt error : %s\n", strerror(errno));
					return -1;
				}

				continue;
			}else{
				return -1;
			}
		}else
			break;
	}
	
	*t = msg.msg_namelen;
	*data_len = n;

	if (msg.msg_controllen < CMSG_SPACE(sizeof(struct in_pktinfo)) || 	(msg.msg_flags & MSG_CTRUNC) )
		return -2;

	if( src_addr == NULL)
		return -2;

	for (cmptr = CMSG_FIRSTHDR(&msg); cmptr != NULL;
		 cmptr = CMSG_NXTHDR(&msg, cmptr)) {

		if( preply_addr->sa_family == AF_INET ){
			if (cmptr->cmsg_level == IPPROTO_IP &&
				cmptr->cmsg_type == IP_PKTINFO) {

				memcpy(&pktinfov4, CMSG_DATA(cmptr),
					   sizeof(struct in_pktinfo));
				memcpy(&(pdstaddrv4->sin_addr), &pktinfov4.ipi_addr, sizeof(struct in_addr));
				pdstaddrv4->sin_family = AF_INET;

				continue;
			}
		}else{
			if (cmptr->cmsg_level == IPPROTO_IPV6 &&
				cmptr->cmsg_type == IPV6_PKTINFO) {

				memcpy(&pktinfov6, CMSG_DATA(cmptr),
					   sizeof(struct in6_pktinfo));
				memcpy(&(pdstaddrv6->sin6_addr), &pktinfov6.ipi6_addr, sizeof(struct in6_addr));
				pdstaddrv6->sin6_family = AF_INET6;
				continue;
			}
		}
		printf("\nUnknown ancillary data, len = %d, level = %d, type = %d\n",
				 (int)cmptr->cmsg_len, cmptr->cmsg_level, cmptr->cmsg_type);
	}


	return 1;
}


int
main(int argc, char **argv)
{
	int					sockfd, n, delay, on=1;
	struct sockaddr_in6	servaddrv6;
	struct sockaddr_in	servaddrv4;
	struct sockaddr_in6	cliaddrv6, *pcliaddrv6;
	struct sockaddr_in	cliaddrv4, *pcliaddrv4;
	struct sockaddr *pservaddr;
	struct sockaddr *preply_addr, *psrc_addr;
	struct sockaddr_storage reply_addr, src_addr;
	int addr_len;
	char				recvline[MAXLINE + 1];
	char			str[INET6_ADDRSTRLEN+1];
	int p_addr_len, data_len;
	int ret;

	preply_addr = (SA *) &reply_addr;
	psrc_addr = (SA *) &src_addr;

	if (argc != 2){
		fprintf(stderr, "usage: a.out <IPaddress> \n");
		return 1;
	}

	bzero(&servaddrv4, sizeof(servaddrv4));
	bzero(&servaddrv6, sizeof(servaddrv6));

	if (inet_pton(AF_INET6, argv[1], &servaddrv6.sin6_addr) <= 0){
	
		if (inet_pton(AF_INET, argv[1], &servaddrv4.sin_addr) <= 0){
			fprintf(stderr,"AF_INET inet_pton error for %s : %s \n", argv[1], strerror(errno));
			return 1;
		}else{
			servaddrv4.sin_family = AF_INET;
			servaddrv4.sin_port   = htons(13);	/* daytime server */
			pservaddr = (SA *) &servaddrv4;
			addr_len =  sizeof(servaddrv4);
			p_addr_len = sizeof(servaddrv4);
			if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
				fprintf(stderr,"AF_INET socket error : %s\n", strerror(errno));
				return 2;
			}

			if (setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on)) < 0){
				perror("setsockopt of IP_PKTINFO");
				return 3;
			}

		}

	//	fprintf(stderr,"inet_pton error for %s : %s \n", argv[1], strerror(errno));
	//	return 1;
	}else{
		servaddrv6.sin6_family = AF_INET6;
		servaddrv6.sin6_port   = htons(13);	/* daytime server */
		pservaddr = (SA *) &servaddrv6;
		addr_len =  sizeof(servaddrv6);
		p_addr_len = sizeof(servaddrv6);
		
		if ( (sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0){
			fprintf(stderr,"AF_INET6 socket error : %s\n", strerror(errno));
			return 1;
		}

		if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on)) < 0){
				perror("setsockopt of IPV6_RECVPKTINFO");
				return 2;
		}
	}
	
	ret = dt_cli( sockfd, pservaddr, addr_len, preply_addr, &p_addr_len, recvline, 
				&data_len, psrc_addr );
	
	if(ret == -1)
		return -1;

	bzero(str, sizeof(str));
	
 	if( preply_addr->sa_family == AF_INET6 ){
		      pcliaddrv6 = (struct sockaddr_in6*) preply_addr;
		      inet_ntop(AF_INET6, (struct sockaddr  *) &pcliaddrv6->sin6_addr,  str, sizeof(str));
	}
	else{
		      pcliaddrv4 = (struct sockaddr_in*) preply_addr;
		      inet_ntop(AF_INET, (struct sockaddr  *) &pcliaddrv4->sin_addr,  str, sizeof(str));
	}

	printf("Time from %s (%d)\n", str,data_len);

	if (addr_len != p_addr_len || memcmp(pservaddr, preply_addr, p_addr_len) != 0) {
		printf("reply from %s (ignored)\n", str);
	}

	recvline[data_len] = 0;	/* null terminate */
	if (fputs(recvline, stdout) == EOF){
		fprintf(stderr,"fputs error : %s\n", strerror(errno));
		exit(1);
	}

	if(ret == -2)
		return -2;

 	if( psrc_addr->sa_family == AF_INET6 ){
		      pcliaddrv6 = (struct sockaddr_in6*) psrc_addr;
		      inet_ntop(AF_INET6, (struct sockaddr  *) &pcliaddrv6->sin6_addr,  str, sizeof(str));
	}
	else{
		      pcliaddrv4 = (struct sockaddr_in*) psrc_addr;
		      inet_ntop(AF_INET, (struct sockaddr  *) &pcliaddrv4->sin_addr,  str, sizeof(str));
	}

	printf("Data destination addr: %s \n", str);

	exit(0);
}
