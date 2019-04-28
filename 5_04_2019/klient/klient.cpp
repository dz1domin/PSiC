#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <string>
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

void aes_decrypt(const byte key[KEY_SIZE], const byte iv[BLOCK_SIZE], const std::string& ctext, std::string& rtext)
{
    EVP_CIPHER_CTX_free_ptr ctx(EVP_CIPHER_CTX_new(), ::EVP_CIPHER_CTX_free);
    int rc = EVP_DecryptInit_ex(ctx.get(), EVP_aes_192_cfb(), NULL, key, iv);
    if (rc != 1)
      throw std::runtime_error("EVP_DecryptInit_ex failed");

    rtext.resize(ctext.size());
    int out_len1 = (int)rtext.size();

    rc = EVP_DecryptUpdate(ctx.get(), (byte*)&rtext[0], &out_len1, (const byte*)&ctext[0], (int)ctext.size());
    if (rc != 1)
      throw std::runtime_error("EVP_DecryptUpdate failed");
  
    int out_len2 = (int)rtext.size() - out_len1;
    rc = EVP_DecryptFinal_ex(ctx.get(), (byte*)&rtext[0]+out_len1, &out_len2);
    if (rc != 1)
      throw std::runtime_error("EVP_DecryptFinal_ex failed");

    rtext.resize(out_len1 + out_len2);
}

int main()
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

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

	const BIGNUM* pkey;
	auto dh = DH_get_2048_256();
	DH_generate_key(dh);
	unsigned char binkey[SHARED_SIZE];
	unsigned char shared[SHARED_SIZE];

	for(auto& el : binkey)
		el = 0;

	ssize_t recv_status = recv(sockfd, binkey, sizeof binkey, 0);
	if(recv_status == -1)
	{
		perror("recv() error");
		exit(1);
	}
	std::cout << "recv() status = " << recv_status << std::endl;

	BIGNUM* recvkey;
	recvkey = BN_bin2bn(binkey, sizeof binkey, NULL);
	DH_compute_key(binkey, recvkey, dh);

	for(unsigned i = 0; i < SHARED_SIZE; i++)
		shared[i] = binkey[i];

	DH_get0_key(dh, &pkey, NULL);
	BN_bn2bin(pkey, binkey);

	int send_status = send(sockfd, binkey, sizeof binkey, 0);
	if(send_status == -1)
	{
		perror("send() error");
		exit(1);
	}

	unsigned char hasz[MD5_DIGEST_LENGTH], rhasz[MD5_DIGEST_LENGTH];
	char buf[100] = { 0 };
	std::string cmessage;

	int r = 0;
	do
	{
		recv_status = recv(sockfd, rhasz, MD5_DIGEST_LENGTH, 0);
		if(recv_status == -1)
		{
			perror("recv() error");
			exit(1);
		}

		recv_status = recv(sockfd, buf, sizeof buf, 0);
		if(recv_status == -1)
		{
			perror("recv() error");
			exit(1);
		}
		cmessage = buf;
		std::cout << "received message: " << cmessage << std::endl;

		MD5((const unsigned char*)cmessage.data(), cmessage.size(), hasz);

		for(unsigned i = 0; i < MD5_DIGEST_LENGTH; i++)
		{
			if(rhasz[i] != hasz[i])
			{
				std::cerr << "HASZE SIE NIE ZGADZAJA !!!\n";
				r = 1;
				cmessage.clear();
				break;
			}
			else
				r = 0;
		}

		send_status = send(sockfd, "err", r * 4, 0);
		if(send_status == -1)
		{
			perror("send() error");
			exit(1);
		}
	}while(r);

	std::string rmessage;

	aes_decrypt(shared, shared + SHARED_SIZE - BLOCK_SIZE, cmessage, rmessage);

	std::cout << "decrypted message: " << rmessage << std::endl;
	
	close(sockfd);

	return 0;
}