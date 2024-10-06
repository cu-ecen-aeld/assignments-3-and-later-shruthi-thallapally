#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define _POSIX_C_SOURCE 200112L  // Enable 2001 version of POSIX features

#define PORT_NUM (9000)
#define BACKLOG (10)
#define BUFFER_SIZE (1024)


int sockfd,fd;
struct addrinfo *res;
volatile sig_atomic_t caught_sig;
char client_ip[INET_ADDRSTRLEN];
FILE *temp_file=NULL;

bool daemon_mode()
{
   int file;
   pid_t pid=fork();
   bool status=false;
   int ret_stdin,ret_stdout,ret_stderr,ret_childdir;
   
   if(pid==-1)
   {
      syslog(LOG_ERR,"Daemon Fork failed");
      return status;
   }
   
   if(pid > 0)
   {
      exit(0);  //exiting parent process
   }
   
   if((ret_childdir =chdir("/"))==-1)
   {
        syslog(LOG_ERR,"failed to change directory");
   }
   
   if((file=open("/dev/null",O_RDWR))==-1)
   {
        syslog(LOG_ERR,"failed to open /dev/null");
        return status;
   }
    
   if((ret_stdin=dup2(file,STDIN_FILENO))==-1)
   {
   	syslog(LOG_ERR,"failed to redirect stdin");
   }
   
   if((ret_stdout=dup2(file,STDOUT_FILENO))==-1)
   {
   	syslog(LOG_ERR,"failed to redirect stdout");
   } 
   
   if((ret_stderr=dup2(file,STDERR_FILENO))==-1)
   {
   	syslog(LOG_ERR,"failed to redirect stderr");
   }
   
    if ((ret_childdir != -1) && (ret_stdin != -1) && (ret_stdout != -1) && (ret_stderr != -1))
    {
       status = true;
    }
    close(file);
    return status;
}
   
static void signal_handler(int signal)
{
	caught_sig=signal;
}

void mem_clean()
{

      if(sockfd !=-1)
      {
      	shutdown(sockfd,SHUT_RDWR);
      	close(sockfd);
      }	
      
      if(res != NULL)
      {
      	freeaddrinfo(res);
      }
      
      if(temp_file != NULL)
      {
      	fclose(temp_file);
      	temp_file = NULL;
      	remove("/var/tmp/aesdsocketdata");
      }	
      	
      if(fd !=-1)
      {
      	shutdown(fd,SHUT_RDWR);
      	close(fd);
      	syslog(LOG_DEBUG,"closed connection from %s",client_ip);
      }	
      	
      closelog();
}	

int main(int argc, char **argv)
{
    openlog("aesdsocket_log",LOG_PID|LOG_CONS,LOG_USER);
    bool in_daemon_mode = false;
    
    if(strcmp(argv[1],"-d") && (argc >=2) == 0)
    {
    	in_daemon_mode = true;
    }
          
    socklen_t addr_len;
    struct addrinfo hints;
    struct sockaddr_storage addr;
    int flag;
    
    memset(&hints,0,sizeof(hints)); //empty the struct
    hints.ai_flags=AI_PASSIVE;
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_STREAM;
    
    if((flag=getaddrinfo(NULL,"9000",&hints,&res))!=0)
    {
    	syslog(LOG_ERR,"failed to get socket address");
    	mem_clean();
    	exit(1);
    }
    
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
    {
        syslog(LOG_ERR, "socket() failed");
        mem_clean();
        exit(1);
    }
    
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) 
    {
        syslog(LOG_ERR, "bind() failed");
        mem_clean();
        exit(1);
    }
    
    if(in_daemon_mode)
    {
    	bool daemon_status = daemon_mode();
    	if(daemon_status ==false)
    	{
    	    mem_clean();
    	    exit(1);
    	 }
    }
    
    if (listen(sockfd, BACKLOG) == -1)
    {
        syslog(LOG_ERR, "Listen failed");
        mem_clean();
        exit(1);
    }
    
    temp_file = fopen("/var/tmp/aesdsocketdata", "w+");
    if(temp_file ==NULL)
    {
    	syslog(LOG_ERR,"/var/tmp/aesdsocketdata file failed to open");
    	mem_clean();
    	exit(1);
    }
    
    struct sigaction action;
    memset(&action,0,sizeof(struct sigaction));
    action.sa_handler = signal_handler;
    if (sigaction(SIGTERM, &action, NULL) != 0)
    {
        syslog(LOG_ERR, "SIGTERM failed");
    }

    if (sigaction(SIGINT, &action, NULL))
    {
        syslog(LOG_ERR, "SIGINT failed");
    }

    while (!caught_sig)
    {
        addr_len = sizeof addr;
        fd = accept(sockfd, (struct sockaddr *)&addr, &addr_len);
        if (fd == -1)
        {
            syslog(LOG_ERR, "accept() failed: %s", strerror(errno));
            mem_clean();
            exit(1);
        }
    	inet_ntop(addr.ss_family, &(((struct sockaddr_in*)&addr)->sin_addr), client_ip, sizeof(client_ip));
        syslog(LOG_DEBUG, "accept() successful from %s", client_ip);
        
        size_t rx_buff_size = BUFFER_SIZE;
        char *buffer = (char *)malloc(rx_buff_size);
        memset(buffer,0,BUFFER_SIZE);
        if(buffer==NULL)
        {
        	syslog(LOG_ERR,"malloc() failed for rx buffer");
        	close(fd);
        	continue;
        }
        
        size_t rx=0, size=0;
        int len;
        char *end_packet=NULL;
        
        do
        {
        	static int count=0;
        	
        	if((count!=0)&&(rx+1>=rx_buff_size))
        	{
        		rx_buff_size +=rx_buff_size;
        		char *resized_buff=(char *)realloc(buffer,rx_buff_size);
        		if(resized_buff==NULL)
        		{
        			syslog(LOG_ERR,"realloc() failed");
        			free(buffer);
        			close(fd);
        			continue;
        		}
        		memset(resized_buff+rx,0,rx_buff_size-rx);
        		buffer=resized_buff;
        	}
        	len=recv(fd,buffer+rx,rx_buff_size-rx-1,0);
        	if(len==-1)
        	{
        		syslog(LOG_ERR,"recv() failed");
        		free(buffer);
        		close(fd);
        		continue;
        	}
        	rx +=len;
        	end_packet=strchr(buffer,'\n');
        	count++;
        }while((len>0) && (end_packet==NULL));
        
        if(end_packet !=NULL)
        {
        	size=end_packet-buffer+1;
        	buffer[size]='\0';
        	fwrite(buffer,sizeof(char),size,temp_file);
        	fflush(temp_file);
        }
        else
        {
        	syslog(LOG_ERR,"no newline received\n");
        }
        
        free(buffer);
        
        fseek(temp_file,0,SEEK_SET);
        size_t tx_buff_size=size;
        char *tx_buff=(char *)malloc(tx_buff_size);
        if(tx_buff==NULL)
        {
        	syslog(LOG_ERR,"malloc() failed for tx");
        	close(fd);
        	continue;
        }
        
        while(fgets(tx_buff,tx_buff_size,temp_file)!=NULL)
        {
        	if(send(fd,tx_buff,strlen(tx_buff),0)==-1)
        	{
        		syslog(LOG_ERR,"send() failed");
        		break;
        	}
        }
        
        free(tx_buff);
        close(fd);
        syslog(LOG_DEBUG,"closed connection from client");
     }
     
     mem_clean();
     closelog();
}		

    
    
      

