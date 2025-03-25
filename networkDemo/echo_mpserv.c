//
// Created by cfan on 2023/11/7.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <zconf.h>
#include <signal.h>
#include <wait.h>
#include "echo_mpserv.h"

#define READ_MAX_LEN 30

void read_childproc(int signal){
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    printf("remove proc id: %d\n", pid);
}

int main() {
    //处理进入僵尸状态的子进程
    struct sigaction recv_sigaction;
    recv_sigaction.sa_handler = read_childproc;//获取子进程退出状态, 获取之后子进程将彻底销毁
    recv_sigaction.sa_flags = 0;
    sigemptyset(&recv_sigaction.sa_mask);
    sigaction(SIGCHLD, &recv_sigaction, 0);

    pid_t pid;
    ssize_t read_str_len;

    char message[READ_MAX_LEN];

    int serv_sock, client_sock;
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr, client_addr;
    socklen_t client_addr_len;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(10001);
    int res = bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if(res == -1){
        printf("bind error\n");
        return 1;
    }

    res = listen(serv_sock, 5);//最多连接数
    if(res == -1){
        printf("listen error\n");
        return 1;
    }

    while(1){
        client_sock = accept(serv_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if(client_sock == -1){
            printf("accept error\n");
            continue;
        }
        printf("new connection\n");

        pid = fork();
        if(pid == -1){
            printf("fork error\n");
            close(client_sock);
            continue;
        }

        if(pid == 0){//子进程进行连接处理
            //fork会复制父进程的所有资源, 但是套接字属于操作系统持有, 所以只复制了连接套接字的文件描述符.
            //复制的文件描述符指向同一个套接字.
            close(serv_sock);//
            while ((read_str_len = read(client_sock, message, READ_MAX_LEN)) != 0){
                write(client_sock, message, read_str_len);
                printf("Message from client(%d): %s", getpid(), message);
            }
            close(client_sock);//只有该套接字连接的所有文件描述符被关闭, 该套接字才会被关闭
            printf("client disconnected\n");
            return 0;//子进程退出后会等待父进程获取并处理退出状态, 进入僵尸状态
        } else
            close(client_sock);//只有该套接字连接的所有文件描述符被关闭，该套接字才会被关闭
    }
    close(serv_sock);
    return 0;
}

