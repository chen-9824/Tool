//
// Created by cfan on 2023/11/10.
//
#include <stdio.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>

#define BUF_SIZE 100

int main(){
    fd_set reads, temps;
    FD_ZERO(&reads);
    FD_SET(0, &reads);//0为标准输入文件描述符, 监视标准输入

    struct timeval time;
    //time.tv_sec = 3;//在调用select函数后, tv_sec以及tv_usec的值会被替换为超前剩余时间
    //time.tv_usec = 0;

    int str_len;
    int res;
    char buf[BUF_SIZE];

    while (1){
        temps = reads;//在调用select函数后, 除发生变化的文件描述符对应位外, 其他位将初始化为0, 这是为了记住初始值
        time.tv_sec = 3;//在调用select函数后, tv_sec以及tv_usec的值会被替换为超前剩余时间
        time.tv_usec = 0;
        res = select(1, &temps, NULL, NULL, &time);
        if(res == -1){
            printf("select error\n");
            break;
        } else if(res == 0){
            printf("select timeout\n");
        } else{
            if(FD_ISSET(0, &temps)){
                str_len = read(0, buf, BUF_SIZE);
                buf[BUF_SIZE] = 0;
                printf("message from console:%s", buf);
            }

        }
    }
    return 0;
}