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

void doServer(int connsocket, struct sockaddr_in cliaddr)
{

	char recvbuf[1024];
	while(1)
	{
		memset(recvbuf,0,sizeof(recvbuf));
		int recvcount=read(connsocket,recvbuf,sizeof(recvbuf));
		if(recvcount==0)
		{
			printf("the client whose ip=%s,and port=%d,closed! \n",inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
			break;
		}
		fputs(recvbuf,stdout);
		write(connsocket,recvbuf,recvcount);
	}
}
int main(void)
{
	//创建一个
	int listensocket;
	if((listensocket=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
	{
		ERR_EXIT("socket");
	}
	struct sockaddr_in seraddr;
	//memset()这个函数是初始化这个内存
	memset(&seraddr,0,sizeof(seraddr));
	seraddr.sin_family=AF_INET;
	seraddr.sin_port=htons(3000);
	//ip地址的初始化可以有以下三种方式，但是推荐第一种
	seraddr.sin_addr.s_addr=htonl(INADDR_ANY);
	/*seraddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	inet_aton("127.0.0.1",&seraddr.sin_addr);*/
	//我们最好在绑定之前设置一下套接字选项SO_REUSEADDR
	int on=1;
	if(setsockopt(listensocket,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on))<0)
	{
		ERR_EXIT("setsockopt");
	}
	//这个socket的地址结构已经初始化好了之后那么接下来就是绑定了,就是将套接字绑定的本地的套接字的地址上；
	if(bind(listensocket,(struct sockaddr*)&seraddr,sizeof(seraddr))<0)
	{
		ERR_EXIT("bind");
	}
	//bind 完了之后就是要监听这个套接字，监听之前的套接字是主动的套接字，监听后的套接字就是被动的套接字了；
	if(listen(listensocket,SOMAXCONN)<0)
	{
		ERR_EXIT("listen");
	}
	//listen之后就是将主动套接字转换为被动套接字，接下来就是接受客户端的连接请求，accept函数的调用；
	struct sockaddr_in cliaddr;
	socklen_t cliaddrlen=sizeof(cliaddr);
	int conn;
	if((conn=accept(listensocket,(struct sockaddr*)&cliaddr,&cliaddrlen))<0)
	{
		ERR_EXIT("accept");
	}
	printf("receive a connection for ip=%s,port=%d\n",inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
	//char recvbuf[1024]={0};
	//创建一个进程，一个用来接收数据，一个用来发送数据；
	pid_t pid;
	pid=fork();
	if(pid==-1)
	{
		ERR_EXIT("fork");
	}
	else if(pid==0)//子进程用来发送数据
	{
		char sendbuf[1024]={0};
		while(fgets(sendbuf,sizeof(sendbuf),stdin)!=NULL)
		{
			write(conn,sendbuf,strlen(sendbuf));
			memset(sendbuf,0,sizeof(sendbuf));
		}
		exit(EXIT_SUCCESS);
	}
	else//父进程用来接收数据
	{
		char recvbuf[1024]={0};
		while(1)
		{
			memset(recvbuf,0,sizeof(recvbuf));
			int readcount=read(conn,recvbuf,sizeof(recvbuf));
			if(readcount==-1)
			{
				ERR_EXIT("read");
			}
			else if(readcount==0)
			{
				printf("peer close！\n");
				break;
			}
			fputs(recvbuf,stdout);
		}
	}
	close(conn);
	close(listensocket);
	return 0;
}
