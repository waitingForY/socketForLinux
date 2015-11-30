#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#define ERR_EXIT(m) \
do{ \
	perror(m); \
	exit(EXIT_FAILURE); \
}while(0)
//这里我们定义自己的包结构
struct packet
{
	int len;//包头，里面存放的是实际的数据长度；
	char buf[1024];
};
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
	struct packet sendbuf;
	struct packet recvbuf;
	memset(&sendbuf,0,sizeof(sendbuf));
	memset(&recvbuf,0,sizeof(recvbuf));
	int n;

	while(fgets(sendbuf.buf,sizeof(sendbuf.buf),stdin)!=NULL)
	{
		n=strlen(sendbuf.buf);
		sendbuf.len=htonl(n);
		writen(clientsocket,&sendbuf,4+n);//发送的是头部4个字节int len，再加上包体的长度n
		int recvcount=readn(clientsocket,&recvbuf.len,4);//要先接收4个字节的头部长度
		if(recvcount==-1)
		{
			ERR_EXIT("readn");
		}
		else if(recvcount<4)//如果接收到的字节数不足4个字节，说明对方关闭了
		{
			printf("peer close\n");
			break;
		}
		//接下来就开始接收包体部分了；
		n=ntohl(recvbuf.len);
		recvcount=readn(clientsocket,&recvbuf.buf,n);
		if(recvcount==-1)
		{
			ERR_EXIT("readn");
		}
		else if(recvcount<n)
		{
			printf("peer close\n");
			break;
		}	
		fputs(recvbuf.buf,stdout);
		memset(&sendbuf,0,sizeof(sendbuf));
		memset(&recvbuf,0,sizeof(recvbuf));
	}
	close(clientsocket);
	return 0;
}
