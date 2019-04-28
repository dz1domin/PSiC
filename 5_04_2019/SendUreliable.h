#include <iostream>
#include <vector>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>

void modifyText (unsigned char* text)
{
  static bool seedInit = false;
  if (!seedInit) {
    srand (time(NULL));
    // srand (1);
    seedInit = true;
  }

  const int value = rand()%10;
  if (value < 5) {
    text[0] = 1;
    text[1] = 2;
    text[2] = 3;
    std::cout<<"Incorrect messsage sent."<<std::endl;
  }
  else
    std::cout<<"Good messsage sent."<<std::endl;
}

void sendUnreliable (const int sock, unsigned char* data, const unsigned int size)
{
  std::vector<unsigned char> toSend;
  toSend.insert(toSend.end(), data, data+size);

  modifyText (toSend.data());
  send (sock, toSend.data(), toSend.size(), 0);
}
