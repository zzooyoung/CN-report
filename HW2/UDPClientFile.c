#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512

void errorFuncExit(int errorCode,char *msg) {
    if (errorCode == -1){
        perror(msg);
        exit(1);   
    }
}

int main(int argc, char *argv[])
{
	int retval;

	// 명령행 인수가 있으면 IP 주소로 사용
	if (argc > 1) SERVERIP = argv[1];

	// 소켓 생성
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
    errorFuncExit(sock, "socket()");

	// 소켓 주소 구조체 초기화
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);

	// 데이터 통신에 사용할 변수
	struct sockaddr_in peeraddr;
	socklen_t addrlen;
	char buf[BUFSIZE + 1];
	int len;

	// 서버와 데이터 통신
	while (1) {
		// 데이터 입력
		printf("\n[보낼 데이터] ");
		if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
			break;

		// '\n' 문자 제거
		len = (int)strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0)
			break;

		// 데이터 보내기
		retval = sendto(sock, buf, (int)strlen(buf), 0,
			(struct sockaddr *)&serveraddr, sizeof(serveraddr));
		if (retval == -1) {
			perror("sendto()");
			break;
		}

		printf("[UDP 클라이언트] %d바이트를 보냈습니다.\n", retval);

	
		// 데이터 (제목) 받기
		addrlen = sizeof(peeraddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(struct sockaddr *)&peeraddr, &addrlen);
		if (retval == -1) {
			perror("recvfrom()");
			break;
		}
		// 받은 데이터 출력
		buf[retval] = '\0';
		printf("[UDP 클라이언트] %d바이트를 받았습니다.\n", retval);
		printf("[받은 데이터] %s\n", buf);
		
		// 파일 받기 준비
        FILE *fp;
        fp = fopen(buf, "wb");
        if (fp == NULL) {
            perror("fopen()");
            break;
        }
		

        int read_count;

        // 데이터를 받아서 파일에 기록
        do {
            retval = recvfrom(sock, buf, BUFSIZE, 0, (struct sockaddr *)&peeraddr, &addrlen);
            
            if (retval == -1) {
                perror("recvfrom()");
                break;
            }
			int chk = strcmp(buf, "1");
            if (chk == 0) {
                printf("파일전송을 완료하였습니다.\n");
                break;
            } else {
				printf("%s\n", buf);
			}

            // 송신자의 주소 체크 (데이터 수신 후 체크로 이동)
            if (memcmp(&peeraddr, &serveraddr, sizeof(peeraddr))) {
                printf("[오류] 잘못된 데이터입니다!\n");
                break;
            }

            // 파일에 데이터 기록
            if (retval > 0) {
                fwrite(buf, sizeof(char), retval, fp);
            }
            else {
                if (fp != NULL) {
                    fclose(fp);
                    fp = NULL;
                    printf("\nShut down\n");
                }
                break;
            }
            
        } while (retval > 0);
        if (fp != NULL) {
            fclose(fp);
            fp = NULL;
            printf("\nShut down\n");
        }
        break;

	}
	// 소켓 닫기
	close(sock);
    exit(0);
	return 0;
}