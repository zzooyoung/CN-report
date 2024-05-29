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

struct packet_buffer {
    char content[4];
    unsigned short checksum;  //checksum : 2 Byte
    int pkt_num_indicator;
    unsigned short checksum;
};
struct packet_buffer pkt_buffer;

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

int main(int argc, char *argv[]) {
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
    

    int ack_num = 0;
    struct packet_buffer *recv_pkt_buf = (struct packet_buffer *)malloc(50);

    FILE *fp;
    fp = fopen("output.txt", "wb");
    if (fp == NULL) {
        perror("fopen()");
        exit(0);
    }


    while(1){
        
        retval = recv(client_sock, (struct packet_buffer*)&pkt_buffer, BUFSIZE, 0);
        if (retval == -1) {
            perror("recv");
            break;
        }
        else if (retval == 0)
            break;
        
        recv
        char *tmp; 
        strcpy(tmp, pkt_buffer.content);
        // checksum 일치 : 0, 불일치 -1
        if(checkChecksum(tmp, (int)strlen(tmp), pkt_buffer.checksum)){
            printf("packet %d is received and there is no error. (%s) ", pkt_buffer.number, pkt_buffer.content);
            if(ack_num == pkt_buffer.Seq){
                ack_num = ack_num + (int)strlen(pkt_buffer.content);
            }
        }
        else {
            printf("An error occurred in packet %d.", pkt_buffer.number);
        }

        
        usleep(100000);  // 수신 주기 0.1 초
        char tmp_pkt_num[20];
        char buf[BUFSIZE];
        sprintf(tmp_pkt_num, "%d", ack_num);
        strcpy(buf, tmp_pkt_num);
        retval = send(client_sock, buf, (int)strlen(buf), 0);
        if (retval == -1){
            perror("send");
            break;
        }
        printf("(ACK = %s) is transmitted.\n", buf);
        
    }


    
    return 0;
}