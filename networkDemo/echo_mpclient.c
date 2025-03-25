//
// Created by cfan on 2023/11/7.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <zconf.h>
#include <string.h>

#define BUF_SIZE 30

void read_routine(int sock){
    char buf[BUF_SIZE];
    int read_len;
    while (1){
        read_len = read(sock, buf, BUF_SIZE);
        if(read_len == 0)
            return;
        buf[read_len] = 0;
        printf("Message from server: %s\n", buf);
    }
}

void write_routine(int sock){
    char buf[BUF_SIZE];
    while (1){
        fgets(buf, BUF_SIZE, stdin);
        if(!strcmp(buf, "q\n") || !strcmp(buf, "Q\n")){
            shutdown(sock, SHUT_WR);
            return;
        }
        write(sock, buf, strlen(buf));
    }
}


int main(){
    pid_t pid;

    int sock;
    sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("192.168.147.128");
    serv_addr.sin_port = htons(10001);
    int res = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if(res == -1){
        printf("connect error\n");
        return 1;
    }

    pid = fork();
    if(pid == -1){
        close(sock);
        printf("fork error\n");
        return 1;
    }

    if(pid == 0){
        write_routine(sock);
    } else{
        read_routine(sock);
    }

    close(sock);
    return 0;
}