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



#define PORT_NUM (9000)
#define BACKLOG (10)
#define BUFFER_SIZE (1024)


int sockfd,fd;
struct addrinfo *res;
bool caught_sig=false;
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
   
   if (setsid() == -1)
    {
        syslog(LOG_ERR, "Failed to create a new session");
        return status;
    }

   if((ret_childdir =chdir("/"))==-1)
   {
        syslog(LOG_ERR,"failed to change directory");
        return status;
   }
   
   if((file=open("/dev/null",O_RDWR))==-1)
   {
        syslog(LOG_ERR,"failed to open /dev/null");
        return status;
   }
    
   if((ret_stdin=dup2(file,STDIN_FILENO))==-1)
   {
   	syslog(LOG_ERR,"failed to redirect stdin");
   	close(file);
   	return status;
   }
   
   if((ret_stdout=dup2(file,STDOUT_FILENO))==-1)
   {
   	syslog(LOG_ERR,"failed to redirect stdout");
   	close(file);
   	return status;
   } 
   
   if((ret_stderr=dup2(file,STDERR_FILENO))==-1)
   {
   	syslog(LOG_ERR,"failed to redirect stderr");
   	close(file);
   	return status;
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
	if(signal==SIGINT || signal == SIGTERM)
	{
		syslog(LOG_INFO,"caught signal, exiting gracefully\n");
	}
	caught_sig=true;
}

void init_sigaction()
{
    struct sigaction action;
     
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0; 
    if (sigaction(SIGTERM, &action, NULL) != 0)
    {
        syslog(LOG_ERR, "SIGTERM failed");
    }

    if (sigaction(SIGINT, &action, NULL))
    {
        syslog(LOG_ERR, "SIGINT failed");
    }

}
int send_to_client(int fd,int data_fd)
{
	char *send_buff;
	send_buff = (char *)malloc(BUFFER_SIZE);
	size_t bytes_read;
	lseek(data_fd,0,SEEK_SET);
	if(send_buff==NULL)
	{
		syslog(LOG_ERR,"malloc failed while sending to client");
		return -1;
	}	
	
	while((bytes_read=read(data_fd,send_buff,sizeof(send_buff)-1))>0)
	{
	send_buff[bytes_read]='\0';
	if(send(fd,send_buff,bytes_read,0)==-1)
	{
		syslog(LOG_ERR,"sending data to client failed");
		break;
	}
	}
	free(send_buff);
	return 0;
	
}
int rx_socket_data(int fd,int data_fd)
{
	size_t mul_factor=1;
	size_t buff_size = BUFFER_SIZE;
	size_t total_rx=0;
	char *buff=NULL;
	
	buff=(char *)malloc(buff_size*sizeof(char));
	if(buff==NULL)
	{
		syslog(LOG_ERR,"malloc failed");
		return -1;
	}
	while(1)
	{
		ssize_t rx_bytes=recv(fd,buff+total_rx,buff_size-total_rx-1,0);
		if(rx_bytes<=0)
		{
			break;
		}
		total_rx+=rx_bytes;
		buff[total_rx]='\0';
		
		if(strchr(buff,'\n')!=NULL)
		{
			break;
		}
		
		mul_factor<<=1;
		size_t new_buff_size=mul_factor*BUFFER_SIZE;
		char * new_buff=(char *)realloc(buff,new_buff_size);
		if(new_buff==NULL)
		{
			syslog(LOG_ERR,"realloc failed");
			free(buff);
			return -1;
		}
		buff=new_buff;
		buff_size=new_buff_size;
	}
	
	if(write(data_fd,buff,total_rx)!=-1)
	{
		fdatasync(data_fd);
	}
	else
	{
		syslog(LOG_ERR,"writing received data into file failed");
		free(buff);
		return -1;
	}
	
	free(buff);
	return 0;
	
}


int main(int argc, char **argv)
{
    openlog("aesdsocket_log",LOG_PID|LOG_CONS,LOG_USER);
    bool in_daemon_mode = false;
    
    if((argc >=2) && strcmp(argv[1],"-d") == 0)
    {
    	in_daemon_mode = true;
    }
          
    socklen_t addr_len;
    struct addrinfo hints;
    struct sockaddr_storage addr;
    int flag,data_fd;
    int yes=1;
    
    memset(&hints,0,sizeof(hints)); //empty the struct
    hints.ai_flags=AI_PASSIVE;
    hints.ai_family=AF_INET;
    hints.ai_socktype=SOCK_STREAM;
    
    if((flag=getaddrinfo(NULL,"9000",&hints,&res))!=0)
    {
    	syslog(LOG_ERR,"failed to get socket address");
    	closelog();
    	exit(1);
    }
    
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
    {
        syslog(LOG_ERR, "socket() failed");
        closelog();
        freeaddrinfo(res);
        exit(1);
    }
    
     // Set socket options to allow reuse of address
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) != 0) {
        syslog(LOG_ERR, "setsockopt() failed");
        closelog();
        freeaddrinfo(res);
        exit(1);
    }
    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1) 
    {
        syslog(LOG_ERR, "bind() failed");
        closelog();
        freeaddrinfo(res);
        exit(1);
    }
    
    if(in_daemon_mode)
    {
    	bool daemon_status = daemon_mode();
    	if(daemon_status ==false)
    	{
    	    syslog(LOG_ERR,"daemon creation failed");
    	    closelog();
            freeaddrinfo(res);
    	    exit(1);
    	 }
    }
    
    if (listen(sockfd, BACKLOG) == -1)
    {
        syslog(LOG_ERR, "Listen failed");
        closelog();
        freeaddrinfo(res);
        exit(1);
    }
    
    data_fd = open("/var/tmp/aesdsocketdata",  O_RDWR |O_CREAT | O_TRUNC,0666);
    if(data_fd ==-1)
    {
    	syslog(LOG_ERR,"/var/tmp/aesdsocketdata file failed to open");
    	closelog();
        freeaddrinfo(res);
    	exit(1);
    }
    
    init_sigaction();
    addr_len=sizeof(addr);
    while (!caught_sig)
    {
        
        fd = accept(sockfd, (struct sockaddr *)&addr, &addr_len);
        if (fd == -1)
        {
            syslog(LOG_ERR, "accept() failed: %s", strerror(errno));
            continue;
        }
    	inet_ntop(addr.ss_family, &(((struct sockaddr_in*)&addr)->sin_addr), client_ip, sizeof(client_ip));
        syslog(LOG_DEBUG, "accept() successful from %s", client_ip);
        
        if(rx_socket_data(fd,data_fd)==0)
        {
        	send_to_client(fd,data_fd);
        }
        if(close(fd)==-1)
        {
        	syslog(LOG_ERR,"closing connection failed");
        }
        
     }
     
     freeaddrinfo(res);
     close(data_fd);
     closelog();
}		

    
    
      
