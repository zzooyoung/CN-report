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
    

    int pkt_num;
    int pkt_seq = 0;
    int drop_chk[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    int record_buf[3] = {-1, -1, -1};
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
        char *tmp_buf = strpbrk(buf, " ");
        pkt_num = atoi(tmp_buf + 1);
        if (pkt_seq == pkt_num){
            if (record_buf[0] != -1) {
                printf("\"%s\" is received.", buf);
                printf("%s", buf);
                for(int i = 0; i<3;i++){
                    if (record_buf[i] != -1){
                        printf(", packet %d", record_buf[i]);
                    }
                }
                printf(" are delivered.");
            }
            else {
                printf("\"%s\" is received.", buf);
            }

            char tmp[1024] ="ACK ";
            char tmp_pkt_num[20];
            sprintf(tmp_pkt_num, "%d", pkt_num);
            strcat(tmp, tmp_pkt_num);
            strcpy(buf, tmp);
            retval = send(client_sock, buf, (int)strlen(buf), 0);
            if (retval == -1){
                perror("send");
                break;
            }
            
            printf("\" %s\" is transmitted.\n", buf);
            if (pkt_num == 2){
                exit(1);
                break;
            }
            pkt_seq++;

        }
        else {
            printf("\"%s\" is received and buffered.", buf);

            int ack_num = atoi(strpbrk(buf, " "));

            int count = 0;
            while(count<3){
                if (record_buf[count] == -1) {
                    record_buf[count] = ack_num;
                    break;
                }
                count++;
            }
            drop_chk[pkt_num] = 1;
            char tmp[1024] = "ACK"; // 충분한 크기의 배열을 선언하고 초기화
            strcat(tmp, strpbrk(buf, " ")); // tmp에 tmp_pkt_seq를 연결
            strcpy(buf, tmp); // buf에 tmp의 내용을 복사
            retval = send(client_sock, buf, (int)strlen(buf), 0);
            if (retval == -1){
                perror("send");
                break;
            }
            printf("\"%s\" is retransmitted.\n", buf);
        }
        sleep(0.0002);
        
    }


    
    return 0;
}