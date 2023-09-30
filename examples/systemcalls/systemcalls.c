#include "systemcalls.h"
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    int ret;

    ret = system(cmd);

    // check if system() executed correctly
    if(-1 == ret)
    {
        return false;
    }

    // check for error in command execution
    if(!WIFEXITED(ret))
    {
	if(WEXITSTATUS(ret))
	{
		return false;
	}
    }

    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    pid_t pid;
    int ret;
    int status;

    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/

    pid = fork();

    // if fork() failed
    if(pid < 0)
    {
        return false;
    }
    else if(pid == 0)
    {   
        // execute command for successful fork()
        ret = execv(command[0], command);
        if(-1 == ret)
        {
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        // check exit status of command
        waitpid(pid,&status,0);
        if(WIFEXITED(status))
        {
            if(WEXITSTATUS(status))
            {
                return false;
            }
            else
            {
                return true;
            }
        }
        else
        {
            return false;
        }
    }

    va_end(args);

    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    pid_t pid;
    int ret;
    int status;

    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
    int fd = open(outputfile, (O_RDWR | O_CREAT | O_TRUNC),
           (S_IWUSR | S_IRUSR | S_IWGRP | S_IRGRP | S_IROTH ));

    if (fd == -1)
    {
        va_end(args);
        return false;
    }

    pid = fork();

    // if fork() failed
    if(pid < 0)
    {
        va_end(args);
        return false;
    }
    else if(pid == 0)
    {   
        // redirect output to file
        if (dup2(fd, 1) == -1)
        {
            va_end(args);   
            return false;
        }
        close(fd);
        // execute command for successful fork()
        ret = execv(command[0], command);
        if(-1 == ret)
        {                
            va_end(args);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        // check exit status of command
        waitpid(pid,&status,0);
        if(WIFEXITED(status))
        {
            if(WEXITSTATUS(status))
            {
                va_end(args);
                return false;
            }
            else
            {
                va_end(args);
                return true;
            }
        }
        else
        {
            va_end(args);
            return false;
        }
    }

    va_end(args);

    return true;
}
