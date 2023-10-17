/***********************************************************************
 * @file      		aesdsocket.c
 * @version   		0.1
 * @brief		Socket server application code
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
 * 
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include <sys/queue.h>
#include <time.h> 

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...) printf("INFO: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("ERROR: " msg "\n" , ##__VA_ARGS__)

#define RET_SUCCESS 		(0)
#define RET_ERROR 		    (-1)

#define PORT                ("9000")
#define BACKLOG_CONNECTIONS	(10)

#define BUF_LEN		(1024)

typedef struct
{
    pthread_t thread_id;
    pthread_mutex_t *mutex;
    bool thread_complete;
    int accept_fd;
    struct sockaddr_storage *client_addr;
}thread_data_t;

typedef struct node
{
    thread_data_t thread_data;

    SLIST_ENTRY(node) nodes;
}node_t;

typedef struct
{
    pthread_t thread_id;
    pthread_mutex_t *mutex;
    int time_interval_secs;
}timestamp_data_t;

typedef struct
{
    bool success;
}command_status_t;
