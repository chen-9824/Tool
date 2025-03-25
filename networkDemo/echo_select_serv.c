//
// Created by cfan on 2023/11/10.
//

#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <zconf.h>
#include <signal.h>

#define READ_MAX_LEN 30


int main() {

    fd_set reads, reads_temp;
    int fd_max;
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

    FD_ZERO(&reads);
    FD_SET(serv_sock, &reads);
    fd_max = serv_sock;
    struct timeval timeout;

    while(1) {
        reads_temp = reads;
        timeout.tv_sec = 3;//超时时间, 调用select前重新设置
        timeout.tv_usec = 0;
        res = select(fd_max + 1, &reads_temp, 0, 0, &timeout);

        if(res == -1){
            printf("select error\n");
            break;
        } else if(res == 0){
            printf("select timeout\n");
            continue;
        } else{
            for(int i = 0; i < fd_max + 1; i++){
                if(FD_ISSET(i, &reads_temp)){
                    if(i == serv_sock){//服务端套接字有输入, 说明有新链接
                        client_sock = accept(serv_sock, (struct sockaddr *) &client_addr, &client_addr_len);
                        FD_SET(client_sock, &reads);//添加新客户端监视
                        if(client_sock > fd_max)
                            fd_max = client_sock;//修改监视范围
                        printf("new connection\n");
                    } else{//客户端套接字有输入, 说明客户端有消息传入, 需要判断是EOF断开连接还是有待接收的数据
                        read_str_len = read(i, message, READ_MAX_LEN);
                        if(read_str_len == 0){//EOF断开连接
                            FD_CLR(i, &reads);
                            close(i);
                            printf("client disconnected\n");
                        } else {//有待接收的数据
                            write(i, message, read_str_len);
                            printf("Message from client(%d): %s", getpid(), message);
                        }
                    }

                }
            }
        }

    }
    close(serv_sock);
    return 0;
}
