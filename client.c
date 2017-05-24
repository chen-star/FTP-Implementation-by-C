/* header files */
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>  /* getservbyname(), gethostbyname() */
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h> 
#include <string.h>
#include <termios.h> 
#include <netinet/in.h> 
#include <signal.h>
#include <sys/ioctl.h> 
#include <linux/if.h> 
#include <sys/stat.h>  /* for definition of errno */
#include <fcntl.h>
#include <ctype.h>
#include <time.h>

/* define macros*/
#define MAXBUF	        1024
#define STDIN_FILENO	0
#define STDOUT_FILENO	1

/* define FTP reply code */
#define USERNAME	220
#define PASSWORD	331
#define LOGIN		230
#define PATHNAME	257
#define CLOSEDATA	226
#define ACTIONOK	250
#define FILE_MODE       0644
#define on              1  
#define off             0   

/* define user command */  
#define ls              1111
#define get             2222
#define put             3333
#define trans           4444 

/* DefinE global variables */
char	*host;		/* hostname or dotted-decimal string */
char	*port;
char	*rbuf, *rbuf1;	/* pointer that is malloc'ed */
char	*wbuf, *wbuf1;	/* pointer that is malloc'ed */

struct  sockaddr_in servaddr;
int     passive=on; /*identify passive mode and active mode*/
int     type=off; /*identify the command used to switch mode*/ //complement~~~
int     file_size=0;
int     judge; 

int	cliopen(char *host, char *port);
int     get_file_size(char *str); //for progress_bar
void	strtosrv(char *str, char *host, char *port);
void	cmd_tcp(int sockfd);
void	ftp_list(int sockfd);
int	ftp_get(int sck, char *pDownloadFileName_s);
int	ftp_put (int sck, char *pUploadFileName_s);
void    help();

int
main(int argc, char *argv[])
{
    int fd;
    if (0 != argc-2) {
      printf("%s\n","missing <hostname>");
      exit(0);
    }

    host = argv[1];
    port = "21";

    //code here: Allocate the read and write buffers before open().
    rbuf = (char *)malloc(MAXBUF);
    rbuf1 = (char *)malloc(MAXBUF);
    wbuf = (char *)malloc(MAXBUF);
    wbuf1 = (char *)malloc(MAXBUF);
             
    printf("\n***********************************************************************");
    printf("\n*                           △~~~~~~△ ");
    printf("\n*                          S        S");
    printf("\n*                          S ` I `  S                                  ");
    printf("\n*                          S   ~    S       \----------|               ");
    printf("\n*                          S        S       |  mie~!!  |               ");
    printf("\n*                          S        S       \----------/               ");
    printf("\n*                          S        S                                  ");
    printf("\n*                          S        %~~~~~~~~~~○");
    printf("\n*                          S                     S                     ");
    printf("\n*                          S                     S                     ");
    printf("\n*                           S                   S                      ");
    printf("\n*                            s  s  s~~~~~~s  s  s                      ");
    printf("\n*                             s ss s      s ss s                       ");
    printf("\n*                                                                   ");
    printf("\n*                              FTPclient v1.2                          ");
    printf("\n*              Copyright by Longfei Ma & Airui Li in 2013/6/6          ");
    printf("\n*----------------------------------------------------------------------");
    printf("\n*                             server: %s \n",host                       );
    printf("\n***********************************************************************");
    printf("\n");

    fd = cliopen(host, port);
    cmd_tcp(fd);
    exit(0);
}


/* Establish a TCP connection from client to server */
int
cliopen(char *host, char *port)
{
    int sock_fd;
    struct in_addr addr;
    struct hostent *s;

    /*check if the socket success*/
    if((sock_fd=socket(PF_INET,SOCK_STREAM,0))<0) {
      printf("socket() fail.\n");
      exit(0);
    }

    /*Decide the input is address or IP*/
    judge=inet_aton(host,&addr);

    /*if the imput is name address*/
    if(judge==0) {
      s=gethostbyname(host);
      servaddr.sin_addr.s_addr=inet_addr(inet_ntoa(*((struct in_addr*)s->h_addr_list[0])));
    }

    /*If the input is IP address*/
    else {
      servaddr.sin_addr.s_addr=inet_addr(host);
    }

    /*fill the serv struct*/
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(atoi(port));

    /*connect and check if it be successful*/
    judge=connect(sock_fd,(struct sockaddr*)&servaddr,sizeof(servaddr));
    if(judge<0) {
      printf("connect is fail--T_T--\n");
      exit(1);
    }
    printf("Connect to server successful--@^____^@--\n");

    return sock_fd;
}

int get_file_size(char *str)
{
   char a[10];
   int  i,k=0;
   int flag=0;
   for(i=0;str[i]!='\0';i++) {
      if(flag) {
        if(48<=str[i]&&str[i]<=57) {
	  a[k]=str[i];
	  k++;
	}
        if(isspace(str[i]))
        a[k]='\0';
      }
      if(str[i]==40)
      flag=1;
   }
   i=atoi(a);
   return i;		
}

