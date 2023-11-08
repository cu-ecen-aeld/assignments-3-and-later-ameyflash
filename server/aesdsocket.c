/***********************************************************************
 * @file      		aesdsocket.c
 * @version   		0.1
 * @brief		    Socket server application code with multi-threading
 *
 * @author    		Amey More, Amey.More@Colorado.edu
 * @date      		Oct 12, 2023
 *
 * @institution 	University of Colorado Boulder (UCB)
 * @course      	ECEN 5713: Advanced Embedded Software Development
 * @instructor  	Dan Walkes
 *
 * @assignment 	    Assignment 6
 * @due        	    Oct 15, 2023 at 11:59 PM
 *
 * @references
 * https://www.gnu.org/software/libc/manual/html_node/
 * Termination-in-Handler.html
 *
 * https://www.thegeekstuff.com/2012/02/c-daemon-process/ 
 *
 * https://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch
 * 
 * https://www.geeksforgeeks.org/strftime-function-in-c/
 ************************************************************************/
#include "aesdsocket.h"

#define USE_AESD_CHAR_DEVICE (1) // used for build switching

#if (USE_AESD_CHAR_DEVICE == 1)
	#define DATA_FILE "/dev/aesdchar"
#else
	#define DATA_FILE "/var/tmp/aesdsocketdata"
#endif

/*
*   Global Data
*/
// Process termination
volatile sig_atomic_t terminate_process = 0;
// Daemon application
bool daemon_mode = false;
// Outout data file
//char *data_file = "/var/tmp/aesdsocketdata";
int data_file_fd;
// Server & Client Socket fd
int socket_fd;
int accept_fd;
// linked list head init
SLIST_HEAD(head_s, node) head;
node_t * new_node = NULL;
// thread mutex
pthread_mutex_t mutex;
// timestamp struct
timestamp_data_t timestamp_data;

#if (USE_AESD_CHAR_DEVICE == 1)
const char *ioctl_str = "AESDCHAR_IOCSEEKTO:";
#endif

/*
*   Function Prototypes
*/
int socket_application();
int open_socket();
int start_daemon();
int start_communication();
void global_clean_up();
void *recv_send_thread(void *thread_param);
int setup_timestamp();
void *timestamp_thread(void *timestamp_param);


void handle_termination(int signo)
{
    int ret;
	if(signo == SIGINT || signo == SIGTERM)
	{
		syslog(LOG_INFO,"Caught signal, exiting\n");

		ret = shutdown(socket_fd,SHUT_RDWR);
		if(ret == RET_ERROR)
		{
			syslog(LOG_ERR,"shutdown failed");
		}

#if (USE_AESD_CHAR_DEVICE != 1)
        ret = pthread_cancel(timestamp_data.thread_id);
        if(ret != 0)
        {
            syslog(LOG_ERR,"pthread cancel failed");
        }
#endif

		terminate_process = 1;
	}
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    return &(((struct sockaddr_in*)sa)->sin_addr);
}

// Application entry point
int main(int argc, char *argv[])
{
	int opt;
    int ret;

	// for data output file
	int file_flags;
	mode_t file_mode;

    // for linked list
    SLIST_INIT(&head);

	// to create logs from application
	openlog(NULL,0,LOG_USER);

    // mutex for threads
    ret = pthread_mutex_init(&mutex, NULL);
    if(ret != 0)
    {
        syslog(LOG_ERR,"mutex init failed");
        return -1;
    }

    // to parse arguments
	while((opt = getopt(argc, argv, "d")) != -1)  
	{
		switch(opt)  
        	{
        		case 'd':
	        		daemon_mode = true;
	        		break; 
        	}
	}

	// signal handler for SIGINT and SIGTERM
	signal(SIGINT, handle_termination);
	signal(SIGTERM, handle_termination);
	
	// run socket server application
    syslog(LOG_INFO,"AESD Socket application started");
	ret = socket_application();

	global_clean_up();
	
	// return -1 if any command in 
	// socket_application() fails
	if(ret == RET_ERROR)
    {
        DEBUG_LOG("Application Failure\n");
        DEBUG_LOG("Check logs\n");
        return -1;
    }
    else
    {
        return 0;
    }
}

