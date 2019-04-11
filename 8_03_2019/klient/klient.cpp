#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <string>
#include <poll.h>
#include <fcntl.h>
#include <chrono> 
#include <thread> 

constexpr int BUFF_SIZE = 128;

void setSocketToNonBlock(const int sockfd)
{
	int flags = fcntl(sockfd, F_GETFL, 0);
	if(flags == -1)
	{
		perror("fcntl() error");
		exit(1);
	}

	flags = fcntl(sockfd, F_SETFL, flags);
	if(flags == -1)
	{
		perror("fcntl() error");
		exit(1);
	}
}

int main(int argc, char** argv)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	char buff[BUFF_SIZE] = {'\0'};
	int port_num, reps;
	double interval;
	std::string str;

	if(argc == 5)
	{
		port_num = std::stoi(argv[1]); 
		str = argv[2];
		interval = std::stod(argv[3]);
		reps = std::stoi(argv[4]);
	}
	else
	{
		std::cerr << "Wrong input, 4 values required";
		exit(1);
	}

	if(sockfd == -1)
	{
		perror("socket() error");
		exit(1);
	}
	std::cout << "socket status = " << sockfd << std::endl;
	setSocketToNonBlock(sockfd);

	int jeden = 1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &jeden, sizeof(jeden)) == -1)
	{
		perror("setsockopt() error");
		exit(1);
	}

	struct sockaddr_in sin_client;
	memset(&sin_client, 0, sizeof(sin_client));
	sin_client.sin_family = AF_INET;
	sin_client.sin_addr.s_addr = inet_addr("127.0.0.1");
	sin_client.sin_port = htons(port_num);

	if(bind(sockfd, reinterpret_cast<struct sockaddr*>(&sin_client), sizeof(sin_client)) == -1)
	{
		perror("bind() error");
		exit(1);
	}
	std::cout << "bind() status = 0" << std::endl;

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

	struct pollfd fds;
	memset(&fds, 0, sizeof(fds));
	fds.fd = sockfd;
	fds.events = POLLOUT;

	if(poll(&fds, 1, -1) == -1)
	{
		perror("poll() error");
		exit(1);
	}

	while(reps + 1)
	{
		if(poll(&fds, 1, -1) != -1)
		{
			if(fds.revents & POLLOUT)
			{
				int send_status = send(sockfd, str.data(), str.size(), 0);
				if(send_status == -1)
				{
					perror("send() error");
					exit(1);
				}
				std::cout << "send() status = " << send_status << std::endl;
				fds.events = POLLIN;
			}

			if(fds.revents & POLLIN)
			{
				ssize_t recv_s = recv(sockfd, buff, sizeof(buff) - 1, 0);
				if(recv_s == -1)
				{
					perror("recv() error");
					exit(1);
				}
				else if (recv_s == 0 && reps == 0)
				{
					close(sockfd);
				}

				std::cout << "recv() status = " << recv_s << std::endl;
		
				std::cout << buff << std::endl;
				reps--;
				fds.events = POLLOUT;
				std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(1000 * interval)));
			}
		}
		else
		{
			perror("poll() error");
			exit(1);
		}
	}
	return 0;
}