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


char message[5][45] = {{"I am a boy."}, {"You are a girl."}, {"There are many animals in the zoo."},
    {"철수와 영희는 서로 좋아합니다!"}, {"나는 점심을 맛있게 먹었습니다."}};  

int var = 0;
int wnd_size = 4;
int wnd_use = 4;
int send_wnd[5];
int pkt_num;
int time_clock;
struct packet
{
    int send_time;
    int ack_flag;
    int Seq;
    char content[45];
};
struct packet pkt[5];

struct packet_buffer {
    int number;
    int Seq;
    char content[45];
    unsigned short checksum;
};
struct packet_buffer pkt_buffer;

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
    
    while(1) {
        if(pkt_num == 5){
            pthread_exit(NULL);
        }
        
        if (pkt_num == 1 && loss_switch == 1) {
                printf("packet %d is transmitted.(%s)\n", pkt_num, pkt[pkt_num].content);
                loss_switch = 0; 
            }
        else {
            strcpy(pkt_buffer.content, pkt[pkt_num].content);
            pkt_buffer.number = pkt_num;
            pkt_buffer.Seq = pkt[pkt_num].Seq;
            char *temp;
            strcpy(temp, pkt[pkt_num].content);
            pkt_buffer.checksum = (unsigned short)0;
            retval = send(sock, (struct packet_buffer*)&pkt_buffer, (int)sizeof(pkt_buffer), 0);
            if (retval == -1){
                perror("send");
                break;
            }
            printf("packet %d is transmitted. (%s)\n", pkt_num, pkt[pkt_num].content);
                
        }
            
        pkt_num++;
        time_clock++;
        for(int i = 0; i<5; i++){  // TimeoutInterver = 15
            if (time_clock - pkt[i].send_time >= 15) {
                pkt_num = i;
            }
        }
            
        usleep(100000); 
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
        
        int ack_num = atoi(buf);
        printf("(ACK = %s)is received.\n", buf);
        for(int i=0; i<5; i++){
            if (pkt[i].Seq = ack_num){
                pkt[i].ack_flag++;
                if(pkt[i].ack_flag>3){
                    strcpy(pkt_buffer.content, pkt[i].content);
                    pkt_buffer.Seq = (int)strlen(pkt[i].content);
                    char *temp;
                    strcpy(temp, pkt[pkt_num].content);
                    pkt_buffer.checksum = (unsigned short)calculateChecksum(temp, (int)strlen(temp));
                    retval = send(sock, (struct packet_buffer*)&pkt_buffer, (int)sizeof(pkt_buffer), 0);
                    if (retval == -1) {
                        perror("send");
                        break;
                    }
                    printf("packet %d is retransmitted. (%s)\n", i, pkt_buffer.content);
                    exit(1);
                    break;

                }
            }
        }
        usleep(100000);
        
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
    
    int seq_acc = 0;
    for(int i=0; i<5; i++){
        pkt[i].ack_flag = 0;
        strcpy(pkt[i].content, message[i]);
        pkt[i].Seq = seq_acc;
        seq_acc = seq_acc + (int)strlen(pkt[i].content);
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