// Socket Application commands
int socket_application()
{
    int ret = 0;
    /********************************************************* 
    *  STEP 1 :
    *  Opens a stream socket bound to port 9000
    *  failing and returning -1 if any of the 
    *  socket connection steps fail.
    *********************************************************/
    ret = open_socket();
    if(ret == RET_ERROR)
    {
        return -1;
    }

    /********************************************************* 
    *  STEP 2 :
    *  Start as a daemon if specified by the user
    *********************************************************/
    if(daemon_mode)
    {
        ret = start_daemon();
        if(ret == RET_ERROR)
        {
            return -1;
        }
    }

#if (USE_AESD_CHAR_DEVICE != 1)
    ret = setup_timestamp();
    if(ret == RET_ERROR)
    {
        return -1;
    }
#endif

    /********************************************************* 
    *  STEP 3:
    *  Listens for and accepts a connection.
    *  Logs message to the syslog 
    *  “Accepted connection from xxxx” 
    *  where XXXX is the IP address of the connected client. 
    *********************************************************/
    ret = listen(socket_fd, BACKLOG_CONNECTIONS);
    if(ret == RET_ERROR)
    {
        syslog(LOG_ERR,"Listen failed");
        return -1;
    }

    ret = start_communication();
    if(ret == RET_ERROR)
    {
        return -1;
    }

    return 0;
}

