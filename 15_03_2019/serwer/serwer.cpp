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
#include <sstream>
#include <fstream>

constexpr int BUFF_SIZE = 1e5;
const string rootdir = "../Strona";
const string success = "HTTP/1.1 200 OK\r\n";
const string failure = "HTTP/1.1 404 Not Found\r\nContent-type:text/html;charset=utf-8\r\nContent-length:0\r\n\r\n";

vector<char> readFile (const char* path)
{
  using namespace std;

  vector<char> result;
  ifstream file_to_read(path, ios::in|ios::binary|ios::ate);
  file_to_read.exceptions( ifstream::failbit | ifstream::badbit );
  result.resize(file_to_read.tellg(), 0);
  file_to_read.seekg(0, ios::beg);
  file_to_read.read(result.data(), result.size());
  file_to_read.close();

  return result;
}

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

string getTime (const char* format)
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

string getSuffix(string& str)
{
	int i = 0;
	unsigned s = str.size();
	try
	{
		while(str.at(s - i - 1) != '.')
			i++;
		return str.substr(s - i, s);
	}
	catch(...)
	{
		return "";
	}
}

vector<char> parseData(string& data)
{
	std::istringstream is(data);
	string line, word, method, path, version;
	std::getline(is, line);
	std::istringstream is_line(line);
	std::getline(is_line, method, ' ');
	std::getline(is_line, path, ' ');
	std::getline(is_line, version, ' ');

	string test = data.substr(data.find("\r\n"), data.find("\r\n\r\n") - data.find("\r\n"));

	std::cout << test << std::endl;

	if(method == "POST")
	{
		string record, key, value;
		string fieldStorage = data.substr(data.find("\r\n\r\n") + 4, data.size());
		
		std::istringstream is_amp(fieldStorage);
		while(std::getline(is_amp, record, '&'))
		{
			key = record.substr(0, record.find("="));
			value = record.substr(record.find("=") + 1, record.size());
			std::cout << std::endl << key << ":" << value;
		}
		std::cout << std::endl;
		if(!key.size() && !value.size())
			std::cout << fieldStorage;

		vector<char> res;
		string headers;
		string file_to_read;
		if(path == "/")
			file_to_read = rootdir + "/index.html";
		else
			file_to_read = rootdir + path;

		string suffix = getSuffix(file_to_read);

		try
		{
			res = readFile(file_to_read.data());	
		}
		catch(...)
		{
			headers = failure;
			std::cerr << "exception trying to load: " << file_to_read << std::endl;
			perror("readFile() error");
			return vector<char>(headers.begin(), headers.end());
		}

		headers = success;
		headers += "Content-type:";

		if(suffix == "html")
			headers += "text/html;charset=utf-8\r\n";
		else if(suffix == "js")
			headers += "text/javascript\r\n";
		else if(suffix == "jpg")
			headers += "image/jpeg\r\n";
		else
			headers += "application/octet-stream\r\n";

		headers += "Content-length:";
		headers += std::to_string(res.size()) + "\r\n\r\n";
		vector<char> v(headers.begin(), headers.end());
		v.insert(v.end(), res.begin(), res.end());
		return v;
	}
	else if(method == "GET")
	{
		vector<char> res;
		string headers;
		string file_to_read;
		if(path == "/")
			file_to_read = rootdir + "/index.html";
		else
			file_to_read = rootdir + path;

		string suffix = getSuffix(file_to_read);

		try
		{
			res = readFile(file_to_read.data());	
		}
		catch(...)
		{
			headers = failure;
			std::cerr << "exception trying to load: " << file_to_read << std::endl;
			perror("readFile() error");
			return vector<char>(headers.begin(), headers.end());
		}

		headers = success;
		headers += "Content-type:";

		if(suffix == "html")
			headers += "text/html;charset=utf-8\r\n";
		else if(suffix == "js")
			headers += "text/javascript\r\n";
		else if(suffix == "jpg")
			headers += "image/jpeg\r\n";
		else
			headers += "text\r\n";

		headers += "Content-length:";
		headers += std::to_string(res.size()) + "\r\n\r\n";
		vector<char> v(headers.begin(), headers.end());
		v.insert(v.end(), res.begin(), res.end());
		return v;
	}
	string s = failure;
	return vector<char>(s.begin(), s.end());
}

int main()
{
	unsigned int recv_size = sizeof(struct sockaddr_in);
	char buff[BUFF_SIZE] = {'\0'};
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	std::map<int, string> polled_data;
	std::map<int, struct sockaddr_in> polled_clients;
	std::map<int, vector<char> > data_to_send;

	if(sockfd == -1)
	{
		perror("socket() error");
		exit(1);
	}
	// std::cout << "socket() status = " << sockfd << std::endl;
	setSocketToNonBlock(sockfd);

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");
	sin.sin_port = htons(65281);

	int jeden = 1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &jeden, sizeof(jeden)) == -1)
	{
		perror("setsockopt() error");
		exit(1);
	}

	if(bind(sockfd, reinterpret_cast<struct sockaddr*>(&sin), sizeof(sin)) == -1)
	{
		perror("bind() error");
		exit(1);
	}
	// std::cout << "bind() status = 0" << std::endl;

	if(listen(sockfd, SOMAXCONN) == -1)
	{
		perror("listen() error");
		exit(1);
	}
	// std::cout << "listen() status = 0" << std::endl;

	struct pollfd fds;
	memset(&fds, 0, sizeof(fds));
	fds.fd = sockfd;
	fds.events = POLLIN;
	vector<struct pollfd> polling_cont;

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
					// std::cout << "accept() status = " << client_sock << std::endl;
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
					int& client_fd = polling_cont[i].fd;
					string data = polled_data[client_fd];

					// polaczenia klienckie - odbieranie
					const ssize_t data_recv = recv(client_fd, buff, BUFF_SIZE - 1, 0);
					if(data_recv == -1)
					{
						perror("recv() error");
						continue;
					}
					// std::cout << "recv() status = " << data_recv << std::endl;

					data += buff;
					
					for(auto& c : buff)
						c = '\0';


					// czyszczenie
					if(data_recv == 0)
					{
						std::cout << "closing connection: ";
						printClientConn(&(polled_clients[client_fd]));
						close(client_fd);
						polled_data.erase(client_fd);
						polled_clients.erase(client_fd);
						data_to_send.erase(client_fd);
						polling_cont.erase(polling_cont.begin() + i);
						break;
					}

					// std::cout << "received data: " << data << std::endl;
					if(data.find("\r\n\r\n") != data.npos)
					{
						data_to_send[client_fd] = parseData(data);
						polling_cont[i].events = POLLOUT;
					}
				}
				
				if(i != 0 && polling_cont[i].revents & POLLOUT)
				{
					int& client_fd = polling_cont[i].fd;

					// polaczenie klienckie - wysylanie
					const int send_status = send(client_fd, data_to_send[client_fd].data(), data_to_send[client_fd].size(), 0);
					if(send_status == -1)
					{
						perror("send() error");
					}
					// std::cout << "send() status = " << send_status << std::endl;
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
