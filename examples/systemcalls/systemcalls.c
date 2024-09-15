
/*******************************************************************************
 * Copyright (C) 2024 by Shruthi Thallapally
 *
 * Redistribution, modification or use of this software in source or binary
 * forms is permitted as long as the files maintain this copyright. Users are
 * permitted to modify this and use it to learn about the field of embedded
 * software. Shruthi Thallapally and the University of Colorado are not liable for
 * any misuse of this material.
 * ****************************************************************************/

/**
 * @file    systemcalls.c
 * @brief   Functions to execute system calls
 *
 * @author  Shruthi Thallapally
 * @date    09/14/2024
 * @references  
 * 1. https://man7.org/linux/man-pages/man3/syslog.3.html
 * 2. https://linux.die.net/man/3/system
 * 3. https://www.geeksforgeeks.org/exit-status-child-process-linux/
 * 4. https://linux.die.net/man/3/execv 
 *
 */
#include "systemcalls.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <syslog.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>


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
 //open syslog for log statements
    openlog("systemcallslog", LOG_PID | LOG_CONS , LOG_USER);
    int exit_status;
    int system_status;
    if(cmd ==NULL)
    {
        syslog(LOG_ERR,"NULL Command!");
        return false;
    }

    system_status=system(cmd);

    if(system_status==-1)
    {
         syslog(LOG_ERR, "Child process could not be created, %s\n", strerror(errno));
        return false;
    }
    else 
    {
        if (WIFEXITED(system_status) )
        {
           exit_status= WEXITSTATUS(system_status);
           if(exit_status==0)
           {
                syslog(LOG_INFO, "Command executed successfully\n" );
                return true;
            }else
            {
                syslog(LOG_ERR, "Child process exited with %d status\n", exit_status);
                 return false;
            }

        } else if(WIFSIGNALED(system_status))
        {
            exit_status= WTERMSIG(system_status);
            syslog(LOG_ERR, "Child process terminated with %d status\n", exit_status);
             return false;
        }else
        {
            syslog(LOG_ERR, "Child process is abnormally terminated \n");
             return false;
        }
    
    }
}
bool check_abs_path(const char *path)
{
    if (path[0]=='/')
    {
        return true;
    }else
    {
        return false;
    }
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
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
 //   command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    if(check_abs_path(command[0])==false)
    {
        syslog(LOG_ERR, "Path must be an absolute path \n");
        va_end(args);
        return false;
    }
        pid_t pid = fork();
    if(pid==0)
    {
        execv(command[0],command);
        perror("execv failed");
        _exit(EXIT_FAILURE);
    }
    else if(pid==-1)
    {
        perror("fork failed");
        va_end(args);
        return false;
    }
    else
    {
        int wait_status;
        if(waitpid(pid,&wait_status,0)==-1)
        {
            syslog(LOG_ERR,"waitpid failed\n");
            va_end(args);
            return false;
        }
        if(WIFEXITED(wait_status) )
        {
            if(WEXITSTATUS(wait_status) == 0)
            {
                va_end(args);
                return true;
            }
            else{
                va_end(args);
                return false;
            }
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
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

    
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
 //   command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

   if(check_abs_path(command[0])==false)
    {
        syslog(LOG_ERR, "Path must be an absolute path \n");
        va_end(args);
        return false;
    }
    pid_t pid = fork();
    if(pid==0)
    {
    // Open the output file for writing, create if it doesn't exist, truncate to zero length
    int fd = open(outputfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open failed");
        _exit(EXIT_FAILURE);
    }
     //Lines 241 to 245 are partially generated by chatgpt using the search "redirecting standard out to a file specified by outputfile"
    // Redirect stdout (file descriptor 1) to the file
    if (dup2(fd, STDOUT_FILENO) == -1)
     {
        perror("dup2 failed");
        close(fd);
        _exit(EXIT_FAILURE);
    }

        // Close the file descriptor since it's now duplicated to stdout
        close(fd);
        execv(command[0],command);
        perror("execv failed");
        _exit(EXIT_FAILURE);
    }
    else if(pid==-1)
    {
        perror("fork failed");
        va_end(args);
        return false;
    }
    else
    {
        int wait_status;
        if(waitpid(pid,&wait_status,0)==-1)
        {
            syslog(LOG_ERR,"waitpid failed\n");
            va_end(args);
            return false;
        }
        if(WIFEXITED(status) )
        {
            if(WEXITSTATUS(wait_status) == 0)
            {
                va_end(args);
                return true;
            }
            else{
                va_end(args);
                return false;
            }
        }
    }
    va_end(args);

    return true;
}