// setup socket and address
int open_socket()
{
    int ret;
	struct addrinfo hints;
	struct addrinfo *result;

    // for setsockopt()
	int yes=1;

    memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;	        /* IPv4 */
	hints.ai_socktype = SOCK_STREAM;    /* stream socket */
	hints.ai_flags = AI_PASSIVE;        /* For local IP address */
	hints.ai_protocol = 0;              /* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

    ret = getaddrinfo(NULL, PORT, &hints, &result);
    if (ret != RET_SUCCESS)
    {
        syslog(LOG_ERR,"getaddrinfo() failed");
        return -1;
    }
    // check for malloc success
    if(result == NULL)
    {
        syslog(LOG_ERR,"getaddrinfo() malloc failed");
        return -1;
    }

    socket_fd = socket(result->ai_family, result->ai_socktype,
                        result->ai_protocol);
    if(socket_fd == RET_ERROR)
    {
        syslog(LOG_ERR,"Socket creation failed");
        return -1;
    }

    ret = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, 
                        sizeof(int)); 
    if (ret == RET_ERROR)
    {
        syslog(LOG_ERR,"Set socket opt failed");
        return -1;
    }

    ret = bind(socket_fd, result->ai_addr,
                sizeof(struct sockaddr));
    if(ret == RET_ERROR)
    {
        syslog(LOG_ERR,"Bind failed");
        return -1;
    }
    
    // free malloced addr struct returned
    freeaddrinfo(result);
    result = NULL;

    return 0;
}

// to start application as daemon
int start_daemon()
{
    int fd;
    int ret;

    pid_t process_id = fork();
    if (process_id < 0)
    {
        syslog(LOG_ERR,"Fork failed");
        return -1;
    }
    
    // PARENT PROCESS. Need to kill it.
    if (process_id > 0)
    {
        syslog(LOG_INFO,"Parent process terminated");
        // return success in exit status
        exit(0);
    }
    
    //unmask the file mode
    umask(0);
    //set new session
    pid_t sid = setsid();
    if(sid < 0)
    {
        syslog(LOG_ERR,"setsid failed");
        return -1;
    }
    // Change the current working directory to root.
    ret = chdir("/");
    if(ret == RET_ERROR)
    {
        syslog(LOG_ERR,"chdir failed");
        return -1;
    }
    // Close stdin. stdout and stderr
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // redirect stdin, stdout, stderr to /dev/null
    fd = open("/dev/null", O_RDWR);
    if(fd == RET_ERROR)
    {
        syslog(LOG_ERR,"/dev/null open failed");
        return -1;
    }
    ret = dup2(fd, STDIN_FILENO);
    if(ret == RET_ERROR)
    {
        syslog(LOG_ERR,"stdin redirect failed");
        return -1;
    }
    ret = dup2(fd, STDOUT_FILENO);
    if(ret == RET_ERROR)
    {
        syslog(LOG_ERR,"stdout redirect failed");
        return -1;
    }
    ret = dup2(fd, STDERR_FILENO);
    if(ret == RET_ERROR)
    {
        syslog(LOG_ERR,"stderr redirect failed");
        return -1;
    }
    close(fd);
    return 0;
}

// accept, receive and send socket commands
int start_communication()
{
    // int ret;
    int pt_ret;

    // for pthreads
    void * thread_rtn = NULL;

    // for accept() command
	struct sockaddr_storage client_addr;
	socklen_t client_addrlen = sizeof(struct sockaddr_storage);

    while(!terminate_process)
	{
        accept_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_addrlen);
        if(accept_fd == RET_ERROR)
        {
            if(terminate_process == 0)
            {
                syslog(LOG_ERR,"Accept failed");
                return -1;
            }
            else
            {
                return 0;
            }
        }
        
        /*
        inet_ntop(client_addr.ss_family,
                    get_in_addr((struct sockaddr *)&client_addr),
                    s, sizeof s);
        syslog(LOG_INFO,"Accepted connection from %s",s);

        ret = recv_send_thread();
        if(ret == RET_ERROR)
        {
            return -1;
        }
        */

        /********************************************************* 
        *  STEP 6 : 
        *  Logs message to the syslog “Closed connection from XXX”
        *  where XXX is the IP address of the connected client.
        *  
        *  Restarts accepting connections from new clients forever 
        *  in a loop until SIGINT or SIGTERM is received.
        *********************************************************/
        /*
        close(accept_fd);
        syslog(LOG_INFO,"Closed connection from %s",s);
        */

        // add new node in linked list
        new_node = malloc(sizeof(node_t));
        if(new_node == NULL)
        {
            syslog(LOG_ERR,"Node malloc failed");
            return -1;
        }
        new_node->thread_data.mutex = &mutex;
        new_node->thread_data.thread_complete = false;
        new_node->thread_data.accept_fd = accept_fd;
        new_node->thread_data.client_addr = (struct sockaddr_storage *)&client_addr;

        // create threads and start communication
        pt_ret = pthread_create(&(new_node->thread_data.thread_id), NULL, \
                                    recv_send_thread, &(new_node->thread_data));
        if(pt_ret == RET_ERROR)
        {
            syslog(LOG_ERR, "Thread create failed");
            free(new_node);
            return -1;
        }
        
        // Actually insert the node e into the queue
        SLIST_INSERT_HEAD(&head, new_node, nodes);
        new_node = NULL;

        // check for thread completion
        SLIST_FOREACH(new_node, &head, nodes)
        {
            if(new_node->thread_data.thread_complete)
            {
                pt_ret = pthread_join(new_node->thread_data.thread_id,&thread_rtn);
                if(pt_ret == RET_ERROR)
                {
                    syslog(LOG_ERR, "Thread join failed");
                    return -1;
                }
                if(thread_rtn == NULL)
                {
                    return -1;
                }
                syslog(LOG_INFO, "Thread join %ld",new_node->thread_data.thread_id);
            }
        } 
    }

    return 0;
}

