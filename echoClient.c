#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define ERR_EXIT(m) \
do{ \
	perror(m); \
	exit(EXIT_FAILURE); \
}while(0)

int main(void)
{
	//创建一个
	int clientsocket;
	if((clientsocket=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
	{
		ERR_EXIT("socket");
	}
	//服务器的地址结构如下：
	struct sockaddr_in seraddr;
	//memset()这个函数是初始化这个内存
	memset(&seraddr,0,sizeof(seraddr));
	seraddr.sin_family=AF_INET;
	seraddr.sin_port=htons(3000);
	//ip地址的初始化可以有以下三种方式，但是推荐第一种
	//seraddr.sin_addr.s_addr=htonl(INADDR_ANY);
	seraddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	/*inet_aton("127.0.0.1",&seraddr.sin_addr);*/
	//下面就是用这个套接字向服务器发起连接；因为这后的套接字就是主动套接字
	if(connect(clientsocket,(struct sockaddr*)&seraddr,sizeof(seraddr))<0)
	{
		ERR_EXIT("connect");
	}
	char sendbuf[1024]={0};
	char recvbuf[1024]={0};
	while(fgets(sendbuf,sizeof(sendbuf),stdin)!=NULL)
	{
		
		write(clientsocket,sendbuf,strlen(sendbuf));
		read(clientsocket,recvbuf,sizeof(recvbuf));
		fputs(recvbuf,stdout);
		memset(sendbuf,0,sizeof(sendbuf));
		memset(recvbuf,0,sizeof(recvbuf));
	}
	close(clientsocket);
	return 0;
}