/*
Compute server's port by a pair of integers and store it in char *port
Get server's IP address and store it in char *host
*/
void
strtosrv(char *str, char *host, char *port)
{

    //code here
    char *str1 = ".";   //define a character"."
    char *str2,*str3,*str4, *str5,*str6,*str7,*str8;         //define six characters for future use

    str2 = strstr(str, "(");    // find if there is "(" in str, if it has, return the value of str  after "(".
    str2 = str2+1;              // get the next value after "("
    str3 = strtok(str2,",");    //split the str2 by the delimiter","(h1,h2,h3,h4,p1,p2)
    str4 = strtok(NULL,",");    // store the h1 to p2 value in str3 to str8 seperately
    str5 = strtok(NULL,",");
    str6 = strtok(NULL,",");
    str7 = strtok(NULL,",");
    str8 = strtok(NULL,",");
    str8 = strtok(str8,")");

    /* Get server's IP address and store it in char *host */
    char *ip;
    ip = (char *)malloc(50);  //define a buffer for record IP
    memset(ip,0,strlen(ip));           //clear the buffer
    strcat(ip,str3);         // str1 is "."
    strcat(ip,str1);         //str3,4,5,6 hold ip address
    strcat(ip,str4);
    strcat(ip,str1);
    strcat(ip,str5);
    strcat(ip,str1); 
    strcat(ip,str6);           //this results in h1.h2.h3.h4
    sprintf(host, "%s", ip);

    /* Compute server's port by a pair of integers and store it in char *port */

    int sol=atoi(str7)*256+atoi(str8);  //str7 and str8 hold port information
    sprintf(port, "%d", sol);
}

