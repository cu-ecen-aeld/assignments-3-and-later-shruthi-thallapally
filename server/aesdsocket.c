/* references:https://blog.taborkelly.net/programming/c/2016/01/09/sys-queue-example.html
	https://man.freebsd.org/cgi/man.cgi?query=SLIST_ENTRY
*/

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
#include <time.h>
#include <pthread.h>
#include <sys/queue.h>


#pragma GCC diagnostic warning "-Wunused-variable"

#define PORT_NUM (9000)
#define BACKLOG (10)
#define BUFFER_SIZE (1024)

typedef struct thread_node
{
	pthread_t thread_id;
	SLIST_ENTRY(thread_node) entry;
}thread_node;

typedef struct threadArgs
{
	int fd;
	int data_fd;
	struct sockaddr_storage addr;
}threadArgs;


pthread_mutex_t thread_list_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
SLIST_HEAD(thread_connections, thread_node) thread_head = SLIST_HEAD_INITIALIZER(thread_head);

int sockfd,fd;
struct addrinfo *res;
bool caught_sig=false;

//FILE *temp_file=NULL;

void *time_stamp(void *ptr)
{
	while(!caught_sig)
	{
		char timestamp[100];
		time_t now=time(NULL);
		struct tm *time_info=localtime(&now);
		strftime(timestamp,sizeof(timestamp),"timestamp:%Y-%m-%d %H:%M:%S\n", time_info);
		syslog(LOG_INFO,"10s has elapsed, time is %s",timestamp);
		pthread_mutex_lock(&file_mutex);
        	write(((threadArgs *)ptr)->data_fd, timestamp, strlen(timestamp));
        	pthread_mutex_unlock(&file_mutex);

        	sleep(10); // Sleep for 10 seconds before appending the next timestamp
	}
	
			
}
// Function to add a new thread node to the list
void add_new_thread_node(pthread_t thread_id)
{
    thread_node *new_node = (thread_node *)malloc(sizeof(thread_node));
    if (new_node == NULL)
    {
        syslog(LOG_ERR, "Failed to allocate memory for thread list node");
        return;
    }
    new_node->thread_id = thread_id;
  

    pthread_mutex_lock(&thread_list_mutex);
    SLIST_INSERT_HEAD(&thread_head,new_node,entry);
    pthread_mutex_unlock(&thread_list_mutex);
}


// Function to wait for all threads to complete
void wait_for_all_threads_to_join()
{
    thread_node *next_node;
    pthread_mutex_lock(&thread_list_mutex);
    thread_node *current = SLIST_FIRST(&thread_head);
    while (current != NULL)
    {
    	next_node = SLIST_NEXT(current,entry);
        if(pthread_join(current->thread_id, NULL)==0)
        {
        	syslog(LOG_INFO,"thread %ld joined successfully",current->thread_id);
        	SLIST_REMOVE(&thread_head,current,thread_node,entry);
        	free(current);
        }
        else
        {
        	syslog(LOG_INFO,"thread %ld was not able to join:%s",current->thread_id,strerror(errno));
        }
        current=next_node;
    }
    
    pthread_mutex_unlock(&thread_list_mutex);
}
bool daemon_mode()
{
   int file;
   pid_t pid=fork();
   bool status=false;
  
   
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

   if (chdir("/") == -1)
	{
		syslog(LOG_ERR, "Failed to change directory");
		return status;
	}

	if ((file = open("/dev/null", O_RDWR)) == -1)
	{
		syslog(LOG_ERR, "Failed to open /dev/null");
		return status;
	}

	if (dup2(file, STDIN_FILENO) == -1 || dup2(file, STDOUT_FILENO) == -1 || dup2(file, STDERR_FILENO) == -1)
	{
		syslog(LOG_ERR, "Failed to redirect standard I/O");
		close(file);
		return status;
	}

	close(file);
	status = true;
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
	
	pthread_mutex_lock(&file_mutex);
	while((bytes_read=read(data_fd,send_buff,sizeof(send_buff)-1))>0)
	{
		send_buff[bytes_read]='\0';
		if(send(fd,send_buff,bytes_read,0)==-1)
		{
			syslog(LOG_ERR,"sending data to client failed");
			break;
		}
	}
	pthread_mutex_unlock(&file_mutex);
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
	
	pthread_mutex_lock(&file_mutex);
	if(write(data_fd,buff,total_rx)!=-1)
	{
		pthread_mutex_unlock(&file_mutex); // Unlock before syncing the file
		fdatasync(data_fd);
	}
	else
	{
		syslog(LOG_ERR,"writing received data into file failed");
		free(buff);
		pthread_mutex_unlock(&file_mutex);
		
		return -1;
	}
	pthread_mutex_unlock(&file_mutex);
	free(buff);
	return 0;
	
}

