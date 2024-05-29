#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#define SERVERPORT 9000
#define BUFSIZE    512

void errorFuncExit(int errorCode, char *msg) {
    if (errorCode == -1){
        perror(msg);
        exit(1);   
    }
}

int main(int argc, char *argv[])
{
	int retval;
	FILE *fp;

	// 소켓 생성
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	errorFuncExit(sock ,"socket"); 

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	errorFuncExit(retval, "bind()");

	// 데이터 통신에 사용할 변수
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char buf[BUFSIZE + 1];

	// 클라이언트와 데이터 통신
	while (1) {
		// 데이터 받기
		addrlen = sizeof(clientaddr);
		retval = recvfrom(sock, buf, BUFSIZE, 0,
			(struct sockaddr *)&clientaddr, &addrlen);
		if (retval == -1) {
			perror("recvfrom()");
			break;
		}

		// 받은 데이터 출력
		buf[retval] = '\0';
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("[UDP/%s:%d] %s\n", addr, ntohs(clientaddr.sin_port), buf);

		// 요청 파일명 확인 
		char cmd[10];
		char file_name[20];

		int len = (int)strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0)
			break;

		char *ptr = strdup(buf);
		memcpy(cmd, &ptr[0],7);
		memcpy(file_name, &ptr[9], 9);
		printf("\nCommand: %s filename: %s\n", cmd, file_name);

		strcpy(buf, file_name);
		int file_name_len = (int)strlen(buf);

		// 데이터 보내기
		retval = sendto(sock, buf, file_name_len , 0,
			(struct sockaddr *)&clientaddr, sizeof(clientaddr));

		if(retval == -1) {
			perror("sendto()");
			break;
		}

		int chk_file_name = strcmp(file_name, "novel.txt");
		printf("%d\n", chk_file_name);

		// 파일 보내기 
		if (chk_file_name == 0){
			fp = fopen(file_name, "rb");
		}
		else {
			printf("해당 파일이 존재하지 않습니다.");
		}
		if (fp == NULL) perror("File Open Failure");
		printf("file open Success");


		int index = 0;
		while (!feof(fp)) {
			int read_count = fread(buf, 1, BUFSIZE, fp);
			printf("\n%d\n", index);
			if (read_count < BUFSIZE) {
				sendto(sock, buf, read_count+1, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
				printf("%s\n", buf); 
				strcpy(buf, "1");
				sendto(sock, buf, 1, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
			}
			else {
				sendto(sock, buf, BUFSIZE, 0, (struct sockaddr *)&clientaddr, sizeof(clientaddr));
				printf("%s\n", buf);
			}
			index++;
		}

		

		// 데이터 보내기
		retval = sendto(sock, buf, retval, 0,
			(struct sockaddr *)&clientaddr, sizeof(clientaddr));
		if(retval == -1) {
			perror("sendto()");
			break;
		}
	}

	// 소켓 닫기
	close(sock);
	fclose(fp);
	return 0;
}