/* Read and write as command connection */
void
cmd_tcp(int sockfd)
{
   int maxfdp1, nread, nwrite, fd, replycode, status;
   char host[16];
   char port[6];
   fd_set rset;
   char *rem_file,*rem_dir;
   char *loc_dir,* loc_file;
   rem_file=(char *)malloc(100);

   loc_dir=(char *)malloc(100);
   loc_file=(char *)malloc(100);
   rem_dir=(char *)malloc(100);
   FD_ZERO(&rset);
   maxfdp1 = sockfd + 1; /* check descriptors [0..sockfd] */
   struct termios new_settings,stored_settings;

   for( ; ; ) { 
      FD_SET(STDIN_FILENO, &rset);
      FD_SET(sockfd, &rset);
      if(select(maxfdp1, &rset, NULL, NULL, NULL) < 0)
      printf("select error\n");

      /* data to read on stdin */
      if(FD_ISSET(STDIN_FILENO, &rset)) {
        memset(rbuf,0,MAXBUF);

        if((nread = read(STDIN_FILENO, rbuf, MAXBUF)) < 0)
        printf("read error from stdin\n");
        nwrite = nread+5;

        /* send username */
        if(replycode == USERNAME) {
          sprintf(wbuf, "USER %s", rbuf);
          if(write(sockfd, wbuf, nwrite) != nwrite)
          printf("write error\n");
        }

        /* send password */
        if(replycode == PASSWORD) {
          sprintf(wbuf1, "PASS %s", rbuf);
          if(write(sockfd, wbuf1, nwrite) != nwrite)
          printf("write error\n");
          printf("\n");
        }

        /* send command */
        if(replycode==LOGIN || replycode==CLOSEDATA || replycode==PATHNAME || replycode==ACTIONOK) {

          /* ls - list files and directories*/
          if(strncmp(rbuf, "ls", 2) == 0) {
           if(*(rbuf+2) =='\n') {
              char *dir; 
              if(passive==on) {
                sprintf(wbuf, "%s", "PASV\n");
                write(sockfd, wbuf, strlen(wbuf));
              }
              strtok(rbuf, " ");
              dir = strtok(NULL," "); 
              if(dir == NULL)
              sprintf(wbuf1, "%s", "LIST -al\n");
              else
              sprintf(wbuf1,"LIST %s",dir);
              nwrite = strlen(wbuf1);
              status = ls;
              continue;
            }
            else if(*(rbuf+2) ==' ') {
              char *dir; 
              if(passive==on) {
                sprintf(wbuf, "%s", "PASV\n");
                write(sockfd, wbuf, strlen(wbuf));
              }
              strtok(rbuf, " ");
              dir = strtok(NULL," "); 
              if(dir == NULL)
              sprintf(wbuf1, "%s", "LIST -al\n");
              else
              sprintf(wbuf1,"LIST %s",dir);
              nwrite = strlen(wbuf1);
              status = ls;
              continue;
            }
            else {
              memset(rbuf,0,strlen(rbuf));
              strcat(rbuf,"Invalid command\nftpml>");
              nread += 24; 
              write(STDOUT_FILENO, rbuf, strlen(rbuf)); 
              continue; 
            }
          }

          /* help - list the command information */
          else if(strncmp(rbuf, "help", 4) == 0) {
            if(*(rbuf+4) =='\n') {
              help();
              memset(rbuf,0,strlen(rbuf));
              strcat(rbuf,"ftpml>");
              nread += 7; 
              write(STDOUT_FILENO, rbuf, strlen(rbuf)); 
              continue;
            }
            else {
              memset(rbuf,0,strlen(rbuf));
              strcat(rbuf,"Invalid command\nftpml>");
              nread += 24; 
              write(STDOUT_FILENO, rbuf, strlen(rbuf)); 
              continue; 
            }
          }

          /* code here: cd - change working directory */ 
          else if(strncmp(rbuf, "cd", 2) == 0) {
           if(*(rbuf+2) ==' ') {
              char *dir;
              strtok(rbuf, " ");
              dir = strtok(NULL," ");
              if(dir == NULL) {
                memset(rbuf,0,strlen(rbuf));
                strcat(rbuf,"Usage: cd dir_name\nftpml>");
                nread += 27; 
                write(STDOUT_FILENO, rbuf, strlen(rbuf));
                continue; 
              }
              sprintf(wbuf, "CWD %s", dir); 
              write(sockfd, wbuf, strlen(wbuf));
              continue;
            }
            else if(*(rbuf+2) =='\n') {
              char *dir;
              strtok(rbuf, " ");
              dir = strtok(NULL," ");
              if(dir == NULL) {
                memset(rbuf,0,strlen(rbuf));
                strcat(rbuf,"Usage: cd dir_name\nftpml>");
                nread += 27; 
                write(STDOUT_FILENO, rbuf, strlen(rbuf));
                continue; 
              }
              sprintf(wbuf, "CWD %s", dir); 
              write(sockfd, wbuf, strlen(wbuf));
              continue;
            }
            else {
              memset(rbuf,0,strlen(rbuf));
              strcat(rbuf,"Invalid command\nftpml>");
              nread += 24; 
              write(STDOUT_FILENO, rbuf, strlen(rbuf)); 
              continue; 
            }
          }

          /* pwd - print working directory */
          else if(strncmp(rbuf, "pwd", 3) == 0) {
            if(*(rbuf+3) =='\n') {
              sprintf(wbuf, "%s", "PWD\n");
              write(sockfd, wbuf, 4);
              continue;
            }
            else {
              memset(rbuf,0,strlen(rbuf));
              strcat(rbuf,"Invalid command\nftpml>");
              nread += 24; 
              write(STDOUT_FILENO, rbuf, strlen(rbuf)); 
              continue; 
            }
          }

          /* code here: mkdir - make a new directory */
          else if(strncmp(rbuf, "mkdir", 5) == 0) {
            if(*(rbuf+5) ==' ') {
              char *dir;
              strtok(rbuf, " ");
              dir = strtok(NULL," ");
              if(dir == NULL) {
                memset(rbuf,0,strlen(rbuf));
                strcat(rbuf,"Usage: mkdir dir_name\nftpml>");
                nread += 31; 
                write(STDOUT_FILENO, rbuf, strlen(rbuf));
                continue;
              }
              sprintf(wbuf, "MKD %s", dir); 
              write(sockfd, wbuf, strlen(wbuf));
              continue;
            }
            else if(*(rbuf+5) =='\n') {
              char *dir;
              strtok(rbuf, " ");
              dir = strtok(NULL," ");
              if(dir == NULL) {
                memset(rbuf,0,strlen(rbuf));
                strcat(rbuf,"Usage: mkdir dir_name\nftpml>");
                nread += 31; 
                write(STDOUT_FILENO, rbuf, strlen(rbuf));
                continue;
              }
              sprintf(wbuf, "MKD %s", dir); 
              write(sockfd, wbuf, strlen(wbuf));
              continue;
            }
            else {
              memset(rbuf,0,strlen(rbuf));
              strcat(rbuf,"Invalid command\nftpml>");
              nread += 24; 
              write(STDOUT_FILENO, rbuf, strlen(rbuf)); 
              continue; 
            }
          }

          /* code here: rmdir - remove a directory */
          else if(strncmp(rbuf, "rmdir", 5) == 0) {
            if(*(rbuf+5) ==' ') {
              char *dir;
              strtok(rbuf, " ");
              dir = strtok(NULL," ");
              if(dir == NULL) {
                memset(rbuf,0,strlen(rbuf));
                strcat(rbuf,"Usage: rmdir dir_name\nftpml>");
                nread += 31; 
                write(STDOUT_FILENO, rbuf, strlen(rbuf));
                continue;
              }
              sprintf(wbuf, "RMD %s", dir); 
              write(sockfd, wbuf, strlen(wbuf));
              continue;
            }
            else if(*(rbuf+5) =='\n') {
              char *dir;
              strtok(rbuf, " ");
              dir = strtok(NULL," ");
              if(dir == NULL) {
                memset(rbuf,0,strlen(rbuf));
                strcat(rbuf,"Usage: rmdir dir_name\nftpml>");
                nread += 31; 
                write(STDOUT_FILENO, rbuf, strlen(rbuf));
                continue;
              }
              sprintf(wbuf, "RMD %s", dir); 
              write(sockfd, wbuf, strlen(wbuf));
              continue;
            }
            else {
              memset(rbuf,0,strlen(rbuf));
              strcat(rbuf,"Invalid command\nftpml>");
              nread += 24; 
              write(STDOUT_FILENO, rbuf, strlen(rbuf)); 
              continue; 
            }
          }

          /* code here: delete - delete a file */
          else if(strncmp(rbuf, "delete", 6) == 0) {
            if(*(rbuf+6) ==' ') {
              char *dir;
              strtok(rbuf, " ");
              dir = strtok(NULL," ");
              if(dir == NULL) {
                memset(rbuf,0,strlen(rbuf));
                strcat(rbuf,"Usage: delete filename\nftpml>");
                nread += 32; 
                write(STDOUT_FILENO, rbuf, strlen(rbuf));
                continue;
              }
              sprintf(wbuf, "DELE %s", dir); 
              write(sockfd, wbuf, strlen(wbuf));
              continue;
            }
            else if(*(rbuf+6) =='\n') {
              char *dir;
              strtok(rbuf, " ");
              dir = strtok(NULL," ");
              if(dir == NULL) {
                memset(rbuf,0,strlen(rbuf));

                strcat(rbuf,"Usage: delete filename\nftpml>");
                nread += 32; 
                write(STDOUT_FILENO, rbuf, strlen(rbuf));
                continue;
              }
              sprintf(wbuf, "DELE %s", dir); 
              write(sockfd, wbuf, strlen(wbuf));
              continue;
            }
            else {
              memset(rbuf,0,strlen(rbuf));
              strcat(rbuf,"Invalid command\nftpml>");
              nread += 24; 
              write(STDOUT_FILENO, rbuf, strlen(rbuf)); 
              continue; 
            }
          }

          /* code here: rename - rename a file */
          else if (strncmp(rbuf, "rename", 6) == 0){
            if(*(rbuf+6) ==' ') {
              char *src,*dest;
              strtok(rbuf, " ");
              src = strtok(NULL," ");
              if(src == NULL) {
                memset(rbuf,0,strlen(rbuf));
                strcat(rbuf,"Usage: rename Raw_filename [New_filename]\nftpml>");
                nread += 50; 
                write(STDOUT_FILENO, rbuf, strlen(rbuf));
                continue;
              }
              dest = strtok(NULL," ");
              if(dest==NULL)
              dest=src;
              sprintf(wbuf, "RNFR %s\n", src); 
              write(sockfd, wbuf, strlen(wbuf));
              sprintf(wbuf1, "RNTO %s", dest);
              nwrite = 5 + strlen(dest);
              continue;
            }
            else if(*(rbuf+6) =='\n') {
              char *src,*dest;
              strtok(rbuf, " ");
              src = strtok(NULL," ");
              if(src == NULL) {
                memset(rbuf,0,strlen(rbuf));
                strcat(rbuf,"Usage: rename Raw_filename [New_filename]\nftpml>");
                nread += 50; 
                write(STDOUT_FILENO, rbuf, strlen(rbuf));
                continue;
              }
              dest = strtok(NULL," ");
              if(dest==NULL)
              dest=src;
              sprintf(wbuf, "RNFR %s\n", src); 
              write(sockfd, wbuf, strlen(wbuf));
              sprintf(wbuf1, "RNTO %s", dest);
              nwrite = 5 + strlen(dest);
              continue;
            }
            else {
              memset(rbuf,0,strlen(rbuf));
              strcat(rbuf,"Invalid command\nftpml>");
              nread += 24; 
              write(STDOUT_FILENO, rbuf, strlen(rbuf)); 
              continue; 
            }
          }

          /* code here: quit - quit from ftp server */
          else if(strncmp(rbuf, "quit", 4) == 0||strncmp(rbuf, "exit", 4) == 0){
            if(*(rbuf+4) =='\n') {
              sprintf(wbuf, "%s","QUIT\n"); 
              write(sockfd, wbuf, 5);
              continue;
            }
            else {
              memset(rbuf,0,strlen(rbuf));
              strcat(rbuf,"Invalid command\nftpml>");
              nread += 24; 
              write(STDOUT_FILENO, rbuf, strlen(rbuf)); 
              continue; 
            }
          }

          //support for binary mode~~~~
          else if(strncmp(rbuf, "binary", 6) == 0) { //for binary transport-----------
            if(*(rbuf+6) =='\n') {
	      type=0;
	      sprintf(wbuf, "%s", "Change to binary mode,enter next command@^____^@\nftpml>");
	      write(STDOUT_FILENO, wbuf, 56);
	      continue;
            }
            else {
              memset(rbuf,0,strlen(rbuf));
              strcat(rbuf,"Invalid command\nftpml>");
              nread += 24; 
              write(STDOUT_FILENO, rbuf, strlen(rbuf)); 
              continue; 
            }
	  }

      	  //support for ascii mode~~~
 	  else if(strncmp(rbuf, "ascii", 5) == 0) { //for ascii transport--------------
            if(*(rbuf+5) =='\n') {
              type=1;
	      sprintf(wbuf, "%s", "Change to ASCII mode,enter next command@^____^@\nftpml>");
	      write(STDOUT_FILENO, wbuf, 55);
	      continue;
            }
            else {
              memset(rbuf,0,strlen(rbuf));
              strcat(rbuf,"Invalid command\nftpml>");
              nread += 24; 
              write(STDOUT_FILENO, rbuf, strlen(rbuf)); 
              continue; 
            }
	  }

         /* code here: get - get file from ftp server */
         else if (strncmp(rbuf, "get", 3) == 0){ 
           if(*(rbuf+3) ==' ') {
              char *src,*dest; 
              char *str1,*str2,*str3;
              int iden; /*store state of the third argument*/
              src=(char *)malloc(100);
              dest=(char *)malloc(100); 
              str1=(char *)malloc(100);
              str2=(char *)malloc(100);
              str3=(char *)malloc(100); 

              strtok(rbuf, " ");
              src = strtok(NULL," ");
              if(src == NULL) {
                memset(rbuf,0,strlen(rbuf));
                strcat(rbuf,"Usage: get Source_file_path [Destination_path]\nftpml>");
                nread += 55; 
                write(STDOUT_FILENO, rbuf, strlen(rbuf));
                continue;
              }
              dest = strtok(NULL," ");
              if(dest==NULL) {
                src = strtok(src,"\n"); 
                dest = src;
                iden=off;
              } 
              else { 
                dest = strtok(dest,"\n");
                iden=on; 
              }

              if(passive==on) {
                sprintf(wbuf, "%s", "PASV\n");
                write(sockfd, wbuf, strlen(wbuf));

                if(type==0) //for binary--------------
                sprintf(wbuf,"%s","TYPE I\n");
                if(type==1) {//for ascii--------------
                  sprintf(wbuf,"%s","TYPE A\n");
                  type=0;
                }
                write(sockfd,wbuf,7);

              }

              str1=strcpy(str1,src);
              if(strstr(str1,"/")!=NULL) {
                strcpy(rem_dir,strtok(src,"/"));
                str2=strtok(NULL,"/");
                do {
                  str3=strtok(NULL,"/"); 
                  if(str3==NULL)
                  break;
                  strcat(rem_dir,"/");
                  strcat(rem_dir,str2);
                  strcpy(str2,str3);
                } while(1);
                strcpy(rem_file,str2);
                sprintf(wbuf1, "RETR %s/%s\n", rem_dir,rem_file);
              }
              else {
                sprintf(rem_dir,"%s","."); 
                strcpy(rem_file,src); 
                sprintf(wbuf1, "RETR %s\n",rem_file);
              }

              if(iden==on) {
                strcpy(str1,dest);
                if(strstr(str1,"/")!=NULL) {
                  strcpy(loc_dir,strtok(dest,"/"));
                  str2=strtok(NULL,"/"); 
                  do {
                    str3=strtok(NULL,"/"); 
                    if(str3==NULL)
                    break;
                    strcat(loc_dir,"/");
                    strcat(loc_dir,str2);
                    strcpy(str2,str3);
                  } while(1);
                  strcpy(loc_file,str2);
                }
                else {
                  sprintf(loc_dir,"%s",".");
                  strcpy(loc_file,dest); 
                }
              }

              else {
                strcpy(loc_file,rem_file); 
                sprintf(loc_dir,"%s",".");
              } 

              status = get;
              continue;
           }
           else {
             memset(rbuf,0,strlen(rbuf));
             strcat(rbuf,"Invalid command\nftpml>");
             nread += 24; 
             write(STDOUT_FILENO, rbuf, strlen(rbuf)); 
             continue; 
           }
         }

         // code here: put - put file upto ftp server
         else if(strncmp(rbuf, "put", 3) == 0) {
           if(*(rbuf+3) ==' ') { 
              char *src,*dest; 
              char *str1,*str2,*str3;
              int iden; /*store state of the third argument*/
              src=(char *)malloc(100);
              dest=(char *)malloc(100); 
              str1=(char *)malloc(100);
              str2=(char *)malloc(100);
              str3=(char *)malloc(100); 

              strtok(rbuf, " ");
              dest = strtok(NULL," ");
              if(src == NULL) {
                memset(rbuf,0,strlen(rbuf));
                strcat(rbuf,"Usage: put Source_file_path [Destination_path]\nftpml>");
                nread += 55; 
                write(STDOUT_FILENO, rbuf, strlen(rbuf));
                continue;
              }

              src = strtok(NULL," ");
              if(src==NULL) {
                dest = strtok(dest,"\n"); 
                src = dest;
                iden=off;
              } 
              else { 
                src = strtok(src,"\n");
                iden=on; 
              }

              if(passive==on) {
                sprintf(wbuf, "%s", "PASV\n");
                write(sockfd, wbuf, strlen(wbuf));

                if(type==0) //for binary-----------------
                sprintf(wbuf,"%s","TYPE I\n");
                if(type==1) {//for ascii-----------------
                  sprintf(wbuf,"%s","TYPE A\n");
                  type=0;
                }
                write(sockfd,wbuf,7);

              }

              str1=strcpy(str1,dest);
              if(strstr(str1,"/")!=NULL) {
                strcpy(loc_dir,strtok(dest,"/"));
                str2=strtok(NULL,"/");
                do {
                  str3=strtok(NULL,"/"); 
                  if(str3==NULL)
                  break;
                  strcat(loc_dir,"/");
                  strcat(loc_dir,str2);
                  strcpy(str2,str3);
                } while(1);
                strcpy(loc_file,str2);
              }

              else {
                sprintf(loc_dir,"%s","."); 
                strcpy(loc_file,dest); 
              }

              if(iden==on) {
                strcpy(str1,src);
                if(strstr(str1,"/")!=NULL) {
                  strcpy(rem_dir,strtok(src,"/"));
                  str2=strtok(NULL,"/"); 
                  do {
                    str3=strtok(NULL,"/"); 
                    if(str3==NULL)
                    break;
                    strcat(rem_dir,"/");
                    strcat(rem_dir,str2);
                    strcpy(str2,str3);
                  } while(1);
                  strcpy(rem_file,str2);
                  sprintf(wbuf1,"STOR %s/%s\n",rem_dir,rem_file);
                }

                else {
                  sprintf(rem_dir,"%s",".");
                  strcpy(rem_file,src); 
                  sprintf(wbuf1, "STOR %s\n", rem_file); 
                }

              }

              else {
                strcpy(rem_file,loc_file); 
                sprintf(rem_dir,"%s",".");
                sprintf(wbuf1, "STOR %s\n", rem_file); 
              } 

              status = put;
              continue;
           }
           else {
             memset(rbuf,0,strlen(rbuf));
             strcat(rbuf,"Invalid command\nftpml>");
             nread += 24; 
             write(STDOUT_FILENO, rbuf, strlen(rbuf)); 
             continue; 
           }
         }

         //other conditions
         else {
              memset(rbuf,0,strlen(rbuf));
              strcat(rbuf,"Invalid command\nftpml>");
              nread += 24; 
              write(STDOUT_FILENO, rbuf, strlen(rbuf)); 
              continue; 
         }
        }
      }

      /* data to read from socket */
      if(FD_ISSET(sockfd, &rset)) { 
        memset(rbuf,0,strlen(rbuf));
        if((nread = recv(sockfd, rbuf, MAXBUF, 0)) < 0)
        printf("recv error\n");
        else if(nread == 0)
        break;


        /* set replycode and wait for user's input */
        if(strncmp(rbuf, "200", 3)==0) {
          char *pasv;
          pasv = (char *) malloc(5);
          if(type==on) {
            strcat(rbuf, "ftpml>");
            nread += 7;
            type=off; 
          }

          else if(passive==off) {
            write(sockfd, wbuf1, strlen(wbuf1));
            fd=accept(fd,NULL,NULL); 
            if(fd<0) {
              printf("accept() failed");
              perror("");
            } 
          }

          else {
            strcat(rbuf, "ftpml>");
            nread += 7;
          }
        }
 
        if(strncmp(rbuf, "220", 3)==0) {
          strcat(rbuf, "your name: ");
          nread += 12;
          replycode = USERNAME;
        }

        if(strncmp(rbuf, "530", 3)==0) {
          strcat(rbuf, "your name: ");
          nread += 12;
          replycode = USERNAME;
          tcsetattr(0,TCSANOW,&stored_settings);
        }

        // code here: handle other response coming from server
        if(strncmp(rbuf, "331", 3)==0) {
          strcat(rbuf, "your password: ");
          nread += 20;
          replycode = PASSWORD;
          tcgetattr(0,&stored_settings);
          new_settings = stored_settings;
          new_settings.c_lflag &=(~ECHO);
          tcsetattr(0,TCSANOW,&new_settings);
        }

        if(strncmp(rbuf, "230", 3)==0) {
          strcat(rbuf, "ftpml>");
          nread += 10;
          replycode = LOGIN;
          tcsetattr(0,TCSANOW,&stored_settings);
        }

        if(strncmp(rbuf, "421", 3)==0) {
          strcat(rbuf, "Sorry!You cannot login because of some eroors.\nPlease release your connection and await or reset your IP address.ftpml>");
          nread += 206;
          replycode = LOGIN;
        }

        if(strncmp(rbuf, "425", 3)==0) {
          strcat(rbuf, "ftpml>");
          nread += 7;
        } 

        /* open data connection*/
        if(strncmp(rbuf, "227", 3) == 0) {
          printf("%s",rbuf); 
          strtosrv(rbuf, host, port);
          printf("host: %s port : %s\nconnecting...\n",host,port);
          memset(rbuf,0,strlen(rbuf));
          fd = cliopen(host, port);
          write(sockfd, wbuf1, strlen(wbuf1));
          nwrite = 0;
        }

        if(strncmp(rbuf, "226", 3) == 0) {
          if(status==trans) {
          //  strcat(rbuf, "ftpml>");
          //  nread += 10; 
            close(fd); 
          }

          if(passive==on) {
            strcat(rbuf, "ftpml>");
            nread += 10; 
            close(fd); 
          }

          replycode = CLOSEDATA;
        }

        if(strncmp(rbuf, "257", 3) == 0) {
          strcat(rbuf, "ftpml>");
          nread += 10; 
          close(fd); 
          replycode = PATHNAME;
        }

        if(strncmp(rbuf, "250", 3) == 0) {
          strcat(rbuf, "ftpml>");
          nread += 10; 
          replycode = ACTIONOK;
        }

        if(strncmp(rbuf, "350", 3) == 0) {
          write(sockfd, wbuf1, strlen(wbuf1));
          nwrite = 0;
        }

        if(strncmp(rbuf, "500", 3) == 0){
          strcat(rbuf, "ftpml>");
          nread += 10; 
          replycode = LOGIN; 
        }

        if(strncmp(rbuf, "550", 3) == 0) {
          strcat(rbuf, "ftpml>");
          nread += 10; 
          replycode = LOGIN; 
        } 

        //code here: receive data from server and handle it
        write(STDOUT_FILENO, rbuf, strlen(rbuf));

        /* start data transfer */
        if(strncmp(rbuf, "150", 3) == 0) {
          if(status==get) {
            file_size=get_file_size(rbuf); //for progress_bar
            sprintf(wbuf,"%s/%s",loc_dir,loc_file);
            ftp_get(fd,wbuf);
            status=trans;
          }

          if(status==put) {
            sprintf(wbuf,"%s/%s",loc_dir,loc_file);
            ftp_put(fd,wbuf);
            status=trans; 
          }
 
          if(status==ls) {
            ftp_list(fd);
            if((strstr(rbuf,"226"))!=NULL) {
              sprintf(wbuf, "%s", "PWD\n");
              write(sockfd, wbuf, 4);
            } 
          }
        }

      }
   }

   printf("Thanks for your use. You have disconnected from the FTP server.\n");
   if(close(sockfd) < 0)
   printf("close error\n");
}

