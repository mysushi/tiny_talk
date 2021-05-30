#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define MAXCLIENTS 5
int main(int argc, char **argv) {
	int sock;
	int csock[MAXCLIENTS] = {-1, -1, -1, -1, -1};
	struct sockaddr_in svr;
	struct sockaddr_in clt;
	int clen, reuse, nbytes, statenum=2, k=0, socknum;
	char rbuf[1024];
	char username[MAXCLIENTS][1024];
	fd_set rfds; //select()で用いるファイル記述子集合
	struct timeval tv; //select()が返ってくるまでの待ち時間を指定する変数

	//ソケットの生成
	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket");
		exit(1);
	}

	//ソケットアドレス再利用の指定
	reuse = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		perror("setsockopt");
		exit(1); 
	}

	//client受付用ソケットの情報設定
	bzero(&svr, sizeof(svr));
	svr.sin_family = AF_INET;
	svr.sin_addr.s_addr = htonl(INADDR_ANY);
	svr.sin_port = htons(10140);
	
	//ソケットにソケットアドレスを割り当てる
	if (bind(sock, (struct sockaddr *)&svr, sizeof(svr)) < 0) {
		perror("bind");
		exit(1);
	}

for(int i=0; i<MAXCLIENTS; i++) {
	csock[i] = -1;
}


	//待ち受けクライアント数の設定
	if (listen(sock, 5) < 0) { //待ち受け数を5に指定
		perror("listen");
		exit(1);
	} do {
		switch (statenum) {
			case 2:
				//printf("case2なう\n");
				FD_ZERO(&rfds); //rfdsを空集合に初期化
				FD_SET(sock, &rfds);
				for (int i = 0; i < k; i++) FD_SET(csock[i], &rfds);
				//監視する待ち時間を1秒に設定
				tv.tv_sec = 1;
				tv.tv_usec = 0;
				if (select(MAXCLIENTS+1, &rfds, NULL, NULL, &tv) > 0) {
					statenum = 3;
					break;
				}
				break;
			case 3:
				//printf("case3なう\n");
				if (FD_ISSET(sock, &rfds)) {
					statenum = 4;
					break;
				} else {
					for (int i = 0; i < k; i++) {
						if (FD_ISSET(csock[i], &rfds)) {
							socknum = i;
							statenum = 6;
							break;
						}
					}
				}
				//statenum = 2;
				break;
			case 4:
				//printf("case4なう\n");
				//クライアントの受付
				clen = sizeof(clt);
				if ((csock[k] = accept(sock, (struct sockaddr *)&clt, &clen)) < 0) {
					perror("accept");
					exit(2);
				}
				k++;
				if (k <= MAXCLIENTS) {
					write(csock[k-1], "REQUEST ACCEPTED\n", strlen("REQUEST ACCEPTED\n"));
					statenum = 5;
					break;
				} else {
					write(csock[k-1], "REQUEST REJECTED\n", strlen("REQUEST REJECTED\n"));
					close(csock[k-1]);
					k--;
					statenum = 2;
					break;
				}
				//statenum = 3;
				break;
			case 5:
				//printf("case5なう\n");
				//クライアントからのメッセージ受信
				if ((nbytes = read(csock[k-1], rbuf, sizeof(rbuf))) < 0) {
					perror("read");
				} else {

					char *p = rbuf;
          int buf_count = 0;
          while (*p != '\n') {
            buf_count++;
            p++;
          }
					
					for (int i = 0; i < k-1; i++) {
						if (strncmp(username[i], rbuf, buf_count) == 0) {
							write(csock[k], "USERNAME REJECTED\n", strlen("USERNAME REJECTED\n"));
//printf("reject\n");
							write(1, rbuf, nbytes);
							close(csock[k-1]);
							csock[k-1] = -1;
							statenum = 2;
							break;
						}
					}
					write(csock[k-1], "USERNAME REGISTERED\n", strlen("USERNAME REGISTERED\n"));
//printf("registered\n");
					write(1, rbuf,  nbytes);
					strncpy(username[k-1], rbuf, buf_count);
					//k++;
					statenum = 2;
					break;
				}
				break;
			case 6:
				//printf("case6なう\n");
				//クライアントからのメッセージ受信
				if ((nbytes = read(csock[socknum], rbuf, sizeof(rbuf))) < 0) {
					perror("read");
				} else {
					if (nbytes == 0) statenum = 7;
					else {
						time_t t = time(NULL);
						struct tm *local = localtime(&t);
	
						printf("%04d/", local->tm_year + 1900);
						printf("%02d/", local->tm_mon + 1);
						printf("%02d", local->tm_mday);

						printf(" ");

						printf("%02d:", local->tm_hour);
						printf("%02d:", local->tm_min);
						printf("%02d", local->tm_sec);

						printf("に発言がありました\n");

						for (int i = 0; i < k; i++) {
							write(csock[i], username[socknum], sizeof(username[socknum]));
							write(csock[i], "> ", strlen("> "));
							write(csock[i], rbuf, nbytes);
						}
						statenum = 2;
					}
					break;
				}
				break;
			case 7:
				//printf("case7なう\n");
				close(csock[socknum]);
				printf("離脱したユーザ: %s\n", username[socknum]);
				strcpy(username[socknum], username[k-1]);
				csock[socknum] = csock[k-1];
				csock[k] = -1;
				k--;
				statenum = 2;
				break;
		}
	} while(1);



}
