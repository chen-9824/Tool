//
// Created by cfan on 2023/5/31.
//

#include "server.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
int server() {
  int serverSocket;
  struct sockaddr_in serverAddress, clientAddress;
  char buffer[1024];

  // 创建UDP套接字
  serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (serverSocket < 0) {
    std::cerr << "Failed to create socket." << std::endl;
    return -1;
  }

  // 设置服务器地址
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  serverAddress.sin_port = htons(8888);

  // 将套接字绑定到服务器地址
  if (bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
    std::cerr << "Failed to bind socket." << std::endl;
    return -1;
  }

  socklen_t clientAddressLength = sizeof(clientAddress);

  while (true) {
    // 接收客户端消息
    int bytesRead = recvfrom(serverSocket, buffer, sizeof(buffer), 0,
                             (struct sockaddr *) &clientAddress, &clientAddressLength);
    if (bytesRead < 0) {
      std::cerr << "Failed to receive data." << std::endl;
      return -1;
    }

    // 在接收到的消息前添加一个结束符
    buffer[bytesRead] = '\0';

    std::cout << "Received message from client: " << buffer << std::endl;

    // 向客户端发送响应
    const char *response = "Message received.";
    sendto(serverSocket, response, strlen(response), 0,
           (struct sockaddr *) &clientAddress, sizeof(clientAddress));
  }

  // 关闭套接字
  close(serverSocket);

  return 0;
}

int main() {
  int serverSocket;
  struct sockaddr_in serverAddress, clientAddress;
  char buffer[1024];

  // 创建UDP套接字
  serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (serverSocket < 0) {
    std::cerr << "Failed to create socket." << std::endl;
    return -1;
  }

  // 设置服务器地址
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  serverAddress.sin_port = htons(20000);

  // 将套接字绑定到服务器地址
  if (bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
    std::cerr << "Failed to bind socket." << std::endl;
    return -1;
  }

  socklen_t clientAddressLength = sizeof(clientAddress);

  while (true) {
    // 接收客户端消息
    int bytesRead = recvfrom(serverSocket, buffer, sizeof(buffer), 0,
                             (struct sockaddr *) &clientAddress, &clientAddressLength);
    if (bytesRead < 0) {
      std::cerr << "Failed to receive data." << std::endl;
      return -1;
    }

    // 在接收到的消息前添加一个结束符
    buffer[bytesRead] = '\0';

    std::cout << "Received message from client: " << buffer << std::endl;

    // 向客户端发送响应
    const char *response = "Message received.";
    sendto(serverSocket, response, strlen(response), 0,
           (struct sockaddr *) &clientAddress, sizeof(clientAddress));
  }

  // 关闭套接字
  close(serverSocket);

  return 0;
}