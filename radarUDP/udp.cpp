#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include "udp.h"

int Socket_cmd;

int udp_init_cmd(char *ip, int port)
{
  struct sockaddr_in ser;    //服务器端地址

  Socket_cmd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);		//创建UDP套接字

  ser.sin_family = AF_INET;   						//初始化服务器地址信息
  ser.sin_port = htons(port);							//端口转换为网络字节序
  ser.sin_addr.s_addr = inet_addr(ip);				//IP地址转换为网络字节序

  if (Socket_cmd == -1)
  {
    printf("socket() Failed:%s\n", strerror(Socket_cmd));
    return -1;
  }

  if (bind(Socket_cmd, (struct sockaddr*)&ser, sizeof(ser)) == -1)
  {
    printf("Failed to bind the IP address and port\n");
    std::cerr << "Failed to bind socket: " << std::strerror(errno) << std::endl;
    return -1;
  }

  printf("udp_cmd init ok\n");
  return 0;
}

int udp_send(char *ip, int port, unsigned char *buff, int len)
{
  struct sockaddr_in s;
  int addrlen,ret;

  s.sin_family = AF_INET;
  s.sin_port = htons(port);
  s.sin_addr.s_addr = inet_addr(ip);
  addrlen = sizeof(s);

  ret = sendto(Socket_cmd, buff, len, 0, (struct sockaddr*)&s, addrlen);
  if (ret < 0)
  {
    printf("send failed\n");
    return ret;
  }
  return ret;
}

int udp_rcv(char *ip, int *port, unsigned char *buff, int *len)
{
  struct sockaddr_in s;
  int ret = 0;
  socklen_t addrlen = sizeof(s);

  ret = recvfrom(Socket_cmd, buff, 512, 0, (struct sockaddr*)&s, &addrlen);

  if (ret > 0)
  {
    memcpy(ip, inet_ntoa(s.sin_addr), strlen(inet_ntoa(s.sin_addr)));   //copy ip to from_ip
    *port = ntohs(s.sin_port);
  }

  *len = ret;

  return ret;
}


int close_udp()
{
  close(Socket_cmd);   //关闭套接字
  return 0;
}


char CheckSumChar(unsigned char *buf, short len)
{
  int sn = 0;
  char ret = 0;

  len = (len - 1) & 0xFF;
  for (sn = 2; sn < len; sn++)
  {
    ret += buf[sn];
  }

  return ret & 0xff;
}

int send_command(unsigned char *send_buf, unsigned short cmdcode)
{
  short cmd_len = 0, cmd_total_len = 0;
  char check_sum;
  int ret = 0;

  unsigned int ip[5] = { 0 };

  if (cmdcode == CLAINS_SET_IP_ADDR)
  {
    printf("请输入IP地址>>>>>>>>>>>>>>>>>>>>>>\n");
    //scanf_s("%x %x %x %x", &ip[0], &ip[1], &ip[2], &ip[3]);
    std::cin >> std::hex >> ip[0] >> ip[1] >> ip[2] >> ip[3];
    // 打印输入的IP地址
    std::cout << "输入的IP地址是：";
    for (int i = 0; i < 4; i++) {
      std::cout << std::hex << ip[i];
      if (i < 3) {
        std::cout << ".";
      }
    }
    std::cout << std::endl;

    cmd_len = 6;
    cmd_total_len = 2 + 2 + 2 + 4 + 1;

    send_buf[0] = (unsigned char)(CLAINS_CMD_HEADER >> 8);
    send_buf[1] = (unsigned char)(CLAINS_CMD_HEADER & 0xff);
    send_buf[2] = (unsigned char)((cmd_total_len - 5) & 0xff);
    send_buf[3] = (unsigned char)((cmd_total_len - 5) >> 8);
    send_buf[4] = (unsigned char)(cmdcode >> 8);
    send_buf[5] = (unsigned char)(cmdcode & 0xff);
    send_buf[6] = (unsigned char)(ip[0]);
    send_buf[7] = (unsigned char)(ip[1]);
    send_buf[8] = (unsigned char)(ip[2]);
    send_buf[9] = (unsigned char)(ip[3]);
    check_sum = CheckSumChar(send_buf, cmd_total_len);
    send_buf[10] = (unsigned char)check_sum;
  }
  else
  {
    cmd_len = 2;
    cmd_total_len = 2 + 2 + 2 + 1;

    send_buf[0] = (unsigned char)(CLAINS_CMD_HEADER >> 8);
    send_buf[1] = (unsigned char)(CLAINS_CMD_HEADER & 0xff);
    send_buf[2] = (unsigned char)((cmd_total_len - 5) & 0xff);
    send_buf[3] = (unsigned char)((cmd_total_len - 5) >> 8);
    send_buf[4] = (unsigned char)(cmdcode >> 8);
    send_buf[5] = (unsigned char)(cmdcode & 0xff);
    check_sum = CheckSumChar(send_buf, cmd_total_len);
    send_buf[6] = (unsigned char)check_sum;
  }


  ret = udp_send("192.168.53.11", 20000, send_buf, cmd_total_len);  //此处雷达IP地址，根据自己设备IP而定

  return ret;
}




