/* Read and write as data transfer connection */
void
ftp_list(int sockfd)
{

    int nread;
    int i,j=0,k=0,num=0,file=0;
    char str[MAXBUF],buf[MAXBUF*10]={'\0'};
    char *some;

    memset(rbuf1,0,strlen(rbuf1));
    for ( ; ; ) {

       /* data to read from socket */
       if ( (nread = recv(sockfd, rbuf1, MAXBUF, 0)) < 0)
       printf("recv error\n");
       else if (nread == 0)
       break;

       strcat(buf,rbuf1); 
       strcat(buf,"\0");
    }

    some=strtok(rbuf1,"\n");
    printf("%s\n",some);
    fflush(stdout);
    some=strtok(NULL,"\n");
    printf("%s\n",some);
    fflush(stdout);

    for(i=119;i<strlen(buf);i++) {
       if(buf[i]=='\n') {
         for(j=i-num;j<i;j++) {
	    str[j-i+num]=buf[j];
         }
	 str[j-i+num]='\0';
	 num=0;
	 for(k=0;k<strlen(str);k++) {
	    if('.'==str[k]) {
	      file=1;
	      if(str[k+1]=='z') printf("\033[01;36m%s\033[0m",str); 
	      else if(str[k+1]=='p') printf("\033[01;35m%s\033[0m",str); 
	      else if(str[k+1]=='m') printf("\033[01;34m%s\033[0m",str); 
	      else if(str[k+1]=='d') printf("\033[01;33m%s\033[0m",str); 
              else if(str[k+1]=='t') printf("\033[01;32m%s\033[0m",str);
              else if(str[k+1]=='g') printf("\033[01;31m%s\033[0m",str);
              else if(str[k+1]=='c') printf("\033[01;97m%s\033[0m",str);
              else if(str[k+1]=='f') printf("\033[01;96m%s\033[0m",str);
              else if(str[k+1]=='r') printf("\033[01;95m%s\033[0m",str);
              else if(str[k+1]=='e') printf("\033[01;94m%s\033[0m",str);
              else if(str[k+1]=='a') printf("\033[01;93m%s\033[0m",str);
	      else printf("\033[01;92m%s\033[0m",str); 
	    }
	 }
	 if(file==0) {
	   printf("\033[01;31m%s\033[0m",str);
	 }
	 file=0;
       }		
       num++;
    }
    printf("\n"); 
    fflush(stdout);

    if (close(sockfd) < 0)
    printf("close error\n");
}

