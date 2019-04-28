#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <string>
#include <netdb.h>

void printClientConn(struct sockaddr_in* sin)
{
	char host[NI_MAXHOST], serv[NI_MAXSERV];
   	getnameinfo((struct sockaddr*)sin, sizeof(*sin), host, NI_MAXHOST, serv, NI_MAXSERV, 0);
   	std::cout << "adress=" << inet_ntoa(sin->sin_addr) << " (" << host << ") port=" << serv << " protocol=" << sin->sin_family << std::endl;
}

int main()
{
	unsigned int recv_size = sizeof(struct sockaddr_in);
	char buff[50] = {'\0'};
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(!sockfd)
	{
		perror("socket() error");
		exit(1);
	}
	std::cout << "socket() status = " << sockfd << std::endl;

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");
	sin.sin_port = htons(65280);

	if(bind(sockfd, (struct sockaddr*)&sin, sizeof(sin)) == -1)
	{
		perror("bind() error");
		exit(1);
	}
	std::cout << "bind() status = 0" << std::endl;

	if(listen(sockfd, SOMAXCONN) == -1)
	{
		perror("listen() error");
		exit(1);
	}
	std::cout << "listen() status = 0" << std::endl;

	int sockfd2 = accept(sockfd, (struct sockaddr*)&sin, &recv_size);
	if(sockfd2 == -1)
	{
		perror("accept() error");
		exit(1);
	}
	std::cout << "accept() status = " << sockfd2 << std::endl;
	printClientConn(&sin);
	

	ssize_t recv_s = recv(sockfd2, buff, sizeof(buff) - 1, 0);
	if(recv_s == -1)
	{
		perror("recv() error");
		exit(1);
	}
	std::cout << "recv() status = " << recv_s << std::endl;

	std::cout << buff << std::endl;
	std::string s = "Re: " + static_cast<std::string>(buff);

	int send_status = send(sockfd2, s.data(), s.size(), 0);
	if(send_status == -1)
	{
		perror("send() error");
		exit(1);
	}
	std::cout << "send() status = " << send_status << std::endl;

	close(sockfd);
	close(sockfd2);

	return 0;
}