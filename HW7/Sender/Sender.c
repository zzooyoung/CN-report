#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>


char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE 512


int var = 0;
int wnd_size = 4;
int wnd_use = 4;
int send_wnd[5];
int send_window = 4;
int pkt_count = 0;
int file_end_flag = 0;

struct packet {  
    char content[10];
    unsigned short checksum;  // checksum : 2 Byte
    short pkt_num_indicator;
};
struct packet pkt;
struct packet window_pkt[4];
int ack_chk[4];
struct timeval timeval[4];
// Checksum 구현 부분
// 두 바이트 값을 더하고, 오버플로 발생 시 처리하는 함수
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
unsigned short calculateChecksum(const char *str, int length) {
    unsigned short checksum = 0;
    int i;

    // 문자열의 각 문자에 대해 루프를 돌면서 체크섬 계산
    for (i = 0; i < length - 1; i += 2) {
        unsigned short word = (unsigned char)str[i] | ((unsigned char)str[i + 1] << 8);
        checksum = addWithCarry(checksum, word);
    }

    // 홀수 길이의 문자열 처리
    if (length & 1) {
        unsigned short lastByte = (unsigned char)str[length - 1];
        checksum = addWithCarry(checksum, lastByte);
    }

    checksum = ~checksum;
    return (unsigned short)checksum;
}

void *sendFunc(void *arg) {
    char buf[BUFSIZE + 1];
    int sock = *(int *)arg;
    int retval;
    int loss_switch = 1;
    int finish_switch = 0; 
    
    FILE *fp;
    fp = fopen("input1.txt", "rb");
    if(fp == NULL) {
        printf("파일 불러오기를 실패하였습니다.\n");
        exit(0);
    }
    while(1) {
        while(send_window > 0){
            if(feof(fp)) {
                printf("파일 전송이 완료되었습니다.\n");
                pthread_exit(NULL);
                exit(0);
            }
            int read_count = fread(pkt.content, sizeof(char), 10, fp);
            pkt.checksum = calculateChecksum(pkt.content, (int)strlen(pkt.content));
            pkt.pkt_num_indicator = pkt_count;
            pkt_count++;
            retval = send(sock, (struct packet_buffer*)&pkt, (int)sizeof(pkt), 0);
            if(retval == -1) {
                perror("send");
                break;
            }
            printf("packet %d is transmitted. (%s)\n", pkt.pkt_num_indicator, pkt.content);
            if(pkt_count < 4) {
                window_pkt[pkt_count] = pkt;
                
                gettimeofday(&timeval[pkt_count], NULL);

            } else {
                window_pkt[0] = window_pkt[1];
                window_pkt[1] = window_pkt[2];
                window_pkt[2] = window_pkt[3];
                window_pkt[3] = pkt;
                timeval[0] = timeval[1];
                timeval[1] = timeval[2];
                timeval[2] = timeval[3];
                gettimeofday(&timeval[3], NULL);

            }
            usleep(500000);  // 송신주기 0.5초
        }
    }
    pthread_exit(NULL);
}

void *recvFunc(void *arg) {
    int sock = *(int *)arg;
    int retval;
    int ack_seq = 0;
    while(1){
        char buf[BUFSIZE + 1];
        retval = recv(sock, buf, BUFSIZE, 0);
        if (retval == -1) {
            perror("recv");
            break;
        }
        else if (retval == 0)
            break;

        buf[retval] = '\0';
        
        int ack_num = atoi(strpbrk(buf, " "));

        if(ack_seq == ack_num){
            printf("\"ACK %s\"is received.", buf);
            send_window++;
        }
        else{
            printf("\"ACK %s\"is received and recorded.\n", buf);
        }
        
    }

    pthread_exit(NULL);
}

int main() {
    pthread_t tid1, tid2;
    int retval;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1){
        perror("socket");
        exit(1);
    }
    
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if(retval == -1) {
        perror("connect");
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