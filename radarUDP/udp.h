#ifndef __UDP_H
#define __UDP_H


#define CLAINS_CMD_HEADER				0x55AA

//Radar command code
#define CLAINS_FIRMWARE_VERSION_NUM_R   0x0002				//读取雷达软件版本号
#define CLAINS_PAUSE_OUTPUT             0x0008				//暂停数据输出
#define CLAINS_TARGET_INFOR_OUTPUT      0x000b				//配置雷达输出目标信息
#define CLAINS_SET_IP_ADDR				0x0055				//设置IP
#define CLAINS_GET_IP_ADDR				0x0056				//获取IP

int udp_init_cmd(char *ip, int port);
int udp_init_data(char *ip, int port);
int udp_send(char *ip, int port, unsigned char *buff, int len);
int udp_rcv(char *ip, int *port, unsigned char *buff, int *len);
int close_udp();
int send_command(unsigned char *send_buf, unsigned short cmdcode);
#endif