void *thread_function(void *arg)
{
    threadArgs *thread_args = (threadArgs *)arg;
    int fd = thread_args->fd;
    int data_fd = thread_args->data_fd;
    struct sockaddr_storage client_addr = thread_args->addr;
    char client_ip[INET_ADDRSTRLEN];
   
    // Convert client address to string
    if (client_addr.ss_family == AF_INET)
    {
        struct sockaddr_in *addr_in = (struct sockaddr_in *)&client_addr;
        inet_ntop(AF_INET, &addr_in->sin_addr, client_ip, sizeof(client_ip));
    }
    else if (client_addr.ss_family == AF_INET6)
    {
        struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&client_addr;
        inet_ntop(AF_INET6, &addr_in6->sin6_addr, client_ip, sizeof(client_ip));
    }
    else
    {
        strcpy(client_ip, "Unknown");
    }

    syslog(LOG_INFO, "Accepted connection from %s", client_ip);

    // Receive data from client and write it to file
   if(rx_socket_data(fd,data_fd)==0)
   {
   	send_to_client(fd,data_fd);
   }
    if(close(fd)==0)
    {
    	syslog(LOG_INFO,"Connection closed successfully from %s",client_ip);
    }
    else
    {
    	syslog(LOG_ERR,"Connection closing failed from %s",client_ip);
    }
    free(thread_args); // Free the memory allocated for thread arguments
  //  pthread_exit(NULL);
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
    struct sockaddr_storage client_addr;
    int flag,data_fd;

    
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
    addr_len=sizeof(client_addr);
    
    //create timestamp thread
    pthread_t timestamp_thread;
    threadArgs *timer_thread_args=(threadArgs *)malloc(sizeof(threadArgs));
    if(timer_thread_args==NULL)
    {
    	syslog(LOG_ERR,"memory allocation for timer thread arguments failed");
    }
    else
    {
    	timer_thread_args->data_fd=data_fd;
    	if(pthread_create(&timestamp_thread,NULL,time_stamp,(void *)timer_thread_args) !=0)
    	{
    		syslog(LOG_ERR,"failed to create timer thread");
    		free(timer_thread_args);
    	}
    }
    while (!caught_sig)
    {
        
        fd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);
        if (fd == -1)
        {
            syslog(LOG_ERR, "accept() failed: %s", strerror(errno));
            continue;
        }
    	pthread_t thread_id;
    	threadArgs *thread_args=(threadArgs *)malloc(sizeof(threadArgs));
    	if(thread_args==NULL)
    	{
    		syslog(LOG_ERR,"memory allocation for thread arguments failed");
    		close(data_fd);
    		continue;
    	}
    	thread_args->fd=fd;
    	thread_args->data_fd=data_fd;
    	thread_args->addr=client_addr;
    	if(pthread_create(&thread_id,NULL,thread_function,(void *)thread_args) !=0)
    	{
    		syslog(LOG_ERR,"failed to create timer thread");
    		free(thread_args);
    		close(data_fd);
    		continue;
    	}
    	add_new_thread_node(thread_id);
        
     }
     wait_for_all_threads_to_join();
     freeaddrinfo(res);
     close(data_fd);
     closelog();
}		

    
    
      
