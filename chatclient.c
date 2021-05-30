#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

int main(int argc, char **argv) {
	int sock, nbytes;
  struct sockaddr_in svr;
	struct hostent *cp;
	int clen, reuse, statenum=2;
	char rbuf[1024], rbuf2[1024];
	char username[98];
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

	//server受付用ソケットの情報設定
	bzero(&svr, sizeof(svr));
	cp = gethostbyname(argv[1]);
	svr.sin_family = AF_INET;
	bcopy(cp->h_addr, &svr.sin_addr, cp->h_length);
	svr.sin_port = htons(10140);



	connect(sock, (struct sockaddr *) &svr, sizeof(svr));

	do {
		switch (statenum) {
			case 2:
				read(sock, rbuf, 17);
				if (strncmp(rbuf, "REQUEST ACCEPTED\n", 17) == 0) statenum = 3;
				else statenum = 6;
				break;
			case 3:
				strcpy(username, strcat(argv[2],"\n"));
				write(sock, username, sizeof(username));
				read(sock, rbuf, 20);
				if(strncmp(rbuf, "USERNAME REGISTERED\n", 20) == 0) statenum = 4;
				else statenum = 6;
				break;
			case 4:
				FD_ZERO(&rfds); //rfdsを空集合に初期化
				FD_SET(0, &rfds); //標準入力
				FD_SET(sock, &rfds); //クライアントを受け付けたソケット

				//監視する待ち時間を1秒に設定
				tv.tv_sec = 1;
				tv.tv_usec = 0;

				//標準入力とソケットからの受信を同時に監視する
				if (select(sock+1, &rfds, NULL, NULL, &tv) > 0) {
					if (FD_ISSET(0, &rfds)) { //標準入力から入力があったなら
						//標準入力から読み込みクライアントに送信
						memset(rbuf, '\0', sizeof(rbuf));
						if ((nbytes = read(0, rbuf, sizeof(rbuf))) == 0) {
							statenum = 5;
							break;
						} else {
							write(sock, rbuf, sizeof(rbuf));
						}
					}
					if (FD_ISSET(sock, &rfds)) { //ソケットから受信したなら
						//ソケットから読み込み端末に出力
						read(sock, rbuf2, sizeof(rbuf2));
						write(1, rbuf2, sizeof(rbuf2));
					}
				}
				break;
			case 5:
				close(sock);
				exit(0);
				break;
			case 6:
				close(sock);
				exit(1);
				break;
		}
	} while(1);
}
