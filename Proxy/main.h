#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>



#define uint unsigned int
#define TRUE 1
#define False 0
#define BUFFSIZE 4096
#define SYSTEM_PORT 0
#define IPLEN 32
#define FTPPORT 21

/************************************************************************/
/*Struct Define                                                         */
/************************************************************************/

enum MODE{
	PASV,
	PORT

};



typedef struct{

	char clientIP[IPLEN];   //客户端的ip
	char serverIP[IPLEN];	//ftp服务器的ip
	char proxyIP[IPLEN];   	//proxy 服务器的ip


}IPADDR_t;


typedef struct{

	uint ProxyServerDataPort;//主动、被动模式下proxy的数据链路port
	uint ClientDataPort;//主动模式下，client的数据链路port
	uint SeverDataPort;//被动模式下，server端的数据链路port

}DATAPORT_t;

typedef struct{
	int HAS_CACHE;
	int NEED_CACHE;
	char cacheName[100];   //cache 文件名
	


}CACHE_t;


/************************************************************************/
/*functions Define                                                      */
/************************************************************************/


uint rewrite_buff(char* buff,int flag,DATAPORT_t* dataport_t,IPADDR_t* ipaddr_t);
int acceptSocket(int socket);
int connectToServer(char *ip,uint port);
int bindAndListenSocket(uint port);
uint find_local_port(int sockfd);
void find_local_ip(int sockfd,char* ip);
void find_peer_ip(int sockfd,char* ip);
void read_serverIP_from_file(IPADDR_t* ipaddr_t,char* filename);


uint has_cache(char* buff,CACHE_t * cache_t);
void is_need_cache(CACHE_t * cache_t);


/************************************************************************/
/*functions                                                             */
/************************************************************************/


//生成一个socket，监听本机所有地址 和 传入的端口
//如果传入的端口是0，则代表由系统自己分配一个端口
int  bindAndListenSocket(unsigned int port){

	struct sockaddr_in myaddress;
	int    socket_fd;

	if( (socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){  
		printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);  
		exit(1);  
	}  
	memset(&myaddress, 0, sizeof(struct sockaddr_in));  

	myaddress.sin_family = AF_INET;
	myaddress.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddress.sin_port = htons(port);

	if(bind(socket_fd,(struct sockaddr*)&myaddress,sizeof(struct sockaddr_in)) < 0){
		printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);  
		exit(1);
	}

	if(listen(socket_fd,10) < 0){
		printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);  
		exit(1);
	}

	return socket_fd;

}


int acceptCmdSocket(int socket_fd)
{
	int connect_fd;

	connect_fd = accept(socket_fd, (struct sockaddr*)NULL, NULL);
	if(connect_fd == -1){
		printf("accept socket error: %s(errno: %d)",strerror(errno),errno);  
		exit(1);
	}

	return connect_fd;
}



int connectToServer(char* ip,unsigned int port){


	int connect_fd;

	struct sockaddr_in servaddr;

	memset(&servaddr, 0, sizeof(struct sockaddr_in));  

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	inet_pton(AF_INET,ip,&servaddr.sin_addr);

	connect_fd = socket(AF_INET,SOCK_STREAM,0);

	if(connect(connect_fd,(struct sockaddr *)&servaddr,sizeof(struct sockaddr_in)) != -1){
		return connect_fd;
	}
	printf("connect socket error: %s(errno: %d)",strerror(errno),errno);  
	exit(1);

}

