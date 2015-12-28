#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
//#include <signal.h>
#include <sys/wait.h>
#define ERR_EXIT(m) \
do{ \
	perror(m); \
	exit(EXIT_FAILURE); \
}while(0)
//首先我们要封装两个函数，readn（）和writen（）函数，我们将原型封装成和read和write函数一样
ssize_t readn(int fd, void *buf, size_t count)
{
	size_t nleft=count;//剩余的字节数；
	ssize_t nread;//表示接收到的字节数；
	char *bufp=(char*)buf;//定义一个指针指向buf
	//既然我们不能保证接收的字节数，那么我们就要循环接收
	while(nleft>0)
	{
		if((nread=read(fd,bufp,nleft))<0)//如果读取到的字节数小于0，有两种情况
		{
			if(errno==EINTR)//如果是被全局中断的，这时我们不认为是出错的
			  continue;
			return -1;//否则就是失败，返回-1
		}
		else if(nread==0)//如果读到的字节数是0，说明对等方关闭了，那么我们就要退出去
		  break;
		//接下来就是，如果我们读到的字节数大于0，那么我们就要进行如下偏移处理
		bufp+=nread;
		nleft-=nread;
	}
	return count-nleft;
}
//接下来我们在实现一些writen方法
size_t writen(int fd, const void *buf, size_t count)
{
	size_t nleft=count;//剩余要发送的字节数；
	ssize_t nwriten;//表示已发送的字节数；
	char *bufp=(char*)buf;//定义一个指针指向buf
	//既然我们不能保证发送的字节数，那么我们就要循环接收
	while(nleft>0)
	{
		if((nwriten=write(fd,bufp,nleft))<0)//如果发送的字节数小于0，有两种情况
		{
			if(errno==EINTR)//如果是被全局中断的，这时我们不认为是出错的，让他继续发送
			  continue;
			return -1;//否则就是失败，返回-1
		}
		else if(nwriten==0)//如果发送的字节数是0，说明发送方出现了问题；
		  break;
		//接下来就是，如果我们读到的字节数大于0，那么我们就要进行如下偏移处理
		bufp+=nwriten;
		nleft-=nwriten;
	}
	return count-nleft;
}
//这里我们封装一个recv_peek函数为了测试recv函数的MSG_PEEK选项（该选项表示只读取缓冲区中的数据，而不清楚缓冲区中的数据）
ssize_t recv_peek(int sockfd, void *buf, size_t len)
{
	//只要有数据就返回；用循环去接收
	while(1)
	{
		int ret=recv(sockfd,buf,len,MSG_PEEK);//指定选项为MSG_PEEK
		if(ret==-1 && errno == EINTR)
		  continue;
		return ret;
	}
}
//我们利用recv_peek函数来实现readline函数；（按行读取）
//readline函数也可以有效解决粘包问题；
//这个readline函数也只能用于套接字，因为我们用到了recv函数
ssize_t readline(int sockfd, void *buf, size_t maxline)
{
	int ret;
	int nread;
	char *bufp=buf;
	int nleft=maxline;
	while(1)
	{
		ret=recv_peek(sockfd,bufp,nleft);
		if(ret<=0)
		  return ret;
		nread=ret;
		int i;
		for(i=0;i<nread;i++)
		{
			if(bufp[i]=='\n')
			{
				ret=readn(sockfd,bufp,i+1);
				if(ret!=i+1)
				  exit(EXIT_FAILURE);
				return ret;
			}
		}
		if(nread > nleft)
		  exit(EXIT_FAILURE);
		nleft-=nread;
		ret=readn(sockfd,bufp,nread);
		if(ret!=nread)
		  exit(EXIT_FAILURE);
		bufp+=nread;
	}
	return -1;
}
//处理客户与服务器的通信
void echo_server(int connsocket, struct sockaddr_in cliaddr)
{
	char recvbuf[1024];
	while(1)
	{
		memset(&recvbuf,0,sizeof(recvbuf));
		int recvcount=readline(connsocket,recvbuf,1024);//要先接收4个字节的头部长度
		if(recvcount==-1)
		{
			ERR_EXIT("readline");
		}
		else if(recvcount==0)//如果接收到的字节数不足4个字节，说明对方关闭了
		{
			printf("client close\n");
			break;
		}
		fputs(recvbuf,stdout);
		writen(connsocket,recvbuf,strlen(recvbuf));//接下来就是回射回去
	}
}
void sig_hander(int sig)
{
	wait(NULL);
}
int main(void)
{
	signal(SIGCHLD,sig_hander);
	//signal(SIGCHLD,SIG_IGN);
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
			echo_server(connsocket,cliaddr);
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
