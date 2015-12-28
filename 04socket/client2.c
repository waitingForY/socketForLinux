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
		if(ret<0)
		  return ret;
		else if (ret==0)
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
void echo_client(int clientsocket)
{
	char sendbuf[1024];
	char recvbuf[1024];
	memset(&sendbuf,0,sizeof(sendbuf));
	memset(&recvbuf,0,sizeof(recvbuf));
	while(fgets(sendbuf,sizeof(sendbuf),stdin)!=NULL)
	{
		writen(clientsocket,sendbuf,strlen(sendbuf));
		int recvcount=readline(clientsocket,recvbuf,sizeof(recvbuf));
		if(recvcount==-1)
		{
			ERR_EXIT("readn");
		}
		else if(recvcount==0)
		{
			printf("peer close\n");
			break;
		}
		fputs(recvbuf,stdout);
		memset(sendbuf,0,sizeof(sendbuf));
		memset(recvbuf,0,sizeof(recvbuf));
	}
	close(clientsocket);
}
int main(void)
{
	//创建五个套接字
	int clientsocket[5];
	int i;
	for(i=0;i<5;++i)
	{
	    if((clientsocket[i]=socket(PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
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
	    if(connect(clientsocket[i],(struct sockaddr*)&seraddr,sizeof(seraddr))<0)
	    {
		    ERR_EXIT("connect");
	    }   
     	//当客户端连接成功之后，我们可以调用getsockname函数来看一下我们的客户端到底是绑定的那个地址；
	    struct sockaddr_in localaddr;
	    socklen_t addrlen=sizeof(localaddr);
	    getsockname(clientsocket[i],(struct sockaddr*)&localaddr,&addrlen);
	    printf("localhost ip=%s,port=%d\n",inet_ntoa(localaddr.sin_addr),ntohs(localaddr.sin_port));
	}
	echo_client(clientsocket[0]);
	return 0;
}