uint rewrite_buff(char* buff,int flag,DATAPORT_t* dataport_t,IPADDR_t* ipaddr_t){

	char* tmp = buff;
	int i;
	uint port,tmp_port;
	char tmp_ip[IPLEN] = {0};
	

	//server发向client的PASS命令回复
	if (PASV == flag)
	{
		tmp += 27;
	}
	//命令格式为ip,ip,ip,ip,port,port
	//把指针挪4次逗号
	tmp = strchr(tmp,',');
	tmp++;
	tmp = strchr(tmp,',');
	tmp++;
	tmp = strchr(tmp,',');
	tmp++;
	tmp = strchr(tmp,',');
	tmp++;
	//获得高位的port
	sscanf(tmp,"%u",&tmp_port);
	tmp = strchr(tmp,',');
	tmp++;
	//获得低位的port
	sscanf(tmp,"%u",&port);
	port = tmp_port * 256 + port;

	memcpy(tmp_ip,ipaddr_t->proxyIP,IPLEN);
	for(i=0;i<IPLEN;i++){
		if(tmp_ip[i]=='.')
			tmp_ip[i] = ',';

	}

	if(PORT == flag){

		//重写buff
		memset(buff,0,BUFFSIZE);
		sprintf(buff,"PORT %s,%d,%d\r\n",tmp_ip,dataport_t->ProxyServerDataPort/256,dataport_t->ProxyServerDataPort%256);
		dataport_t->ClientDataPort = port; //将主动模式下，客户端开启的数据监听port保存
	}
	else{

		memset(buff+27,0,BUFFSIZE-27);
		sprintf(buff+27,"%s,%d,%d).\r\n",tmp_ip,dataport_t->ProxyServerDataPort/256,dataport_t->ProxyServerDataPort%256);
		dataport_t->SeverDataPort= port; //将主动模式下，服务端开启的数据监听port保存

	}

}


uint find_local_port(int sockfd){

	struct sockaddr_in addr_i;
	socklen_t addr_i_len = sizeof(struct sockaddr_in);
	if (getsockname(sockfd,(struct sockaddr *)&addr_i,&addr_i_len) != -1)
	{
			return ntohs(addr_i.sin_port);
	}
	printf("find_local_port error: %s(errno: %d)",strerror(errno),errno);  
		exit(1);

}
void find_local_ip(int sockfd,char* ip){

	
	struct sockaddr_in addr_i;
	socklen_t addr_i_len = sizeof(struct sockaddr_in);
	memset(ip,0,IPLEN);
	
	if (getsockname(sockfd,(struct sockaddr *)&addr_i,&addr_i_len) != -1)
	{

		inet_ntop(AF_INET,&addr_i.sin_addr,ip,addr_i_len);
		return;
	}


	printf("getsockname error: %s(errno: %d)",strerror(errno),errno);  
	exit(1);

}
void find_peer_ip(int sockfd,char* ip){

	struct sockaddr_in addr_i;
	socklen_t addr_i_len = sizeof(struct sockaddr_in);
	memset(ip,0,IPLEN);

	if (getpeername(sockfd,(struct sockaddr *)&addr_i,&addr_i_len) != -1)
	{

		inet_ntop(AF_INET,&addr_i.sin_addr,ip,addr_i_len);
		return;
	}


	printf("getsockname error: %s(errno: %d)",strerror(errno),errno);  
	exit(1);



}



void read_serverIP_from_file(IPADDR_t* ipaddr_t,char* filename){


	FILE *fp;
	char text[1024]={0};
	char* find=NULL;
	fp=fopen(filename,  "r"); 

	fgets(text, 1024, fp);
	if(strlen(text)<IPLEN){

		find = strchr(text, '\n');          //查找换行符  
		if(find)                            //如果find不为空指针  
    		*find = '\0';   
		strcpy(ipaddr_t->serverIP,text);
	}
	else
		exit(1);

}



uint has_cache(char* buff,CACHE_t * cache_t){

	char *filename = strchr(buff,' ');
	if (filename != NULL){
		filename++;
		memset(cache_t->cacheName,0,100);
		strncpy(cache_t->cacheName,filename,strlen(filename)-2);
		if((access(cache_t->cacheName,F_OK))==0)
		{

			return TRUE;


		}
		else{

			printf("Cache %s not exit!!!",cache_t->cacheName);
			return False;
		}

	}

}


void is_need_cache(CACHE_t * cache_t){

	char *filename_type = strchr(cache_t->cacheName,'.');
	
	if (filename_type != NULL){
		filename_type++;
		if(0 == strncmp(filename_type,"pdf",strlen("pdf")) ||0 ==  strncmp(filename_type,"jpg",strlen("jpg")))
		{	
			//cache_t->NEED_CACHE = fopen(cache_t->cacheName, "wb");
			cache_t->NEED_CACHE = 1;


		}
		else{

			cache_t->NEED_CACHE = 0;

		}

	}


}

