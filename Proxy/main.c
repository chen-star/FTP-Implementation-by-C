

#include "main.h"

int main(int argc, const char *argv[])
{
    fd_set master_set, working_set;  //文件描述符集合
    struct timeval timeout;          //select 参数中的超时结构体
    int proxy_cmd_socket    = 0;     //proxy listen控制连接
    int accept_cmd_socket   = 0;     //proxy accept客户端请求的控制连接
    int connect_cmd_socket  = 0;     //proxy connect服务器建立控制连接
    int proxy_data_socket   = 0;     //proxy listen数据连接
    int accept_data_socket  = 0;     //proxy accept得到请求的数据连接（主动模式时accept得到服务器数据连接的请求，被动模式时accept得到客户端数据连接的请求）
    int connect_data_socket = 0;     //proxy connect建立数据连接 （主动模式时connect客户端建立数据连接，被动模式时connect服务器端建立数据连接）
    int selectResult = 0;     //select函数返回值
    int select_sd = 10;    //select 函数监听的最大文件描述符
    int i = 0;
    CACHE_t cache_t = {0};

    
    int ftpmode = PASV;   //默认被动模式
    IPADDR_t ipaddr_t;
    DATAPORT_t dataport_t;
    

    read_serverIP_from_file(&ipaddr_t,"config.txt");
    
    
    

    FD_ZERO(&master_set);   //清空master_set集合
    bzero(&timeout, sizeof(timeout));

    proxy_cmd_socket = bindAndListenSocket(FTPPORT);  //开启proxy_cmd_socket、bind（）、listen操作
    FD_SET(proxy_cmd_socket, &master_set);  //将proxy_cmd_socket加入master_set集合

    while (TRUE) {
        //printf("select!\n");
        timeout.tv_sec = 60;    //Select的超时结束时间
        timeout.tv_usec = 0;    //ms
        FD_ZERO(&working_set); //清空working_set文件描述符集合
        memcpy(&working_set, &master_set, sizeof(master_set)); //将master_set集合copy到working_set集合

        //select循环监听 这里只对读操作的变化进行监听（working_set为监视读操作描述符所建立的集合）,第三和第四个参数的NULL代表不对写操作、和误操作进行监听
        selectResult = select(select_sd, &working_set, NULL, NULL, &timeout);

        // fail
        if (selectResult < 0) {
            perror("select() failed\n");
            exit(1);
        }

        // timeout
        if (selectResult == 0) {
            printf("select() timed out.\n");
            continue;
        }

        // selectResult > 0 时 开启循环判断有变化的文件描述符为哪个socket
        for (i = 0; i < select_sd; i++) {
            //判断变化的文件描述符是否存在于working_set集合
            if (FD_ISSET(i, &working_set)) {

                if (i == proxy_cmd_socket) {
                    accept_cmd_socket = acceptCmdSocket(proxy_cmd_socket);  //执行accept操作,建立proxy和客户端之间的控制连接
                    connect_cmd_socket = connectToServer(ipaddr_t.serverIP,FTPPORT); //执行connect操作,建立proxy和服务器端之间的控制连接

                    //读取proxy自身的IP地址
                    find_local_ip(connect_cmd_socket,ipaddr_t.proxyIP);

                    //读取客户端的IP地址
                    find_peer_ip(accept_cmd_socket,ipaddr_t.clientIP);

                    printf("ftp server ip : %s\n",ipaddr_t.serverIP);
                    printf("ftp client ip : %s\n",ipaddr_t.clientIP);

                    //将新得到的socket加入到master_set结合中
                    FD_SET(accept_cmd_socket, &master_set);
                    FD_SET(connect_cmd_socket, &master_set);
                }

                if (i == accept_cmd_socket) {
                    char buff[BUFFSIZE] = {0};

                    if (read(i, buff, BUFFSIZE) == 0) {
                        close(i); //如果接收不到内容,则关闭Socket
                        close(connect_cmd_socket);

                        //socket关闭后，使用FD_CLR将关闭的socket从master_set集合中移去,使得select函数不再监听关闭的socket
                        FD_CLR(i, &master_set);
                        FD_CLR(connect_cmd_socket, &master_set);

                    } else {
                        printf("command received from client : %s\n",buff);
                        //如果接收到内容,则对内容进行必要的处理，之后发送给服务器端（写入connect_cmd_socket）

                        //处理客户端发给proxy的request，部分命令需要进行处理，如PORT、RETR、STOR                        
                        //PORT
                        //////////////
                        if (0 == strncmp(buff,"PORT",strlen("PORT")))    //需要修改命令中的ip和port
                        {
                            //主动模式下 代理服务器需要保存客户端发来的port
                            //并开启监听，并将此时系统分配的监听端口发送给ftp服务器
                            proxy_data_socket = bindAndListenSocket(SYSTEM_PORT);
                            FD_SET(proxy_data_socket, &master_set);

                            //根据系统生成的socket 来获取 ip地址和port

                            dataport_t.ProxyServerDataPort = find_local_port(proxy_data_socket);
                            ftpmode = PORT;  //变成主动模式
                            //重写buff的内容，并保存客户端的数据链路port
                            rewrite_buff(buff,PORT,&dataport_t,&ipaddr_t);
                            printf("command sent to server : %s\n",buff);
                            //写入proxy与server建立的cmd连接,除了PORT之外，直接转发buff内容
                            write(connect_cmd_socket, buff, strlen(buff));
                        }
                        //RETR 判断是否有缓存
                        //////////////
                        else if(0 == strncmp(buff,"RETR",strlen("RETR"))){

                            //判断是否有缓存
                            //如果有缓存，设置标志位
                            if(has_cache(buff,&cache_t)){
                                cache_t.HAS_CACHE = 1;
								
                                write(accept_cmd_socket,"150 Opening BINARY Mode data connection for file transfer.\r\n",strlen("150 Opening BINARY Mode data connection for file transfer..\r\n"));
                                if(ftpmode == PORT)
									goto PORT_DO;
                                    
                                    
                            }
                            //如果没有缓存，则通过ftp服务器获取，并标记pdf ,image 需要缓存
                            else{
                                is_need_cache(&cache_t);
                                printf("command sent to server : %s\n",buff);
                                //写入proxy与server建立的cmd连接,除了PORT之外，直接转发buff内容
                                write(connect_cmd_socket, buff, strlen(buff));
                            }
                            
                            
                        }
                        
                        else{

                                                
                            printf("command sent to server : %s\n",buff);
                            //写入proxy与server建立的cmd连接,除了PORT之外，直接转发buff内容
                            write(connect_cmd_socket, buff, strlen(buff));

                        }
                        //STOR 无需处理
                        //////////////

                    }
                }

                if (i == connect_cmd_socket) {
                    //处理服务器端发给proxy的reply，写入accept_cmd_socket
                    char buff[BUFFSIZE] = {0};
                    if(read(i,buff,BUFFSIZE) == 0){
                        close(i);
                        close(accept_cmd_socket);
                        FD_CLR(i,&master_set);
                        FD_CLR(accept_cmd_socket,&master_set);
                    }

                    printf("reply received from server : %s\n",buff);
                    //PASV收到的端口 227 （port）被动模式
                    //////////////
                    if(0 == strncmp(buff,"227",strlen("227"))){
                        proxy_data_socket = bindAndListenSocket(SYSTEM_PORT); //在proxy上监听端口，等待client端发起accept
                        FD_SET(proxy_data_socket, &master_set);//将proxy_data_socket加入master_set集合
                        dataport_t.ProxyServerDataPort= find_local_port(proxy_data_socket);
                        //重写buff的内容，并保存服务器端的数据链路port
                        rewrite_buff(buff,PASV,&dataport_t,&ipaddr_t);
                        
                    }
                    printf("reply sent to client : %s\n",buff);

                    write(accept_cmd_socket,buff,strlen(buff));
                }

                if (i == proxy_data_socket) {
                    //建立data连接(accept_data_socket、connect_data_socket)
                    if(ftpmode == PASV){            //被动模式
                        //接收客户端主动发起的socket连接
                        accept_data_socket = acceptCmdSocket(proxy_data_socket);
                        FD_SET(accept_data_socket, &master_set);
                    
                        printf("data connectiong established\n");
                        //proxy主动向ftp服务器发起数据socket连接无cache的情况下
                        if(cache_t.HAS_CACHE == 0){
                            connect_data_socket = connectToServer(ipaddr_t.serverIP,dataport_t.SeverDataPort);
                            FD_SET(connect_data_socket, &master_set);
                        }
                        else{
                            //往客户端发数据并关闭Cache开关
                            //直接把缓存发送给客户端，不与ftp服务器数据传输
                                                        //往客户端发数据并关闭Cache开关
                            //往客户端发数据并关闭Cache开关
                            printf("Proxy begin sent cache %s\n",cache_t.cacheName);
                            int fd =  open(cache_t.cacheName, O_RDONLY);
                            //int fd = open(cache_t->cacheName, O_RDONLY);

                        
                            char buffer[BUFFSIZE];  
                            int bytes_read;
                            printf("fd:%d len:%d \n",fd,bytes_read);
                            while ((bytes_read = read(fd,buffer,BUFFSIZE))!= 0) {  
                                
                                write(accept_data_socket,buffer,bytes_read);
                                //printf("len:%d ",bytes_read);
                                //usleep(100);
                            }
                            
                            //write(connect_data_socket,'\0',1);
                            printf("Proxy end sent cache \n");
                            

        
                            close(accept_data_socket);
                            FD_CLR(accept_data_socket, &master_set);
                            close(fd);
                            //close(proxy_data_socket);
                            //FD_CLR(proxy_data_socket,&master_set);
                            
                            write(accept_cmd_socket,"226 transfer complete.\r\n",strlen("226 transfer complete.\r\n"));
                            cache_t.HAS_CACHE = 0;

                        }
                    }
                    else{    //主动模式
                    
                     PORT_DO:    
                        //proxy主动向客户端发起数据socket连接
                        connect_data_socket = connectToServer(ipaddr_t.clientIP,dataport_t.ClientDataPort);
                        FD_SET(connect_data_socket, &master_set);
                        //无cache的情况下接收服务器端主动发起的socket连接
                        if(cache_t.HAS_CACHE == 0){
                            //printf("acceptCmdSocket accept_data_socket !\n");
                            accept_data_socket = acceptCmdSocket(proxy_data_socket);
                            FD_SET(accept_data_socket, &master_set);
                            
                        }
                        else{
                            //往客户端发数据并关闭Cache开关
                            printf("Proxy begin sent cache %s\n",cache_t.cacheName);
                            int fd =  open(cache_t.cacheName, O_RDONLY);
                            //int fd = open(cache_t->cacheName, O_RDONLY);

                        
                            char buffer[BUFFSIZE];  
                            int bytes_read;
                            printf("fd:%d len:%d \n",fd,bytes_read);
                            while ((bytes_read = read(fd,buffer,BUFFSIZE))!= 0) {  
                                
                                write(connect_data_socket,buffer,bytes_read);
                                //printf("len:%d ",bytes_read);
                                //usleep(100);
                            }
                            
                            //write(connect_data_socket,'\0',1);
                            printf("Proxy end sent cache \n");
                            

        
                            close(connect_data_socket);
                            FD_CLR(connect_data_socket, &master_set);
                            close(fd);
                            //close(proxy_data_socket);
                            //FD_CLR(proxy_data_socket,&master_set);
                            
                            write(accept_cmd_socket,"226 transfer complete.\r\n",strlen("226 transfer complete.\r\n"));
                            cache_t.HAS_CACHE = 0;

                        }
                            
                    }

                }

                if (i == accept_data_socket) {
                    //判断主被动和传输方式（上传、下载）决定如何传输数据
                    char buff[BUFFSIZE] = {0};
                    int len = read(i,buff,BUFFSIZE);
                    printf("accept_data_socket len == %d \n",len);
                    if( len == 0){
                        if(cache_t.NEED_CACHE == 1){
                            printf("Proxy end save cache \n");
                             cache_t.NEED_CACHE = 0;
                        }  
                        close(i);
                        close(connect_data_socket);
                        close(proxy_data_socket);
                        FD_CLR(proxy_data_socket,&master_set);
                        FD_CLR(i, &master_set);
                        FD_CLR(connect_data_socket, &master_set);
                    }
                    else{
                        //往反方向传数据
                        write(connect_data_socket,buff,len);
                        //往缓存写
                        if(cache_t.NEED_CACHE == 1){
                            printf("Proxy begin save cache %s\n",cache_t.cacheName);
                             //write(cache_t.NEED_CACHE,buff,len);
                             FILE* fd =  fopen(cache_t.cacheName, "a+b");
                             fwrite(buff,len,1,fd);
                             fclose(fd);

                        }
                    }
                }

                if (i == connect_data_socket) {
                    //判断主被动和传输方式（上传、下载）决定如何传输数据
                    char buff[BUFFSIZE] = {0};
                    int len = read(i,buff,BUFFSIZE);
                    printf("connect_data_socket len == %d \n",len);
                    if( len == 0){
                        if(cache_t.NEED_CACHE == 1){
                            printf("Proxy end save cache \n");
                             cache_t.NEED_CACHE = 0;
                        }  
                        close(i);
                        close(accept_data_socket);
                        close(proxy_data_socket);
                        FD_CLR(proxy_data_socket,&master_set);
                        FD_CLR(i, &master_set);
                        FD_CLR(accept_data_socket, &master_set);
                    }
                    else{
                        //往反方向传数据
                        write(accept_data_socket,buff,len);
                        //往缓存写
                        if(cache_t.NEED_CACHE == 1){
                            printf("Proxy begin save cache %s\n",cache_t.cacheName);
                            FILE* fd =  fopen(cache_t.cacheName, "a+b");
                             fwrite(buff,len,1,fd);
                             fclose(fd);

                        }
                    }
                }
            }
        }
    }

    return 0;
}
