#include "extra.h"

void setSocketToNonBlock(const int sockfd)
{
	int flags = fcntl(sockfd, F_GETFL);
	if(flags == -1)
	{
		perror("fcntl() error");
		exit(1);
	}
	flags = flags | O_NONBLOCK;
	flags = fcntl(sockfd, F_SETFL, flags);
	if(flags == -1)
	{
		perror("fcntl() error");
		exit(1);
	}
}

int create_socket(int port)
{
    int s;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s < 0) {
	perror("Unable to create socket");
	exit(EXIT_FAILURE);
    }

    setSocketToNonBlock(s);

    int jeden = 1;
	if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &jeden, sizeof(jeden)) == -1)
	{
		perror("setsockopt() error");
		exit(1);
	}

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
	perror("Unable to bind");
	exit(EXIT_FAILURE);
    }

    if (listen(s, 1) < 0) {
	perror("Unable to listen");
	exit(EXIT_FAILURE);
    }

    return s;
}

void init_openssl()
{ 
    SSL_load_error_strings();	
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl()
{
    EVP_cleanup();
}

SSL_CTX *create_context()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();

    ctx = SSL_CTX_new(method);
    if (!ctx) {
	perror("Unable to create SSL context");
	ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX *ctx)
{
    SSL_CTX_set_ecdh_auto(ctx, 1);

    /* Set the key and cert */
    if (SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM) <= 0 ) {
        ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
    }
}

int main()
{
    int sock;
    SSL_CTX *ctx;

    init_openssl();
    ctx = create_context();

    configure_context(ctx);

    sock = create_socket(12340);
    map<int, SSL*> connected_clients;
    map<int, string> client_data;
    map<int, string> server_response;
    vector<struct pollfd> polling_cont;
    struct pollfd serv;
    memset(&serv, 0, sizeof serv);
    serv.fd = sock;
    serv.events = POLLIN;

    polling_cont.push_back(serv);

    /* Handle connections */
    while(1) 
    {
    	if(poll(polling_cont.data(), polling_cont.size(), -1) != -1)
    	{
    		for(unsigned i = 0; i < polling_cont.size(); i++)
    		{
    			if (i == 0 && polling_cont[i].revents & POLLIN)
			    {
				   	struct sockaddr_in addr;
				    uint len = sizeof(addr);
				    SSL *ssl;

				    int client = accept(sock, (struct sockaddr*)&addr, &len);
				    if (client < 0) {
				        perror("Unable to accept");
				        continue;
				    }
				    std::cout << "client accept = " << client << std::endl;

				    setSocketToNonBlock(client);

				    ssl = SSL_new(ctx);
				    int ret = SSL_set_fd(ssl, client);
				    if(!ret)
				    {
				    	std::cout << "set fd failed" << std::endl;
				    	continue;
				    }

				    SSL_set_accept_state(ssl);

				    connected_clients[client] = ssl;

				    struct pollfd client_fd;
				    memset(&client_fd, 0, sizeof client_fd);
				    client_fd.fd = client;
				    client_fd.events = POLLIN | POLLOUT;
				    polling_cont[0].events = POLLIN;

				    polling_cont.push_back(client_fd);
				    break;
				}
				else if(i != 0 && (polling_cont[i].revents & (POLLIN | POLLOUT)))
				{
					int client = polling_cont[i].fd;
					char buf[1024] = { '\0' };
					polling_cont[i].events = 0;
					///////////////////////////////////////////////////////////////////////////
					int result;

					if  ((!server_response.count(client) || !server_response[client].size()) || (polling_cont[i].revents & POLLIN))
					{
						do
						{
							result = SSL_read(connected_clients[client], buf, sizeof(buf) - 1);

							if(result > 0 && result < sizeof buf)
								client_data[client] += buf;
						}
						while(SSL_pending(connected_clients[client]));

						if(client_data[client].find("\r\n\r\n") != string::npos)
						{
							server_response[client] = parseData(client_data[client]);

							client_data[client].clear();
						}

						if(result == 0)
						{
							std::cout << "closing " << client << std::endl;
							close(client);
							SSL_free(connected_clients[client]);
							client_data.erase(client);
							polling_cont.erase(polling_cont.begin() + i);
							break;
						}
					}

					if ((server_response.count(client) && server_response[client].size() > 0) || (polling_cont[i].revents & POLLOUT))
					{
						result = SSL_write(connected_clients[client], server_response[client].data(), server_response[client].size());

						if(result > 0 && result <= server_response[client].size())
							server_response[client].erase(0, result);

						if(!server_response[client].size())
						{
							SSL_write(connected_clients[client], success_with_content.data(), success_with_content.size());
							server_response.erase(client);
							// std::cout << "closing(write done) " << client << std::endl;
							// close(client);
							// SSL_free(connected_clients[client]);
							// client_data.erase(client);
							// polling_cont.erase(polling_cont.begin() + i);
							// break;
						}
					}

					auto err = SSL_get_error(connected_clients[client], result);
					if (err == SSL_ERROR_WANT_WRITE)	
						polling_cont[i].events = POLLOUT;
					else if(err == SSL_ERROR_WANT_READ)
						polling_cont[i].events = POLLIN;
					else
						polling_cont[i].events = POLLIN | POLLOUT;
				}
			}
		}
		else
		{
			perror("poll() error");
			exit(1);
		}
    }

    close(sock);
    SSL_CTX_free(ctx);
    cleanup_openssl();
}