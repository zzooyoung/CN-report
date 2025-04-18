#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE 512

int var = 0;
int wnd_use = 4;
int send_wnd[5];
int pkt_num;
int time_clock;

int pkt_num_index = 0;

struct packet_buffer {
    char content[4];
    unsigned short checksum;  //checksum : 2 Byte
    int pkt_num_indicator;
};
struct packet_buffer pkt_buffer;

struct timeout_buf {
    int time;
    FILE *ptr;
    int ack;
};
struct timeout_buf t_buf[50];

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
    int sock = *(int *)arg;
    int retval;
    int loss_switch = 1;
    pkt_num = 0;
    time_clock = 0;

    FILE *fp;
    char f_name[10] = "text.txt";
    fp = fopen(f_name, "rb");
    if(fp == NULL) {
        printf("파일 불러오기를 실패하였습니다.\n");
        exit(0);
    }
    
    while(1) {
        int read_count;
        wnd_use--;

        for(int idx=0; idx<50; idx++){
            if(t_buf[idx].ack==0) {
                if(clock() - t_buf[idx].time / CLOCKS_PER_SEC >0.5) {
                    fp = t_buf[idx].ptr;
                    pkt_num_index = idx;
                    break;
                }
            }
        }

        read_count = fread(pkt_buffer.content, 2, sizeof(pkt_buffer.content), fp);
        pkt_buffer.checksum = calculateChecksum(pkt_buffer.content, (int)strlen(pkt_buffer.content));
        pkt_buffer.pkt_num_indicator = pkt_num_index;
        pkt_num_index++;
        retval = send(sock, (struct packet_buffer*)&pkt_buffer, (int)sizeof(pkt_buffer), 0);
        if (retval == -1){
            perror("send");
            break;
        }
        printf("packet %d is transmitted. (%s)\n", pkt_buffer.pkt_num_indicator, pkt_buffer.content);
        pkt_num++;
        t_buf[pkt_buffer.pkt_num_indicator].ack = 0;
        t_buf[pkt_buffer.pkt_num_indicator].time = clock();
        t_buf[pkt_buffer.pkt_num_indicator].ptr = fp;
            
        if (feof(fp)){
            read_count = fread(pkt_buffer.content, 2, sizeof(pkt_buffer.content), fp);
            break;
        }

        while(wnd_use<=0){

        }


        usleep(50000);  // 송신 주기 0.05초 
    }
    

    pthread_exit(NULL);
}

void *recvFunc(void *arg) {
    int sock = *(int *)arg;
    int retval;
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
        wnd_use++;
        int ack_num = atoi(buf);
        printf("(ACK = %s)is received. and", buf);
        t_buf[ack_num].ack = 1;
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