/***********************************************************************
 * @file      		writer.c
 * @version   		0.1
 * @brief			Implementation for writing a string to a file
 *
 * @author    		Amey More, Amey.More@Colorado.edu
 * @date      		Sept 9, 2023
 *
 * @institution 	University of Colorado Boulder (UCB)
 * @course      	ECEN 5713: Advanced Embedded Software Development
 * @instructor  	Dan Walkes
 *
 * @assignment 		Assignment 2
 * @due        		Sept 10, 2023 at 11:59 PM
 *
 * @references
 * 
 ************************************************************************/
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// Application entry point
int main(int argc, char *argv[])
{
    char * writefile;
    char * writestr;
    int file_open_ret;
    ssize_t write_file_value;

    // to create logs from application
    openlog(NULL,0,LOG_USER);

    // validating arguments
    if(argc != 3)
    {
        syslog(LOG_ERR,"Invalid number of arguments : %d\n",argc);
        syslog(LOG_ERR,"Usage: $ writer <writefile> <writestring>\n");
        return 1;
    }

    // parse arguments
    writefile = argv[1];
    writestr = argv[2];

    // open file specified in the argument
    // enter error log if failed
    file_open_ret = open(writefile, (O_RDWR | O_CREAT | O_TRUNC),
           (S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH ));
    if (file_open_ret == -1)
    {
        syslog(LOG_ERR,"Cannot open file : %s\n",writefile);
        return 1;
    }

    // write string specified in the arguments to the file
    // enter error log if failed
    write_file_value = write(file_open_ret, writestr, strlen (writestr));
    if (write_file_value == -1)
    {
        syslog(LOG_ERR,"Write failed!!!");
        return 1;
    }

    // enter log for successful write
    syslog(LOG_DEBUG,"Writing %s to %s",writestr,writefile);

    return 0;
}