// thread function for receive and send commmands
void *recv_send_thread(void *thread_param)
{
	int ret;
    	// receive bytes
	ssize_t recv_bytes = 0;
	char recv_buf[BUF_LEN];
	
	// send bytes
	ssize_t send_bytes = 0;
	char send_buf[BUF_LEN];
	ssize_t bytes_read;

    	// to print IP
    	char s[INET6_ADDRSTRLEN];

	memset(recv_buf, 0, BUF_LEN);
	memset(send_buf, 0, BUF_LEN);

	thread_data_t *thread_data = (thread_data_t*)thread_param;

	inet_ntop(thread_data->client_addr->ss_family,
                get_in_addr((struct sockaddr *)&(thread_data->client_addr)),
                s, sizeof s);
	syslog(LOG_INFO,"Accepted connection from %s",s);

	syslog(LOG_INFO,"Started thread %ld",thread_data->thread_id);

	    // open data file
	file_flags = (O_RDWR | O_CREAT | O_APPEND);
	file_mode = (S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH);
	data_file_fd = open(DATA_FILE, file_flags, file_mode);
	if(data_file_fd == RET_ERROR)
	{
		syslog(LOG_ERR,"Data file open failed");
        DEBUG_LOG("Application Failure\n");
        DEBUG_LOG("Check logs\n");
        return -1;
	}

    /********************************************************* 
    *  STEP 4 : 
    *  Receives data over the connection and 
    *  appends to file /var/tmp/aesdsocketdata
    *********************************************************/

    // receive and write
    do
    {
        // receive data on socket
        recv_bytes = recv(thread_data->accept_fd, recv_buf, BUF_LEN, 0);
        if(recv_bytes == RET_ERROR)
        {
            syslog(LOG_ERR,"Receive failed");
            return NULL;
        }
        
#if (USE_AESD_CHAR_DEVICE == 1)
	if (strcmp(recv_buf, ioctl_str) == 0)
	{
		struct aesd_seekto aesd_seekto_data;
		sscanf(recv_buf, "AESDCHAR_IOCSEEKTO:%d,%d", &aesd_seekto_data.write_cmd, &aesd_seekto_data.write_cmd_offset); 
		
	    	if(ioctl(data_file_fd, AESDCHAR_IOCSEEKTO, &aesd_seekto_data))
	    	{
			syslog(LOG_ERR,"ioctl failed\n");
			return NULL;
	    	}
	}
#endif

#if (USE_AESD_CHAR_DEVICE != 1)
	// acquire lock
	ret = pthread_mutex_lock(thread_data->mutex);
	if(ret == RET_ERROR)
	{
		syslog(LOG_ERR,"mutex lock failed\n");
		return NULL;
	}

        // write data to file
        ret = write(data_file_fd, recv_buf, recv_bytes);
        if(ret == RET_ERROR)
        {
            syslog(LOG_ERR,"File write failed");
            return NULL;
        }
        
        // release lock
    	ret = pthread_mutex_unlock(thread_data->mutex);
    	if(ret == RET_ERROR)
    	{
        	syslog(LOG_ERR,"mutex unlock failed\n");
        	return NULL;
    	}
#endif
    }while((memchr(recv_buf, '\n', recv_bytes)) == NULL);


    /********************************************************* 
    *  STEP 5 : 
    *  Returns the full content of /var/tmp/aesdsocketdata 
    *  to the client as soon as the received data packet 
    *  completes.
    *********************************************************/

    off_t seek_ret = lseek(data_file_fd, 0, SEEK_SET);
    if(seek_ret == RET_ERROR)
    {
        syslog(LOG_ERR,"lseek failed");
        return NULL;
    }

    // read and send
    do
    {
        // acquire lock
	ret = pthread_mutex_lock(thread_data->mutex);
	if(ret == RET_ERROR)
	{
		syslog(LOG_ERR,"mutex lock failed\n");
		return NULL;
	}
    
        // read data from file
        bytes_read = read(data_file_fd, send_buf, BUF_LEN);
        if(bytes_read == RET_ERROR)
        {
            syslog(LOG_ERR,"File read failed");
            return NULL;
        }
        
    	// release lock
	ret = pthread_mutex_unlock(thread_data->mutex);
	if(ret == RET_ERROR)
	{
		syslog(LOG_ERR,"mutex unlock failed\n");
		return NULL;
	}
        
        // send data on socket
        send_bytes = send(thread_data->accept_fd, send_buf, bytes_read, 0);
        if(send_bytes == RET_ERROR)
        {
            syslog(LOG_ERR,"Send failed");
            return NULL;
        }
    }while(bytes_read > 0);

    close(thread_data->accept_fd);
    syslog(LOG_INFO,"Closed connection from %s",s);

    // thread completed
    thread_data->thread_complete = true;
    return thread_param;
}

