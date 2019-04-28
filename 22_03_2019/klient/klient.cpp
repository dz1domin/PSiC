#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <string>
#include <poll.h>
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <random>
#include <climits>

constexpr unsigned int BUFF_SIZE = 512;

struct DNSheader
{
	uint16_t ID;
	uint16_t RCODE:4;
	uint16_t Z:3;
	uint16_t RA:1;
	uint16_t RD:1;
	uint16_t TC:1;
	uint16_t AA:1;
	uint16_t OPCODE:4;
	uint16_t QR:1;
	uint16_t QDCOUNT;
	uint16_t ANCOUNT;
	uint16_t NSCOUNT;
	uint16_t ARCOUNT;

	friend std::ostream& operator<<(std::ostream& o, const DNSheader& h)
	{
		o << "ID: " << h.ID << "\nQR: " << (int)h.QR << "\nOPCODE: " << (int)h.OPCODE << "\nAA: " << (int)h.AA
			<< "\nTC: " << (int)h.TC << "\nRD: " << (int)h.RD << "\nRA: " << h.RA << "\nZ: " << (int)h.Z
			<< "\nRCODE: " << (int)h.RCODE << "\nQDCOUNT: " << h.QDCOUNT << "\nANCOUNT: " << h.ANCOUNT
			<< "\nNSCOUNT: " << h.NSCOUNT << "\nARCOUNT: " << h.ARCOUNT << std::endl;
		return o;
	}
};

struct DNSpacket
{
	DNSheader head;
	unsigned char body[BUFF_SIZE - sizeof(DNSheader)];
};

void parseResponse(DNSpacket& p)
{
	uint16_t* ptr = (uint16_t*)&p;
	for(unsigned int i = 0; i < 6; i++)
		ptr[i] = ntohs(ptr[i]);

	std::cout << p.head;

	// TODO: dokonczyc przetwarzenie odpowiedzi DNS
}

DNSpacket makeDNSquery(std::string hostname)
{
	std::random_device r;
    std::mt19937 e1(r());
    std::uniform_int_distribution<int> uniform_dist(0, USHRT_MAX);
	DNSpacket p;
	memset(&p, 0, sizeof(p));

	p.head.ID = uniform_dist(e1); // bedzie random
	p.head.RD = 1;
	p.head.QDCOUNT = 1;

	std::string part;
	std::istringstream i(hostname);
	int itr = 0;

	// robienie ciala pakietu
	while(std::getline(i, part, '.'))
	{
		p.body[itr++] = part.size();
		for(auto& el : part)
			p.body[itr++] = el;
	}
	
	p.body[itr] = 0;

	uint16_t* pp = (uint16_t*)(p.body + itr);

	if(itr % 2)
		pp += 2;
	else
		pp += 1;

	pp[0] = 1;
	pp[1] = 1;

	uint16_t* ptr = (uint16_t*)&p;
	for(unsigned int i = 0; i < 6; i++)
		ptr[i] = htons(ptr[i]);

	return p;
}

unsigned int get_resolver_ip()
{
	std::ifstream ifs("/etc/resolv.conf");

	if (ifs.is_open())
	{
		std::string line;

		while (std::getline(ifs, line))
		{
			std::istringstream iss(line);
			std::string token;
			if (std::getline(iss, token, ' '))
			{
				if (token == "nameserver")
				{
					if (std::getline(iss, token))
					{
						return inet_addr(token.c_str());
					}
				}
			}
		}
	}

	return 0;
}

int main(int argc, char** argv)
{
	DNSpacket buff;
	memset(&buff, 0, sizeof(buff));

	if(argc != 2)
	{
		std::cerr << "Zla liczba argumentow\n";
		exit(1);
	}

	std::string hostname = argv[1];
	int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(sockfd == -1)
	{
		perror("socket() error");
		exit(1);
	}

	struct timeval tv{5,0};
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1)
	{
		perror("setsockopt() error");
		exit(1);
	}

	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = get_resolver_ip();
	sin.sin_port = htons(53);

	DNSpacket dgram = makeDNSquery(hostname);

	int send_status = sendto(sockfd, &dgram, sizeof(dgram), 0,
							reinterpret_cast<struct sockaddr*>(&sin), sizeof(sin));
	if(send_status == -1)
	{
		perror("send() error");
		exit(1);
	}

	ssize_t recv_s = recv(sockfd, &buff, sizeof(buff), 0);
	if(recv_s == -1)
	{
		perror("recv() error");
		exit(1);
	}

	parseResponse(buff);
}