/***********************************************************************
 * @file      		aesdsocket.c
 * @version   		0.1
 * @brief		
 *
 * @author    		Amey More, Amey.More@Colorado.edu
 * @date      		Oct 4, 2023
 *
 * @institution 	University of Colorado Boulder (UCB)
 * @course      	ECEN 5713: Advanced Embedded Software Development
 * @instructor  	Dan Walkes
 *
 * @assignment 	Assignment 5
 * @due        	Oct 08, 2023 at 11:59 PM
 *
 * @references
 * https://www.gnu.org/software/libc/manual/html_node/
 * Termination-in-Handler.html
 *
 * 
 ************************************************************************/
#include "aesdsocket.h"

volatile sig_atomic_t cleanup = 0;

// For socket commands
command_status_t command_status;
bool deamon_mode = false;

// Outout data file
char *data_file = "/var/tmp/aesdsocketdata";
int data_file_fd;

// Server & Client Socket
int socket_fd;
int accept_fd;

// prototypes
void global_clean_up();
void socket_application();

// https://www.gnu.org/software/libc/manual/html_node/
// Termination-in-Handler.html
void handle_termination(int signo)
{
	syslog(LOG_INFO,"Caught signal, exiting\n");
	
	
	/* 
	* Since this handler is established for more than one kind of signal, 
	* it might still get invoked recursively by delivery of some other 
	* kind of signal. Use a static variable to keep track of that.
	*/
	if (cleanup)
		raise (signo);
	cleanup = 1;
	
	/*
	* Clean-up actions
	*/
	global_clean_up();
	
	

	/* 
	* Now reraise the signal.  We reactivate the signal’s
	* default handling, which is to terminate the process.
	* We could just call exit or abort,
	* but reraising the signal sets the return status
	* from the process correctly.
	*/
	signal(signo, SIG_DFL);
	raise(signo);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void global_clean_up()
{
	int ret;
	
	// Close data file
	ret = close(data_file_fd);
	if(ret == RET_ERROR)
	{
		syslog(LOG_ERR,"File close failed\n");
		command_status.success = false;
	}
	
	// delete data file
	ret = unlink(data_file);
	if(ret == RET_ERROR)
	{
		syslog(LOG_ERR,"File delete failed\n");
	}
	
	// close socket
	close(socket_fd);
	
	// close client
	close(accept_fd);
	
	// Close syslog
	syslog(LOG_INFO,"AESD Socket application end\n");
	closelog();
}

// Application entry point
int main(int argc, char *argv[])
{
	int opt;
	
	while((opt = getopt(argc, argv, "d")) != -1)  
	{
		switch(opt)  
        	{
        		case 'd':
	        		deamon_mode = true;
	        		break; 
        	}
	}
	
	socket_application();
	
	// return -1 if any command in 
	// socket_application fails
	if(command_status.success)
	{
		return (0);
	}
	else
	{
		return (-1);
	}
}

void socket_application()
{
	// local variables
	int ret;
	
	// receive bytes
	ssize_t recv_bytes = 0;
	char buf[BUF_LEN];
	
	// output file
	int file_flags;
	mode_t file_mode;
	
	// send bytes
	ssize_t send_bytes = 0;
	char read_buf[BUF_LEN];
	ssize_t bytes_read;
	
    	char s[INET6_ADDRSTRLEN];
    
	int yes=1;
	
	// structure for getaddrinfo()
	/*
	struct addrinfo {
               int              ai_flags;
               int              ai_family;
               int              ai_socktype;
               int              ai_protocol;
               socklen_t        ai_addrlen;
               struct sockaddr *ai_addr;
               char            *ai_canonname;
               struct addrinfo *ai_next;
           };
	*/
	struct addrinfo hints;
	struct addrinfo *result;
	struct sockaddr_storage client_addr;
	socklen_t client_addrlen = sizeof(struct sockaddr_storage);
	
	// to create logs from application
	openlog(NULL,0,LOG_USER);
	syslog(LOG_INFO,"AESD Socket application started");
	if(deamon_mode)
		syslog(LOG_INFO,"Started as a deamon");
	DEBUG_LOG("AESD Socket start\n");
	
	// signal handler for SIGINT and SIGTERM
	signal(SIGINT, handle_termination);
	signal(SIGTERM, handle_termination);
    
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;	/* IPv4 */
	hints.ai_socktype = SOCK_STREAM; /* stream socket */
	hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
	hints.ai_protocol = 0;          /* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	
	command_status.success = true;
	
	// open data file
	file_flags = (O_RDWR | O_CREAT | O_APPEND);
	file_mode = (S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
	data_file_fd = open(data_file, file_flags, file_mode);
	if(data_file_fd == RET_ERROR)
	{
		ERROR_LOG("File open failed\n");
		syslog(LOG_ERR,"File open failed\n");
		command_status.success = false;
	}
	
	do
	{
		// if file not opened
		if(command_status.success == false)
		{
			break;
		}
		
		/********************************************************* 
		*  STEP 1 :
		*  Opens a stream socket bound to port 9000
		*  failing and returning -1 if any of the 
		*  socket connection steps fail.
		*********************************************************/
		
		/*
		*	-> get socket addr struct information
		*	-> returns 0 on success
		*	-> node 	- identify an Internet host
		*	-> service 	- identify an Internet host
		*	-> hints 	- specifies criteria for 
					  selecting the socket address
		*	-> res		- socket address structures returned
		*/
		ret = getaddrinfo(NULL, "9000", &hints, &result);
		if (ret != RET_SUCCESS)
		{
			ERROR_LOG("getaddrinfo() failed\n");
			syslog(LOG_ERR,"getaddrinfo() failed\n");
			command_status.success = false;
			break;
		}
		if(result == NULL)
		{
			ERROR_LOG("getaddrinfo() malloc failed\n");
			syslog(LOG_ERR,"getaddrinfo() malloc failed\n");
			command_status.success = false;
			break;
		}

		/*
		*	-> creates endpoint for communication
		*	-> returns FD of endpoint created
		*	-> domain 	- AF_INET -> IPv4 Internet protocols
		*	-> type 	- SOCK_STREAM
		*	-> protocol 	- 0
		*/
		socket_fd = socket(result->ai_family, result->ai_socktype,
				    result->ai_protocol);
		if(socket_fd == RET_ERROR)
		{
			ERROR_LOG("Socket creation failed\n");
			syslog(LOG_ERR,"Socket creation failed\n");
			command_status.success = false;
			break;
		}
		
		if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, 
				sizeof(int)) == -1)
               {
			ERROR_LOG("Set socket opt failed\n");
			syslog(LOG_ERR,"Set socket opt failed\n");
			command_status.success = false;
			break;
		}
		
		/*
		*	-> bind a address to a socket
		*	-> returns FD of endpoint created
		*	-> sockfd 	- socket file descriptor
		*	-> addr 	- address to assign
		*	-> addrlen 	- size, in bytes, of the 
		*			  address structure
		*/
		ret = bind(socket_fd, result->ai_addr,
			    sizeof(struct sockaddr));
		if(ret == RET_ERROR)
		{
			ERROR_LOG("Bind failed\n");
			syslog(LOG_ERR,"Bind failed\n");
			command_status.success = false;
			break;
		}
		
		// free malloced addr struct returned
		freeaddrinfo(result);
		
		/*
		*	-> listen for connections on a socket
		*	-> returns 0 on success, -1 on error
		*	-> sockfd 	- socket file descriptor
		*	-> backlog 	- No. of pending connections 
		*			  allowed
		*/	
		ret = listen(socket_fd, BACKLOG_CONNECTIONS);
		if(ret == RET_ERROR)
		{
			ERROR_LOG("Listen failed\n");
			syslog(LOG_ERR,"Listen failed\n");
			command_status.success = false;
			break;
		}
		
		while(1)
		{
			/********************************************************* 
			*  STEP 2 :
			*  Listens for and accepts a connection.
			*  Logs message to the syslog 
			*  “Accepted connection from xxx” 
			*  where XXXX is the IP address of the connected client. 
			*********************************************************/
			
			/*
			*	-> accept a connection on a socket
			*	-> returns a FD on success, -1 on error
			*	-> sockfd 	- socket file descriptor
			*	-> addr 	- location to store connection
			*			  address of client
			*	-> addrlen	- length of socket address
			*/
			accept_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addrlen);
			if(accept_fd == RET_ERROR)
			{
				ERROR_LOG("Accept failed\n");
				syslog(LOG_ERR,"Accept failed\n");
				command_status.success = false;
				break;
			}
			
			inet_ntop(client_addr.ss_family,
			    	   get_in_addr((struct sockaddr *)&client_addr),
			    	   s, sizeof s);
			//printf("server: got connection from %s\n", s);
			syslog(LOG_INFO,"Accepted connection from %s\n",s);
			
			/********************************************************* 
			*  STEP 3 : 
			*  Receives data over the connection and 
			*  appends to file /var/tmp/aesdsocketdata
			*********************************************************/
			
			/*
			*	-> receive a message from a socket
			*	-> returns number of bytes received on success
			*	   or -1 on error
			*	-> sockfd 	- accepted socket file descriptor
			*	-> buf[.len] 	- to store bytes received
			*	-> len		- length of buffer
			* 	-> flags	- MSG_WAITALL - Blocking mode
			*/
			do
			{
				recv_bytes = recv(accept_fd, buf, BUF_LEN, 0);
				if(recv_bytes == RET_ERROR)
				{
					ERROR_LOG("Receive failed\n");
					syslog(LOG_ERR,"Receive failed\n");
					command_status.success = false;
					break;
				}
				//buf[recv_bytes]='\0';
				DEBUG_LOG("Buffer : %ld",recv_bytes);
				DEBUG_LOG("Buffer : %s",buf);
				
				// write to file
				ret = write(data_file_fd, buf, recv_bytes);
				if(ret == RET_ERROR)
				{
					ERROR_LOG("File write failed\n");
					syslog(LOG_ERR,"File write failed\n");
					command_status.success = false;
					break;
				}
			}while(buf[recv_bytes-1] != '\n');
			if(command_status.success == false)
			{
				break;
			}
			
			/********************************************************* 
			*  STEP 4 : 
			*  Returns the full content of /var/tmp/aesdsocketdata 
			*  to the client as soon as the received data packet 
			*  completes.
			*  
			*********************************************************/
			
			// lseek to start of the file
			/*
			*	-> send a message on a socket
			*	-> returns number of bytes sent on success
			*	   or -1 on error
			*	-> fd 		- file descriptor
			*	-> offset 	- file offset position
			*	-> whence	- SEEK_SET, SEEK_CUR, SEEK_END
			*/
			off_t seek_ret = lseek(data_file_fd, 0, SEEK_SET);
			if(seek_ret == RET_ERROR)
			{
				ERROR_LOG("lseek failed\n");
				syslog(LOG_ERR,"lseek failed\n");
				command_status.success = false;
				break;
			}
			
			// read from file
			do
			{
				bytes_read = read(data_file_fd, read_buf, BUF_LEN);
				if(bytes_read == RET_ERROR)
				{
					ERROR_LOG("File read failed\n");
					syslog(LOG_ERR,"File read failed\n");
					command_status.success = false;
					break;
				}
				
				DEBUG_LOG("Bytes Read : %ld",bytes_read);			
				DEBUG_LOG("Read Buffer : \n%s",read_buf);
				
				/*
				*	-> send a message on a socket
				*	-> returns number of bytes sent on success
				*	   or -1 on error
				*	-> sockfd 	- sending socket file descriptor
				*	-> buf[.len] 	- buffer with bytes to be sent
				*	-> len		- length of buffer
				* 	-> flags	- 0
				*/
				send_bytes = send(accept_fd, read_buf, bytes_read, 0);
				if(send_bytes == RET_ERROR)
				{
					ERROR_LOG("Send failed\n");
					syslog(LOG_ERR,"Send failed\n");
					command_status.success = false;
					break;
				}
			}while(read_buf[send_bytes-1] != '\n');
			if(command_status.success == false)
			{
				break;
			}
			
			
			/********************************************************* 
			*  STEP 5 : 
			*  Logs message to the syslog “Closed connection from XXX”
			*  where XXX is the IP address of the connected client.
			*  
			*  Restarts accepting connections from new clients forever 
			*  in a loop until SIGINT or SIGTERM is received.
			*********************************************************/
			close(accept_fd);
			syslog(LOG_INFO,"Closed connection from %s\n",s);
		}
		
	}while(0);
	
	global_clean_up();
}

