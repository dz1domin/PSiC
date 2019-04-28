#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <string>
#include <netdb.h>
#include <time.h>
#include <sstream>
#include <poll.h>
#include <vector>
#include <map>
#include <fcntl.h>

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

std::string getTime (const char* format)
{
  time_t rawtime;
  struct tm* timeinfo;
  char output[40];

  time (&rawtime);
  timeinfo = localtime (&rawtime);

  std::stringstream formatString;
  formatString<<"server time: "<<format;
  strftime (output, sizeof(output), formatString.str().data(), timeinfo);

  return output;
}

void printClientConn(struct sockaddr_in* sin)
{
	char host[NI_MAXHOST], serv[NI_MAXSERV];
   	getnameinfo((struct sockaddr*)sin, sizeof(*sin), host, NI_MAXHOST, serv, NI_MAXSERV, 0);
   	std::cout << "adress=" << inet_ntoa(sin->sin_addr) << " (" << host << ") port=" << serv << " protocol=" << sin->sin_family << std::endl;
}

int main()
{
	unsigned int recv_size = sizeof(struct sockaddr_in);
	char buff[BUFF_SIZE] = {'\0'};
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	std::map<int, std::string> polled_data;
	std::map<int, struct sockaddr_in> polled_clients;

	if(sockfd == -1)
	{
		perror("socket() error");
		exit(1);
	}
	std::cout << "socket() status = " << sockfd << std::endl;
	setSocketToNonBlock(sockfd);

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");
	sin.sin_port = htons(65280);

	int jeden = 1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &jeden, sizeof(jeden)) == -1)
	{
		perror("setsockopt() error");
		exit(1);
	}

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

	struct pollfd fds;
	memset(&fds, 0, sizeof(fds));
	fds.fd = sockfd;
	fds.events = POLLIN;
	std::vector<struct pollfd> polling_cont;

	polling_cont.push_back(fds);

	while(1)
	{
		if(poll(polling_cont.data(), polling_cont.size(), -1) != -1)
		{
			for(unsigned i = 0; i < polling_cont.size(); i++)
			{
				if(i == 0 && polling_cont[i].revents & POLLIN)
				{
					// przychodzace polacznia
					struct sockaddr_in sin_inc;
					memset(&sin, 0, sizeof(sin_inc));
					const int client_sock = accept(sockfd, reinterpret_cast<struct sockaddr*>(&sin_inc), &recv_size);
					if(client_sock == -1)
					{
						perror("accept() error");
						continue;
					}
					std::cout << "accept() status = " << client_sock << std::endl;
					std::cout << "client connected from: ";
					polled_clients[client_sock] = sin_inc;
					printClientConn(&polled_clients[client_sock]);

					struct pollfd pfd_to_add;
					memset(&pfd_to_add, 0, sizeof(pfd_to_add));
					pfd_to_add.fd = client_sock;
					pfd_to_add.events = POLLIN;

					polling_cont.push_back(pfd_to_add);
				}
				
				if (i != 0 && polling_cont[i].revents & POLLIN)
				{
					// polaczenia klienckie - odbieranie
					const ssize_t data_recv = recv(polling_cont[i].fd, buff, BUFF_SIZE - 1, 0);
					if(data_recv == -1)
					{
						perror("recv() error");
						continue;
					}
					std::cout << "recv() status = " << data_recv << std::endl;

					polled_data[polling_cont[i].fd] = buff;

					for(auto& c : buff)
						c = '\0';

					// czyszczenie
					if(data_recv == 0)
					{
						close(polling_cont[i].fd);
						polled_data.erase(polling_cont[i].fd);
						polled_clients.erase(polling_cont[i].fd);
						polling_cont.erase(polling_cont.begin() + i);
						break;
					}

					std::cout << "received data: " << polled_data[polling_cont[i].fd] << std::endl;
					polling_cont[i].events = POLLOUT;
				}
				
				if(i != 0 && polling_cont[i].revents & POLLOUT)
				{
					// polaczenie klienckie - wysylanie
					polled_data[polling_cont[i].fd] = getTime(polled_data[polling_cont[i].fd].data());
					const int send_status = send(polling_cont[i].fd, polled_data[polling_cont[i].fd].data(), polled_data[polling_cont[i].fd].size(), 0);
					if(send_status == -1)
					{
						perror("send() error");
					}
					std::cout << "send() status = " << send_status << std::endl;
					std::cout << "sent data to: ";
					printClientConn(&(polled_clients[polling_cont[i].fd]));
					polling_cont[i].events = POLLIN;
				}
			}
		}
		else
		{
			perror("poll() error");
			exit(1);
		}
	}

	close(sockfd);

	return 0;
}