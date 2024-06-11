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
#define PACKETBUFSIZE 70

struct packet {
    char content[10];
    unsigned short checksum;  //checksum : 2 Byte
    short pkt_num_indicator;
};
struct packet pkt;
struct packet pkt_proc;
struct packet pkt_buffer[5];
struct packet pkt_window[4];
int buf_full;

/*********************** checksum 구현부 *********************/
unsigned short addWithCarry(unsigned short a, unsigned short b) {
    unsigned int sum = a + b;
    // 오버플로 발생 시 처리 ( 윤회식 자리올림 )
    while (sum > 0xFFFF) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    sum = sum + 1;
    return (unsigned short)sum;
}

// 문자열로부터 2바이트 체크섬을 계산하는 함수
int checkChecksum(const char *str, int length, int Checksum) {
    unsigned short sum = 0;
    int i;

    // 문자열의 각 문자에 대해 루프를 돌면서 체크섬 계산
    for (i = 0; i < length - 1; i += 2) {
        unsigned short word = (unsigned char)str[i] | ((unsigned char)str[i + 1] << 8);
        sum = addWithCarry(sum, word);
    }

    // 홀수 길이의 문자열 처리
    if (length & 1) {
        unsigned short lastByte = (unsigned char)str[length - 1];
        sum = addWithCarry(sum, lastByte);
    }
    if (~sum == Checksum){
        return 0;  // checksum 일치
    }
    else {
        return -1;  // checksum 불일치 
    }
}


/************************* Thread 함수 선언부 ****************************/
void *recvBufFunc(void *arg) {
    int sock = *(int *)arg;
    int len;
    int retval;
    
    int buf_count = 0;
    while(1) {
        retval = send(sock, (struct packet_buffer*)&pkt, (int)sizeof(pkt), 0);                 
        if (retval == -1){
            perror("send");
            break;
        }
        if(buf_full<5) {
            pkt_buffer[buf_full] = pkt;
            buf_full++;
        }
        else {
            printf("packet %d is dropped. Buffer is Full.", pkt.pkt_num_indicator);
        }
        
    }
    pthread_exit(NULL);
}

void *recvProcFunc(void *arg) {
    int sock = *(int *)arg;
    int retval;
    int pkt_count = 0;
    FILE *fp;
    fp = fopen("output.txt", "wb");
    if (fp == NULL) {
        perror("fopen()");
        exit(0);
    }

    while(1){
        if(buf_full<=0) {
            continue;
        }
        else {
            pkt_proc = pkt_buffer[buf_full-1];
            for(int i=0; i<buf_full; i++) {
                pkt_buffer[i] = pkt_buffer[i+1];
            }
            
            char *tmp; 
            strcpy(tmp, pkt_proc.content);
            // checksum 일치 : 0, 불일치 -1
            if(checkChecksum(tmp, (int)strlen(tmp), pkt_proc.checksum)){
                printf("packet %d is received and there is no error. (%s) ", pkt_proc.pkt_num_indicator, pkt_proc.content);
            }
            else {
                printf("An error occurred in packet %d.", pkt_proc.pkt_num_indicator);
                usleep(1000000);
                continue;
            }
            char buf[BUFSIZE];
            char *tmp_pkt_num;
            sprintf(tmp_pkt_num, "%d", pkt_count);
            strcpy(buf, tmp_pkt_num);
            retval = send(sock, buf, (int)strlen(buf), 0);
            if (retval == -1){
                perror("send");
                break;
            }
            printf("(ACK = %s) is transmitted.\n", buf);
            
            if(pkt_count==pkt_proc.pkt_num_indicator) {
                pkt_count++;
                if (retval > 0) {
                    fwrite(pkt_proc.content, sizeof(char), (int)strlen(pkt_proc.content), fp);
                }
                else {
                    if (fp != NULL) {
                        fclose(fp);
                        fp = NULL;
                        printf("\nShut down\n");
                    }
                    break;
                }
            } else {
                
            }


            usleep(1000000); // 수신 주기 1초 
        }
    }
    
    


    // while(1){
    //     char buf[BUFSIZE + 1];
    //     retval = recv(sock, buf, BUFSIZE, 0);
    //     if (retval == -1) {
    //         perror("recv");
    //         break;
    //     }
    //     else if (retval == 0)
    //         break;
        
    //     buf[retval] = '\0';
    //     printf("client: %s\n", buf);
    // }

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
    printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n", 
        addr, ntohs(clientaddr.sin_port));
    
    int *sock_ptr = &client_sock;
    
    if(pthread_create(&tid1, NULL, recvBufFunc, sock_ptr) != 0){
        fprintf(stderr, "thread create error\n");
        exit(1);
    }

    if(pthread_create(&tid2, NULL, recvProcFunc, sock_ptr) != 0) {
        fprintf(stderr, "thread create error\n");
        exit(1);
    }

    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);


    
    return 0;
}