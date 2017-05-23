

#include "main.h"

int main(int argc, const char *argv[])
{
    fd_set master_set, working_set;  //�ļ�����������
    struct timeval timeout;          //select �����еĳ�ʱ�ṹ��
    int proxy_cmd_socket    = 0;     //proxy listen��������
    int accept_cmd_socket   = 0;     //proxy accept�ͻ�������Ŀ�������
    int connect_cmd_socket  = 0;     //proxy connect������������������
    int proxy_data_socket   = 0;     //proxy listen��������
    int accept_data_socket  = 0;     //proxy accept�õ�������������ӣ�����ģʽʱaccept�õ��������������ӵ����󣬱���ģʽʱaccept�õ��ͻ����������ӵ�����
    int connect_data_socket = 0;     //proxy connect������������ ������ģʽʱconnect�ͻ��˽����������ӣ�����ģʽʱconnect�������˽����������ӣ�
    int selectResult = 0;     //select��������ֵ
    int select_sd = 10;    //select ��������������ļ�������
    int i = 0;
    CACHE_t cache_t = {0};

    
    int ftpmode = PASV;   //Ĭ�ϱ���ģʽ
    IPADDR_t ipaddr_t;
    DATAPORT_t dataport_t;
    

    read_serverIP_from_file(&ipaddr_t,"config.txt");
    
    
    

    FD_ZERO(&master_set);   //���master_set����
    bzero(&timeout, sizeof(timeout));

    proxy_cmd_socket = bindAndListenSocket(FTPPORT);  //����proxy_cmd_socket��bind������listen����
    FD_SET(proxy_cmd_socket, &master_set);  //��proxy_cmd_socket����master_set����

    while (TRUE) {
        //printf("select!\n");
        timeout.tv_sec = 60;    //Select�ĳ�ʱ����ʱ��
        timeout.tv_usec = 0;    //ms
        FD_ZERO(&working_set); //���working_set�ļ�����������
        memcpy(&working_set, &master_set, sizeof(master_set)); //��master_set����copy��working_set����

        //selectѭ������ ����ֻ�Զ������ı仯���м�����working_setΪ���Ӷ������������������ļ��ϣ�,�����͵��ĸ�������NULL������д����������������м���
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

        // selectResult > 0 ʱ ����ѭ���ж��б仯���ļ�������Ϊ�ĸ�socket
        for (i = 0; i < select_sd; i++) {
            //�жϱ仯���ļ��������Ƿ������working_set����
            if (FD_ISSET(i, &working_set)) {

                if (i == proxy_cmd_socket) {
                    accept_cmd_socket = acceptCmdSocket(proxy_cmd_socket);  //ִ��accept����,����proxy�Ϳͻ���֮��Ŀ�������
                    connect_cmd_socket = connectToServer(ipaddr_t.serverIP,FTPPORT); //ִ��connect����,����proxy�ͷ�������֮��Ŀ�������

                    //��ȡproxy�����IP��ַ
                    find_local_ip(connect_cmd_socket,ipaddr_t.proxyIP);

                    //��ȡ�ͻ��˵�IP��ַ
                    find_peer_ip(accept_cmd_socket,ipaddr_t.clientIP);

                    printf("ftp server ip : %s\n",ipaddr_t.serverIP);
                    printf("ftp client ip : %s\n",ipaddr_t.clientIP);

                    //���µõ���socket���뵽master_set�����
                    FD_SET(accept_cmd_socket, &master_set);
                    FD_SET(connect_cmd_socket, &master_set);
                }

                if (i == accept_cmd_socket) {
                    char buff[BUFFSIZE] = {0};

                    if (read(i, buff, BUFFSIZE) == 0) {
                        close(i); //������ղ�������,��ر�Socket
                        close(connect_cmd_socket);

                        //socket�رպ�ʹ��FD_CLR���رյ�socket��master_set��������ȥ,ʹ��select�������ټ����رյ�socket
                        FD_CLR(i, &master_set);
                        FD_CLR(connect_cmd_socket, &master_set);

                    } else {
                        printf("command received from client : %s\n",buff);
                        //������յ�����,������ݽ��б�Ҫ�Ĵ���֮���͸��������ˣ�д��connect_cmd_socket��

                        //����ͻ��˷���proxy��request������������Ҫ���д�����PORT��RETR��STOR                        
                        //PORT
                        //////////////
                        if (0 == strncmp(buff,"PORT",strlen("PORT")))    //��Ҫ�޸������е�ip��port
                        {
                            //����ģʽ�� �����������Ҫ����ͻ��˷�����port
                            //������������������ʱϵͳ����ļ����˿ڷ��͸�ftp������
                            proxy_data_socket = bindAndListenSocket(SYSTEM_PORT);
                            FD_SET(proxy_data_socket, &master_set);

                            //����ϵͳ���ɵ�socket ����ȡ ip��ַ��port

                            dataport_t.ProxyServerDataPort = find_local_port(proxy_data_socket);
                            ftpmode = PORT;  //�������ģʽ
                            //��дbuff�����ݣ�������ͻ��˵�������·port
                            rewrite_buff(buff,PORT,&dataport_t,&ipaddr_t);
                            printf("command sent to server : %s\n",buff);
                            //д��proxy��server������cmd����,����PORT֮�⣬ֱ��ת��buff����
                            write(connect_cmd_socket, buff, strlen(buff));
                        }
                        //RETR �ж��Ƿ��л���
                        //////////////
                        else if(0 == strncmp(buff,"RETR",strlen("RETR"))){

                            //�ж��Ƿ��л���
                            //����л��棬���ñ�־λ
                            if(has_cache(buff,&cache_t)){
                                cache_t.HAS_CACHE = 1;
								
                                write(accept_cmd_socket,"150 Opening BINARY Mode data connection for file transfer.\r\n",strlen("150 Opening BINARY Mode data connection for file transfer..\r\n"));
                                if(ftpmode == PORT)
									goto PORT_DO;
                                    
                                    
                            }
                            //���û�л��棬��ͨ��ftp��������ȡ�������pdf ,image ��Ҫ����
                            else{
                                is_need_cache(&cache_t);
                                printf("command sent to server : %s\n",buff);
                                //д��proxy��server������cmd����,����PORT֮�⣬ֱ��ת��buff����
                                write(connect_cmd_socket, buff, strlen(buff));
                            }
                            
                            
                        }
                        
                        else{

                                                
                            printf("command sent to server : %s\n",buff);
                            //д��proxy��server������cmd����,����PORT֮�⣬ֱ��ת��buff����
                            write(connect_cmd_socket, buff, strlen(buff));

                        }
                        //STOR ���账��
                        //////////////

                    }
                }

                if (i == connect_cmd_socket) {
                    //����������˷���proxy��reply��д��accept_cmd_socket
                    char buff[BUFFSIZE] = {0};
                    if(read(i,buff,BUFFSIZE) == 0){
                        close(i);
                        close(accept_cmd_socket);
                        FD_CLR(i,&master_set);
                        FD_CLR(accept_cmd_socket,&master_set);
                    }

                    printf("reply received from server : %s\n",buff);
                    //PASV�յ��Ķ˿� 227 ��port������ģʽ
                    //////////////
                    if(0 == strncmp(buff,"227",strlen("227"))){
                        proxy_data_socket = bindAndListenSocket(SYSTEM_PORT); //��proxy�ϼ����˿ڣ��ȴ�client�˷���accept
                        FD_SET(proxy_data_socket, &master_set);//��proxy_data_socket����master_set����
                        dataport_t.ProxyServerDataPort= find_local_port(proxy_data_socket);
                        //��дbuff�����ݣ�������������˵�������·port
                        rewrite_buff(buff,PASV,&dataport_t,&ipaddr_t);
                        
                    }
                    printf("reply sent to client : %s\n",buff);

                    write(accept_cmd_socket,buff,strlen(buff));
                }

                if (i == proxy_data_socket) {
                    //����data����(accept_data_socket��connect_data_socket)
                    if(ftpmode == PASV){            //����ģʽ
                        //���տͻ������������socket����
                        accept_data_socket = acceptCmdSocket(proxy_data_socket);
                        FD_SET(accept_data_socket, &master_set);
                    
                        printf("data connectiong established\n");
                        //proxy������ftp��������������socket������cache�������
                        if(cache_t.HAS_CACHE == 0){
                            connect_data_socket = connectToServer(ipaddr_t.serverIP,dataport_t.SeverDataPort);
                            FD_SET(connect_data_socket, &master_set);
                        }
                        else{
                            //���ͻ��˷����ݲ��ر�Cache����
                            //ֱ�Ӱѻ��淢�͸��ͻ��ˣ�����ftp���������ݴ���
                                                        //���ͻ��˷����ݲ��ر�Cache����
                            //���ͻ��˷����ݲ��ر�Cache����
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
                    else{    //����ģʽ
                    
                     PORT_DO:    
                        //proxy������ͻ��˷�������socket����
                        connect_data_socket = connectToServer(ipaddr_t.clientIP,dataport_t.ClientDataPort);
                        FD_SET(connect_data_socket, &master_set);
                        //��cache������½��շ����������������socket����
                        if(cache_t.HAS_CACHE == 0){
                            //printf("acceptCmdSocket accept_data_socket !\n");
                            accept_data_socket = acceptCmdSocket(proxy_data_socket);
                            FD_SET(accept_data_socket, &master_set);
                            
                        }
                        else{
                            //���ͻ��˷����ݲ��ر�Cache����
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
                    //�ж��������ʹ��䷽ʽ���ϴ������أ�������δ�������
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
                        //������������
                        write(connect_data_socket,buff,len);
                        //������д
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
                    //�ж��������ʹ��䷽ʽ���ϴ������أ�������δ�������
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
                        //������������
                        write(accept_data_socket,buff,len);
                        //������д
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