/* download file from ftp server */
int 
ftp_get(int sck, char *pDownloadFileName_s)
{

      // code here
    int nread,fd,T1,T2;
    int size=0;
    int i=1;
    double percent,speed;
    double time=0.0;
    char *progress_bar;
    progress_bar=(char *)malloc(MAXBUF*sizeof(char));
    memset(progress_bar,0,strlen(progress_bar));
    memset(rbuf1,0,strlen(rbuf1));

    if((fd=creat(pDownloadFileName_s,FILE_MODE))<0) { //create a same file in local place
      printf("--T_T--file creat error\n");
      exit(1);
    }

    T1=clock();

    for ( ; ; ) {

      /* data to read from socket */
      if ( (nread = recv(sck, rbuf1, MAXBUF, 0)) < 0)
      printf("--T_T--recv error2\n");
      else if (nread == 0)
      break;

      if (write(fd, rbuf1, nread) != nread)
      printf("--T_T--write error to the file we create\n");

      size=size+nread;
      percent=(double)size/file_size;
      if(percent>0.05*i){
	strcat(progress_bar,">>");
	write(STDOUT_FILENO, progress_bar, 2);
	i++;
      }
    }
    printf(" @^____^@finish\n");

    T2=clock();
    time=(double)(T2-T1)/CLOCKS_PER_SEC;
    speed=(double)size/1024/time;
    printf("Send File:%s\nTotal Flow:%.2fKB\nTime:%.2fs\nSpeed:%.2fkb/s\n",pDownloadFileName_s,(double)size/1024,time,speed);

    if (close(sck) < 0)
    printf("--T_T--close error\n");
}

