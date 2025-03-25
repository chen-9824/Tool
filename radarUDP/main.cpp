#include <cstdio>
#include <cstdlib>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <pthread.h>

#include <sstream>
#include "udp.h"

pthread_t h1;
//extern int Socket_cmd;

void *Thread1(void *arg)
{
  unsigned char buff[1024] = { 0 };
  int rcv_len,i,ret = 0,port;
  char ip[25] = { 0 };
  struct sockaddr_in s;
  int addrlen,target_len,target_num;
  addrlen = sizeof(s);

  char target_sn = 0;
  double range_x = 0.0, range_y = 0.0,speed = 0.0;
  short fudu = 0, snr = 0;
  sleep(1);
  while (1)
  {
    memset(buff, 0, sizeof(buff));

    ret = udp_rcv(ip, &port, buff, &rcv_len);
    printf("ret = %d\n", ret);

    //根据回复字节数判断回复的数据
    if (ret == 7)
    {
      printf("心跳包\n");
      for (i = 0; i < 7; i++)
      {
        printf("buff[%d] = %#x\n", i, buff[i]);
      }
      printf("---------------------------------------------------------\n");
    }
    else if (ret >= 22)
    {
      //目标信息
      target_len = (buff[3] << 8) + buff[2];
      rcv_len = target_len + 5;
      target_num = target_len / 17;    //一个目标信息长度为17字节
      printf("共有%d个目标\n", target_num);
      for (i = 0; i < target_num; i++)
      {
        target_sn = buff[6+i*17];
        printf("目标%d的编号 = %d\n", i+1, target_sn);

        range_y = (unsigned short)((buff[8 + i*17] << 8) + buff[7 + i*17])*0.1;
        printf("目标%d的纵向距离 = %.f\n", i+1, range_y);

        range_x = (short)((buff[10 + i*17] << 8) + buff[9 + i*17])*0.1;
        printf("目标%d的横向距离 = %.1f\n", i+1, range_x);

        speed = (short)((buff[12 + i*17] << 8) + buff[11 + i*17])*0.1;
        printf("目标%d的速度 = %.1f\n", i+1, speed);

        fudu = buff[13 + i*17];
        printf("目标%d的幅度 = %d\n", i+1, fudu);

        snr = buff[14 + i*17];
        printf("目标%d的信噪比 = %d\n", i+1, snr);

        printf("---------------------------------------------------------------------------------\n");
      }
    }
    else
    {	//指令回复
      printf("一应一答指令回复\n");
      for (i = 0; i < ret; i++)
      {
        printf("buff[%d] = %#x\n", i, buff[i]);
      }
      printf("---------------------------------------------------------------------------------\n");
    }
  }
}

int udp_test()
{
  unsigned char buff[1024] = { 0 };
  int rcv_len;
  char ip[25] = { 0 };
  int port,ret;
  unsigned char send_buf[552] = {0};
  int i = 0;
  unsigned short cmdcode;
  //char cmd[2] = { 0 };

  std::string cmd;

  printf("udp_test\n");

  udp_init_cmd("192.168.53.17", 30000);				 //此处Ip地址和端口号根据自己实际情况而定

  while (1)
  {
    printf("请输入指令码>>>>>>>>>>>>>>>>>>\n");
    std::getline(std::cin, cmd);
    std::stringstream ss(cmd);
    ss >> std::hex >> cmdcode;
    std::cout << "输入的指令码为：" << std::hex << cmdcode << std::endl;
    switch (cmdcode)
    {
      case CLAINS_FIRMWARE_VERSION_NUM_R:
        ret = send_command(send_buf, cmdcode);
        break;
      case CLAINS_TARGET_INFOR_OUTPUT:
        ret = send_command(send_buf, cmdcode);
        break;
      case CLAINS_PAUSE_OUTPUT:
        ret = send_command(send_buf, cmdcode);
        break;
      case CLAINS_SET_IP_ADDR:
        ret = send_command(send_buf, cmdcode);
        break;
      case CLAINS_GET_IP_ADDR:
        ret = send_command(send_buf, cmdcode);
        break;
      default:
        printf("The input command code is incorrect\n");
        break;
    }
  }

  return 0;
}

int main(int argc, char *argv[])
{
  int res = pthread_create(&h1, nullptr, Thread1, nullptr);
  udp_test();

  return 0;
}
