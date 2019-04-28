#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>

int main()
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	char buff[50] = "Dziala!";

	if(sockfd == -1)
	{
		perror("socket() error");
		exit(1);
	}
	std::cout << "socket status = " << sockfd << std::endl;

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");
	sin.sin_port = htons(65280);

	if(connect(sockfd, reinterpret_cast<struct sockaddr*>(&sin), sizeof(sin)) == -1)
	{
		perror("connect() error");
		exit(1);
	}
	std::cout << "connect() status = 0" << std::endl; 

	int send_status = send(sockfd, buff, sizeof(buff) - 1, 0);
	if(send_status == -1)
	{
		perror("send() error");
		exit(1);
	}
	std::cout << "send() status = " << send_status << std::endl;

	ssize_t recv_s = recv(sockfd, buff, sizeof(buff) - 1, 0);
	if(recv_s == -1)
	{
		perror("recv() error");
		exit(1);
	}
	std::cout << "recv() status = " << recv_s << std::endl;

	std::cout << buff << std::endl;

	close(sockfd);

	return 0;
}