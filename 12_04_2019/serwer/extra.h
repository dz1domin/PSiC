#pragma once

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
#include <openssl/ssl.h>
#include <openssl/err.h>

using std::vector;
using std::map;
using std::string;

const string rootdir = "../Strona";
const string success = "HTTP/1.1 200 OK\r\n";
const string success_with_content = "HTTP/1.1 200 OK\r\nContent-length:0\r\n\r\n";
const string failure = "HTTP/1.1 404 Not Found\r\nContent-type:text/html;charset=utf-8\r\nContent-length:0\r\n\r\n";

std::string getSuffix(std::string& str);

string parseData(std::string& data);

vector<char> readFile (const char* path);