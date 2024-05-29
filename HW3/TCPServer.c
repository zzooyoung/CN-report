#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE 512


int var = 0;

void *sendFunc(void *arg) {
    int client_sock = *(int *)arg;
    char buf[BUFSIZE + 1];
    int len;
    int retval;
    while(1) {
        if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
            break;
        
        // '\n' 문자 제거
        len = (int)strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        if (strlen(buf) == 0)
            break;

        retval = send(client_sock, buf, (int)strlen(buf), 0);
        if (retval == -1){
            perror("send");
            break;
        }
        printf("server: %s\n", buf);
    }

    pthread_exit(NULL);
}

void *recvFunc(void *arg) {
    int client_sock = *(int *)arg;
    int retval;

    while(1){
        char buf[BUFSIZE + 1];
        retval = recv(client_sock, buf, BUFSIZE, 0);
        if (retval == -1) {
            perror("recv");
            break;
        }
        else if (retval == 0)
            break;
        
        buf[retval] = '\0';
        printf("client: %s\n", buf);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    pthread_t tid1, tid2;

    int retval;

    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if(retval == -1) {
        perror("bind");
        exit(1);
    }

    retval = listen(listen_sock, SOMAXCONN);
    if (retval == -1) {
        perror("listen");
        exit(1);
    }

    int client_sock;
    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    char buf[BUFSIZE + 1];

    addrlen = sizeof(clientaddr);
    client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
    if (client_sock == -1) {
        perror("accept");
        //break;
    }

    // 접속한 클라이언트 정보 출력
    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
    printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));




    int *sock_ptr = &client_sock;
    
    if(pthread_create(&tid1, NULL, sendFunc, sock_ptr) != 0){
        fprintf(stderr, "thread create error\n");
        exit(1);
    }

    if(pthread_create(&tid2, NULL, recvFunc, sock_ptr) != 0) {
        fprintf(stderr, "thread create error\n");
        exit(1);
    }

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);

    return 0;
}