/* upload file to ftp server */
int 
ftp_put (int sck, char *pUploadFileName_s)
{
   // code here
    int nsend,fd;
    int size=0;
    int i=1;
    double percent,speed;
    double time=0.0;
    clock_t T1,T2;
    char *progress_bar;
    progress_bar=(char *)malloc(MAXBUF*sizeof(char));
    memset(progress_bar,0,strlen(progress_bar));
    memset(rbuf1,0,strlen(rbuf1));
    file_size=0;

    if((fd=open(pUploadFileName_s,FILE_MODE))<0) { //open the file we want to put, if it is not exist, then we'll create one
      printf("--T_T--file open error\n");
      exit(1);
    }
    
    T1=clock();
    file_size=lseek(fd,0,SEEK_END);
    lseek(fd,0,SEEK_SET);

    for ( ; ; ) {
      if ( (nsend = read(fd, rbuf1, MAXBUF) ) < 0)
      printf("--T_T--read error from file\n"); 
      else if (nsend == 0)
      break; 


      /* data to send send socket */
      if ( (send(sck, rbuf1, nsend, 0)) != nsend)
      printf("--T_T--send error\n");
      
      size=size+nsend;
      percent=(double)size/file_size;
      if(percent>0.05*i) {
	strcat(progress_bar,">>");
	write(STDOUT_FILENO, progress_bar, 2);
	i++;
      }
    }
    printf(" @^____^@finish\n");
		
    T2=clock();
    time=(double)(T2-T1)/CLOCKS_PER_SEC;
    speed=(double)(file_size)/1024/time;
    printf("Send File:%s\nTotal Flow:%.2fKB\nTime:%.2fs\nSpeed:%.2fkb/s\n",pUploadFileName_s,(double)file_size/1024,time,speed);

    if (close(sck) < 0)
    printf("--T_T--close error\n");
}

