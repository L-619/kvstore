#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<sys/poll.h>
#include<sys/epoll.h>
#include<string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include"kvstore.h"


#define ENABLE_HTTP_RESPONSE 1






int epfd;
int accept_cb(int fd);
int recv_cb(int fd);
int send_cb(int fd);


#if ENABLE_HTTP_RESPONSE

typedef struct conn_item connection_t;

#endif
struct conn_item connlist[1024] = { 0 };
void* client_thread(void* arg)
{
	int clientfd = *(int*)arg;
	while (1)
	{
		char buffer[1024];
		int count = recv(clientfd, buffer, sizeof(buffer) - 1, 0);
		if (count == 0)
		{
			printf("client closed\n");
			close(clientfd);
			return NULL;
		}
		send(clientfd, buffer, count, 0);
		printf("clientfd:%d count:%d buffer:%s\n", clientfd, count, buffer);
		

	}
}
int set_event(int fd, int event,int flag)
{
	if (flag == 1)//1添加，0修改
	{
		struct epoll_event ev;
		ev.events = event;
		ev.data.fd = fd;
		epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
	}
	else
	{
		struct epoll_event ev;
		ev.events = event;
		ev.data.fd = fd;
		epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
	}
	return 1;
}
int accept_cb(int fd)
{
	struct sockaddr_in clientaddr;
	socklen_t len = sizeof(clientaddr);
	int clientfd = accept(fd, (struct sockaddr*)&clientaddr, &len);
	if (clientfd < 0)
	{
		return -1;
	}
	connlist[clientfd].fd = clientfd;
	connlist[clientfd].rlen = 0;
	connlist[clientfd].wlen = 0;
	memset(connlist[clientfd].rbuffer, 0, BUFFER_SIZE);
	memset(connlist[clientfd].wbuffer, 0, BUFFER_SIZE);
	set_event(clientfd, EPOLLIN, 1);
	connlist[clientfd].recv_t.recv_callback = recv_cb;
	connlist[clientfd].send_callback = send_cb;
	printf("accept clientfd:%d\n", clientfd);
	return clientfd;
}
int recv_cb(int fd)
{
	char* buffer = connlist[fd].rbuffer;
	memset(connlist[fd].rbuffer, 0, BUFFER_SIZE);
	int count = recv(fd, buffer, BUFFER_SIZE, 0);
	if (count == 0)
	{
		connlist[fd].fd = 0;
		connlist[fd].rlen = 0;
		connlist[fd].wlen = 0;
		memset(connlist[fd].rbuffer, 0, BUFFER_SIZE);
		memset(connlist[fd].wbuffer,0,BUFFER_SIZE);
		epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
		close(fd);
		return -1;
	}
	connlist[fd].rlen = count;
#if 0	//echo need to send

	memcpy(connlist[fd].wbuffer, connlist[fd].rbuffer, connlist[fd].rlen);
	connlist[fd].wlen = connlist[fd].rlen;
	connlist[fd].rlen -= connlist[fd].rlen;
#else 
	kvstore_request(&connlist[fd]);
	connlist[fd].wlen = strlen(connlist[fd].wbuffer);
#endif
	set_event(fd, EPOLLOUT, 0);

	return count;
}
int send_cb(int fd)
{
	char* buffer = connlist[fd].wbuffer;
	int idx = connlist[fd].wlen;
	int count= send(fd, buffer,idx, 0);

	set_event(fd, EPOLLIN, 0);
	return count;
}



int epoll_entry(void)
{   //socket 绑定 监听
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in serveraddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(9096);
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	if(-1==bind(sockfd,(struct sockaddr*)&serveraddr,sizeof(serveraddr)))
	{
		printf("bind error\n");
		return -1;
	}
	listen(sockfd, 10);
	
	//epoll
	epfd = epoll_create(1);
	connlist[sockfd].recv_t.accept_callback = accept_cb;
	connlist[sockfd].fd = sockfd;
	set_event(sockfd, EPOLLIN, 1);

	struct epoll_event events[1024] = {0};
	while (1)
	{
		int nready = epoll_wait(epfd, events, 1024, -1);
		
		for (int i = 0; i < nready; ++i)
		{
			int connfd = events[i].data.fd;
			 if(events[i].events & EPOLLIN)
			 {
				 int count=connlist[connfd].recv_t.recv_callback(connfd);
				/* cout << "clientfd:" << connfd << " count:" << count << "buffer:" << connlist[connfd].rbuffer << endl;*/

			 }
			else if (events[i].events & EPOLLOUT)
			{
				 /*cout << "clientfd:" << connfd  << endl;*/
				 int count=connlist[connfd].send_callback(connfd);
			}
		}
	}


	getchar();
	return 0;
}

