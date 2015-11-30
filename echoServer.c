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
	int connsocket;
	pid_t pid;
	while(1)
	{
		if((connsocket=accept(listensocket,(struct sockaddr*)&cliaddr,&cliaddrlen))<0)
		{
			ERR_EXIT("accept");
		}
		printf("接收到一个连接：客户端的ip=%s，port=%d\n",inet_ntoa(cliaddr.sin_addr),ntohs(cliaddr.sin_port));
		pid=fork();
		if(pid==-1)
		{
			ERR_EXIT("fork");
		}
		if(pid==0)//也就是如果是子进程，那么子进程就不需要监听套接字，首先要关闭监听套接字，然后就是处理通信的过程
		{
			close(listensocket);
			doServer(connsocket,cliaddr);
			exit(EXIT_SUCCESS);
		}
		else
		{
			close(connsocket);
		}
	}
	//close(connsocket);
	//close(listensocket);
	return 0;
}






