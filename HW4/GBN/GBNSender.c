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
int wnd_size = 4;
int wnd_use = 4;
int send_wnd[5];
int pkt_num;
struct packet
{
    char name[20];
    int timer;
    int ack_flag;
};
struct packet pkt[8];



// void *sendFunc(void *arg) {
//     char buf[BUFSIZE + 1];
//     int sock = *(int *)arg;
//     int len;
//     int retval;
//     int loss_switch = 1;
    
//     while(1) {
//         if (wnd_use > 0) {
            
//             if (pkt_num == 2 && loss_switch == 1) {
//                 printf("\"%s\"is transmitted.\n", pkt[pkt_num].name);
//                 loss_switch = 0;
//             }
//             else {
//                 strcpy(buf, pkt[pkt_num].name);
//                 retval = send(sock, buf, (int)strlen(buf), 0);
//                 if (retval == -1){
//                     perror("send");
//                     break;
//                 }
//                 printf("\"%s\"is transmitted.\n", buf);
//             }
            
//             wnd_use--;
            
//             for(int j=0;j<pkt_num;j++){
//                 if(pkt[j].timer<5){
//                     pkt[j].timer++;
//                     printf("%d's timer : %d\n", j, pkt[j].timer);
//                 }
//                 if(pkt[j].timer>4 && pkt[j].ack_flag==0){
//                         printf("%s is timeout.\n", pkt[j].name);
//                         wnd_use = pkt_num - j;
//                         pkt_num = j - 1;

//                     }
//             }
            
//             pkt_num++;
//             sleep(0.0001);
//         }
//     }
    

//     pthread_exit(NULL);
// }

void *sendFunc(void *arg) {
    char buf[BUFSIZE + 1];
    int sock = *(int *)arg;
    int retval;
    int loss_switch = 1;
    int finish_switch = 0; 
    
    while(1) {
        if (wnd_use > 0) {
            
            if (pkt_num == 2 && loss_switch == 1) {
                printf("\"%s\" is transmitted.\n", pkt[pkt_num].name);
                loss_switch = 0; // 여기를 고침
            }
            else {
                strcpy(buf, pkt[pkt_num].name);
                retval = send(sock, buf, (int)strlen(buf), 0);
                if (retval == -1){
                    perror("send");
                    break;
                }
                printf("\"%s\" is transmitted.\n", buf);
            }
            
            wnd_use--;
            
            for(int j=0; j<=pkt_num; j++){ // 여기를 고침: 모든 패킷의 타이머를 검사
                if(pkt[j].timer<5 && pkt[j].ack_flag == 0){
                    pkt[j].timer++;
                    // printf("%d's timer : %d\n", j, pkt[j].timer);
                }
                if(pkt[j].timer>4 && pkt[j].ack_flag==0){
                        pkt[j].timer = 0;
                        printf("%s is timeout.\n", pkt[j].name);
                        wnd_use = wnd_size - (pkt_num - j); // wnd_use와 pkt_num을 적절히 조정
                        pkt_num = j-1; // 재전송을 시작할 위치를 조정
                        break; // 타임아웃을 처리한 후 루프를 빠져나옴
                    }
            }
            if (pkt_num == 5 && finish_switch == 0){
                finish_switch = 1;
            }
            else if (pkt_num == 5 && finish_switch != 0){
                exit(1);
                break;
            }
            pkt_num++;
            
            
            sleep(0.0001); // sleep(0.0001)은 효과적이지 않으므로, usleep(100)으로 변경
        }
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
        
        int ack_num = atoi(strpbrk(buf, " "));

        if(pkt[ack_num].ack_flag == 1){
            printf("\"%s\"is received and ignored.\n", buf);
        }
        else{
            pkt[ack_num].ack_flag = 1;
            printf("\"%s\"is received.", buf);
            wnd_use++;
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
    
    for(int i=0;i<8;i++){
        char tmp[20] = "packet ";
        char tmp_i[5];
        sprintf(tmp_i, "%d", i);
        strcat(tmp, tmp_i);
        strcpy(pkt[i].name, tmp);
        pkt[i].timer = 1;
        pkt[i].ack_flag =0;
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