// close and free resources used
void global_clean_up()
{
	int ret;
	
    syslog(LOG_INFO,"Performing clean up");

	// Close data file
	ret = close(data_file_fd);
	if(ret == RET_ERROR)
	{
		syslog(LOG_ERR,"File close failed");
	}
	
#if (USE_AESD_CHAR_DEVICE != 1)
	// delete data file
	ret = unlink(DATA_FILE);
	if(ret == RET_ERROR)
	{
		syslog(LOG_ERR,"File delete failed");
	}
#endif

    // free the elements from the queue
    while (!SLIST_EMPTY(&head))
    {
        new_node = SLIST_FIRST(&head);
        SLIST_REMOVE(&head, new_node, node, nodes);
        free(new_node);
        new_node = NULL;
    }

#if (USE_AESD_CHAR_DEVICE != 1)
    // join timestamp thread
    pthread_join(timestamp_data.thread_id, NULL);
#endif

    // destroy mutex
    pthread_mutex_destroy(&mutex);
	
	// close socket
	close(socket_fd);
	
	// Close syslog
	syslog(LOG_INFO,"AESD Socket application end");
	closelog();
}

#if (USE_AESD_CHAR_DEVICE != 1)
// timestamp struct init
int setup_timestamp()
{
    int pt_ret;

    timestamp_data.mutex = &mutex;
    timestamp_data.time_interval_secs = 10;

    // create and start timestamp thread
    pt_ret = pthread_create(&(timestamp_data.thread_id), NULL, \
                                timestamp_thread, &(timestamp_data));
    if(pt_ret == RET_ERROR)
    {
        syslog(LOG_ERR, "Timestamp Thread create failed");
        return -1;
    }

    return 0;
}

// thread to log timestamps
void *timestamp_thread(void *thread_param)
{
    int ret;
    timestamp_data_t *thread_ts_data = (timestamp_data_t *)thread_param;

    time_t t ;
    struct tm *tmp ;
    char time_stamp[50];
    struct timespec ts;

    syslog(LOG_INFO, "Timestamp thread started");

    while(1)
    {
        ret = clock_gettime(CLOCK_MONOTONIC, &ts);
        if(ret)
        {
            syslog(LOG_ERR,"clock_gettime failed");
            return NULL;
        }
        ts.tv_sec += thread_ts_data->time_interval_secs;
        ret = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, \
                                &ts, NULL);
        if(ret)
        {
            syslog(LOG_ERR,"clock_nanosleep failed");
            return NULL;
        }

        time( &t );

        tmp = localtime( &t );
     
        // using strftime to display time
        int time_len = strftime(time_stamp, sizeof(time_stamp), "timestamp: %Y, %b %d, %H:%M:%S\n", tmp);

        // acquire lock
        ret = pthread_mutex_lock(thread_ts_data->mutex);
        if(ret == RET_ERROR)
        {
            syslog(LOG_ERR,"mutex lock failed");
            return NULL;
        }

        // write data to file
        ret = write(data_file_fd, time_stamp, time_len);
        if(ret == RET_ERROR)
        {
            syslog(LOG_ERR,"File write failed");
            return NULL;
        }

        // release lock
        ret = pthread_mutex_unlock(thread_ts_data->mutex);
        if(ret == RET_ERROR)
        {
            syslog(LOG_ERR,"mutex unlock failed");
            return NULL;
        }
    }

}
#endif
