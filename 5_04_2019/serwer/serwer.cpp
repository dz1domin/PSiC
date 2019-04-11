#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <string>
#include <netdb.h>
#include "../SendUreliable.h"
#include <openssl/evp.h>
#include <openssl/dh.h>
#include <openssl/bn.h>
#include <openssl/md5.h>
#include <memory>

typedef unsigned char byte;
using EVP_CIPHER_CTX_free_ptr = std::unique_ptr<EVP_CIPHER_CTX, decltype(&::EVP_CIPHER_CTX_free)>;
constexpr unsigned int SHARED_SIZE = 256;
constexpr unsigned int KEY_SIZE = 24;
constexpr unsigned int BLOCK_SIZE = 16;

void printClientConn(struct sockaddr_in* sin)
{
	char host[NI_MAXHOST], serv[NI_MAXSERV];
   	getnameinfo((struct sockaddr*)sin, sizeof(*sin), host, NI_MAXHOST, serv, NI_MAXSERV, 0);
   	std::cout << "adress=" << inet_ntoa(sin->sin_addr) << " (" << host << ") port=" << serv << " protocol=" << sin->sin_family << std::endl;
}

void aes_encrypt(const byte key[KEY_SIZE], const byte iv[BLOCK_SIZE], const std::string& ptext, std::string& ctext)
{
    EVP_CIPHER_CTX_free_ptr ctx(EVP_CIPHER_CTX_new(), ::EVP_CIPHER_CTX_free);
    int rc = EVP_EncryptInit_ex(ctx.get(), EVP_aes_192_cfb(), NULL, key, iv);
    if (rc != 1)
      throw std::runtime_error("EVP_EncryptInit_ex failed");

    ctext.resize(ptext.size()+BLOCK_SIZE);
    int out_len1 = (int)ctext.size();

    rc = EVP_EncryptUpdate(ctx.get(), (byte*)&ctext[0], &out_len1, (const byte*)&ptext[0], (int)ptext.size());
    if (rc != 1)
      throw std::runtime_error("EVP_EncryptUpdate failed");
  
    int out_len2 = (int)ctext.size() - out_len1;
    rc = EVP_EncryptFinal_ex(ctx.get(), (byte*)&ctext[0]+out_len1, &out_len2);
    if (rc != 1)
      throw std::runtime_error("EVP_EncryptFinal_ex failed");

    ctext.resize(out_len1 + out_len2);
}

int main()
{
	unsigned int recv_size = sizeof(struct sockaddr_in);
	int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(!sockfd)
	{
		perror("socket() error");
		exit(1);
	}
	std::cout << "socket() status = " << sockfd << std::endl;

	int jeden = 1;
	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &jeden, sizeof(jeden)) == -1)
	{
		perror("setsockopt() error");
		exit(1);
	}

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

	const BIGNUM* pkey;
	auto dh = DH_get_2048_256();
	DH_generate_key(dh);
	unsigned char binkey[SHARED_SIZE];
	unsigned char shared[SHARED_SIZE];

	for(auto& el : binkey)
		el = 0;

	DH_get0_key(dh, &pkey, NULL);
	BN_bn2bin(pkey, binkey);

	int send_status = send(sockfd2, binkey, sizeof binkey, 0);
	if(send_status == -1)
	{
		perror("send() error");
		exit(1);
	}

	int recv_status = recv(sockfd2, binkey, sizeof binkey, 0);
	if(recv_status == -1)
	{
		perror("recv() error");
		exit(1);
	}

	BIGNUM* recvkey;
	recvkey = BN_bin2bn(binkey, sizeof binkey, NULL);

	DH_compute_key(binkey, recvkey, dh);

	for(unsigned i = 0; i < SHARED_SIZE; i++)
		shared[i] = binkey[i];

	std::string pmessage = "To be or not to be";
	std::string cmessage;

	aes_encrypt(shared, shared + SHARED_SIZE - BLOCK_SIZE, pmessage, cmessage);

	unsigned char hasz[MD5_DIGEST_LENGTH];

	MD5((const unsigned char*)cmessage.data(), cmessage.size(), hasz);

	std::cout << "encrypted message: " << cmessage << std::endl;

	int r = 0;
	unsigned char t[4];
	do
	{
		send_status = send(sockfd2, hasz, MD5_DIGEST_LENGTH, 0);
		if(send_status == -1)
		{
			perror("send() error");
			exit(1);
		}

		sendUnreliable(sockfd2, (unsigned char*)cmessage.data(), cmessage.size());

		r = recv(sockfd2, t, 4, 0);
		if(recv_status == -1)
		{
			perror("recv() error");
			exit(1);
		}
	}while(r == 4);

	close(sockfd);
	close(sockfd2);

	return 0;
}