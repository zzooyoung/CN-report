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
#define CLIENTPORT 9010
#define BUFSIZE 512


int var = 0;

void *sendFunc(void *arg) {
    int sock = *(int *)arg;
    char buf[BUFSIZE + 1];
    int len;
    int retval;

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(SERVERPORT);

    while(1){
        if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
            break;

        // '\n'문자 제거
        len = (int)strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        if (strlen(buf) == 0) 
            break;

        retval = sendto(sock, buf, (int)strlen(buf), 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
        if (retval == -1) {
            perror("sendto");
            break;
        }
        printf("client: %s\n", buf);
    }
    
    pthread_exit(NULL);
}

void *recvFunc(void *arg) {
    int sock = *(int *)arg;
    int retval;
    char buf[BUFSIZE + 1];

    struct sockaddr_in clientaddr;
	memset(&clientaddr, 0, sizeof(clientaddr));
	clientaddr.sin_family = AF_INET;
	clientaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	clientaddr.sin_port = htons(CLIENTPORT);
	retval = bind(sock, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
	if (retval == -1){
        perror("bind");
        exit(1);
    }
    struct sockaddr_in serveraddr;
    socklen_t addrlen;
    
    while(1){
        addrlen = sizeof(serveraddr);
        retval = recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &addrlen);
        if (retval == -1){
            perror("recvfrom");
            break;
        }
        buf[retval] = '\0';
        printf("server: %s\n", buf);
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t tid1, tid2;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(1);
    }

    int *sock_ptr = &sock;
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

    close(sock);

    return 0;
}