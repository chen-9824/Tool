//
// Created by cfan on 2023/5/31.
//

#include "client.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

int client() {
  int clientSocket;
  struct sockaddr_in serverAddress;
  char buffer[1024];

  // 创建UDP套接字
  clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (clientSocket < 0) {
    std::cerr << "Failed to create socket." << std::endl;
    return -1;
  }

  // 设置服务器地址
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(8888);
  if (inet_pton(AF_INET, "127.0.0.1", &(serverAddress.sin_addr)) <= 0) {
    std::cerr << "Failed to setup server address." << std::endl;
    return -1;
  }

  while (true) {
    std::cout << "Enter a message to send (or 'q' to quit): ";
    std::cin.getline(buffer, sizeof(buffer));

    if (strcmp(buffer, "q") == 0) {
      break;
    }

    // 向服务器发送消息
    if (sendto(clientSocket, buffer, strlen(buffer), 0,
               (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
      std::cerr << "Failed to send data." << std::endl;
      return -1;
    }

    // 接收服务器的响应
    /*int bytesRead = recvfrom(clientSocket, buffer, sizeof(buffer), 0, nullptr, nullptr);
    if (bytesRead < 0) {
      std::cerr << "Failed to receive data." << std::endl;
      return -1;
    }

    // 在接收到的消息前添加一个结束符
    buffer[bytesRead] = '\0';

    std::cout << "Server response: " << buffer << std::endl;*/
  }

  // 关闭套接字
  close(clientSocket);

  return 0;
}

int main() {
  int clientSocket;
  struct sockaddr_in serverAddress;
  char buffer[1024];

  // 创建UDP套接字
  clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
  if (clientSocket < 0) {
    std::cerr << "Failed to create socket." << std::endl;
    return -1;
  }

  // 设置服务器地址
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(8888);
  if (inet_pton(AF_INET, "127.0.0.1", &(serverAddress.sin_addr)) <= 0) {
    std::cerr << "Failed to setup server address." << std::endl;
    return -1;
  }

  while (true) {
    std::cout << "Enter a message to send (or 'q' to quit): ";
    std::cin.getline(buffer, sizeof(buffer));

    if (strcmp(buffer, "q") == 0) {
      break;
    }

    // 向服务器发送消息
    if (sendto(clientSocket, buffer, strlen(buffer), 0,
               (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
      std::cerr << "Failed to send data." << std::endl;
      return -1;
    }

    // 接收服务器的响应
    int bytesRead = recvfrom(clientSocket, buffer, sizeof(buffer), 0, nullptr, nullptr);
    if (bytesRead < 0) {
      std::cerr << "Failed to receive data." << std::endl;
      return -1;
    }

    // 在接收到的消息前添加一个结束符
    buffer[bytesRead] = '\0';

    std::cout << "Server response: " << buffer << std::endl;
  }

  // 关闭套接字
  close(clientSocket);

  return 0;
}
