#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define SERVERPORT 9000
#define BUFSIZE    512
#define CALENDERSIZE 200

int is_leaf_year(int year){
	if(year % 400 == 0) return 1;
	if((year%100 != 0) && (year&4 == 0)) return 1;
	return 0;
}



int get_day_of_month(int year, int month){
	int day_of_month[13] = {0, 31, 27, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	day_of_month[2] += is_leaf_year(year);
	return day_of_month[month];
}

int get_day(int year, int month){
	int past = 0;
	for(int y=1; y<year; y++) past = past + 365 + is_leaf_year(y);
	for(int m=1; m<month; m++) past = past + get_day_of_month(year, month);
	return (1+past) % 7;
}


void print_cal(int start_day, int day_num){
	printf(" Sun Mon Tue Wed Thu Fri Sat\n");
	for(int i = 0; i<start_day;i++) printf("    ");
	for(int day = 1, lineNum = start_day; day <= day_num; day++, lineNum++){
		printf("%4d", day);
		if(lineNum%7 == 6) printf("\n");
	}
	printf("\n");
}


int main(int argc, char *argv[])
{
	int retval, listen_sock;

	// 소켓 생성
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == -1) {
        perror("socket");
        exit(1);
    }

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (retval == -1) {
        perror("bind");
        exit(1);
    }
    // err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == -1) {
        perror("listen");
        exit(1);
    }
    //err_quit("listen()");

	// 데이터 통신에 사용할 변수
	int client_sock;
	struct sockaddr_in clientaddr;
	socklen_t addrlen;
	char buf[BUFSIZE + 1];

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
		if (client_sock == -1) {
			perror("accept");  //err_display("accept()");
			break;
		}

		// 접속한 클라이언트 정보 출력
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			addr, ntohs(clientaddr.sin_port));

		// 클라이언트와 데이터 통신
		while (1) {
			// 데이터 받기
			retval = recv(client_sock, buf, BUFSIZE, 0);
			if (retval == -1) {
				perror("recv");  //err_display("recv()");
				break;
			}
			else if (retval == 0)
				break;

			// 받은 데이터 출력
			buf[retval] = '\0';
			printf("[TCP/%s:%d] %s\n", addr, ntohs(clientaddr.sin_port), buf);

			// 달력 출력
			char temp_year[5];
			char temp_month[3];
			char *ptr = strdup(buf);
			memcpy(temp_year, &ptr[0],4);
			memcpy(temp_month, &ptr[5], 2);
			int year = atoi(temp_year);
			int month = atoi(temp_month);
			printf("\n년: %d 월: %d\n", year, month);

			int start_day = get_day(year, month);
			int day_num = get_day_of_month(year, month);
			print_cal(start_day, day_num);

			

			// 달력 데이터 보내기 

			char buf_cal[CALENDERSIZE] = " Sun Mon Tue Wed Thu Fri Sat\n";
			for(int i = 0; i<start_day;i++){
				strcat(buf_cal, "    ");
			}
			
			char buf_day[5];
			for(int day = 1, lineNum = start_day; day <= day_num; day++, lineNum++){
				sprintf(buf_day, "%4d", day);
				strcat(buf_cal, buf_day);
				
				if(lineNum%7 == 6) {
					strcat(buf_cal, "\n");
				}
			}
			strcat(buf_cal, "\n");
			retval = send(client_sock, buf_cal, CALENDERSIZE, 0);
			if (retval == -1) {
				perror("send");  //err_display("send()");
				break;
			}
		}
 
		// 소켓 닫기
		close(client_sock);
		printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
			addr, ntohs(clientaddr.sin_port));
	}

	// 소켓 닫기
	close(listen_sock);
	return 0;
}