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
 * 
 ************************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...) printf("INFO: " msg "\n" , ##__VA_ARGS__)

#define ERROR_LOG(msg,...) printf("ERROR: " msg "\n" , ##__VA_ARGS__)

#define RET_SUCCESS 		(0)
#define RET_ERROR 		(-1)

#define BACKLOG_CONNECTIONS	(10)

#define BUF_LEN		(1024)

typedef struct
{
    bool success;
}command_status_t;