/* introduce operation to user */
void help()
{
    printf("\n");
    printf("\033[1;97m%s\033[0m",">>help   --List the commmand infomation.\n");	
    printf("\033[1;97m%s\033[0m",">>cd     --Change server's working directory.\n");		
    printf("\033[1;97m%s\033[0m",">>ls     --List contents of server's directory.\n");	
    printf("\033[1;97m%s\033[0m",">>pwd    --Print working directory on server.\n");	
    printf("\033[1;97m%s\033[0m",">>mkdir  --Make a new directory\n");	
    printf("\033[1;97m%s\033[0m",">>rmdir  --Remove a directory.\n");		
    printf("\033[1;97m%s\033[0m",">>get    --Receive file from server.\n");	
    printf("\033[1;97m%s\033[0m",">>put    --Send file to server.\n");
    printf("\033[1;97m%s\033[0m",">>delete --Delete file in server.\n");
    printf("\033[1;97m%s\033[0m",">>rename --Rename file.\n");	
    printf("\033[1;97m%s\033[0m",">>binary --Put or get file in binary mode.\n");
    printf("\033[1;97m%s\033[0m",">>ascii  --Put or get file in ascii mode.\n");
    printf("\033[1;97m%s\033[0m",">>quit   --Quit from ftpclient.\n");
    printf("\033[1;97m%s\033[0m",">>exit   --The same function with quit.\n");	
    puts ("");
}
