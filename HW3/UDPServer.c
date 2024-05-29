#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

char *CLIENTIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define CLIENTPORT 9010
#define BUFSIZE 512


int var = 0;

void *sendFunc(void *arg) {
    int client_sock = *(int *)arg;
    char buf[BUFSIZE + 1];
    int len;
    int retval;

    struct sockaddr_in clientaddr;
    memset(&clientaddr, 0, sizeof(clientaddr));
    clientaddr.sin_family = AF_INET;
    inet_pton(AF_INET, CLIENTIP, &clientaddr.sin_addr);
    clientaddr.sin_port = htons(CLIENTPORT);
    
    while(1) {
    
        if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
            break;
        
        len = (int)strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';
        if (strlen(buf) == 0)
            break;
        
        retval = sendto(client_sock, buf, (int)strlen(buf), 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
        if (retval == -1) {
            perror("sendto");
            break;
        }
        printf("server: %s\n", buf);
    }

    pthread_exit(NULL);
}

void *recvFunc(void *arg) {
    int sock = *(int *)arg;
    int retval;
    
    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    char buf[BUFSIZE + 1];

    while(1){
        addrlen = sizeof(clientaddr);
        retval = recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, &addrlen);
        if (retval == -1) {
            perror("recvfrom");
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


    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(1);
    }

    //bind()
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if(retval == -1) {
        perror("bind");
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

    return 0;
}