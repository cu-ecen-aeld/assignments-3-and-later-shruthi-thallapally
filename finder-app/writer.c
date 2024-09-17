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
 * @file    writer.c
 * @brief	Assignment 2. C implementation of writer.sh
 * @author  Shruthi Thallapally
 * @date    09-08-2024
 * @References https://codeforwin.org/c-programming/c-program-check-file-or-directory-exists-not
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <syslog.h>
 #include <errno.h>
 #include <string.h>
 #include <unistd.h>
 #include <linux/stat.h>
 #include <sys/stat.h>
 #include <libgen.h>

 int main(int argc,char *argv[])
 {
    //open syslog for log statements
    openlog("writerlog", LOG_PID | LOG_CONS , LOG_USER);

    //Check if 3 arguments are not given
    if (argc != 3) {
        syslog(LOG_ERR, "Insufficient arguments. Usage: %s <filename> <string>", argv[0]);
        printf("Insufficient arguments\n  Usage: %s <filename> <string>\n", argv[0]);
        closelog();
        return 1;
    }

    //extracting file and string from arguments
    char *writefile = argv[1];
    char *writestr = argv[2];

    //extracting directory from the writefile
    char *directory=strdup(writefile); //making a copy of writefile string to extract directory name
    directory=dirname(directory); //getting the directory name

    //checking if directory exist
    struct stat dir_exists;
    if(stat(directory,&dir_exists)!=0)
    {
        syslog(LOG_ERR,"Directory %s doesn't exist",directory);
        printf("Invalid Directory\n");
        free(directory);
        closelog();
        return 1;
    }

    //opening file for writing
    FILE *file=fopen(writefile,"w");
    if(file==NULL)
    {
        syslog(LOG_ERR,"Error: %s opening the file: %s ",strerror(errno),writefile);
        printf("Error opening the file\n");
        free(directory);
        closelog();
        return 1;
    }

    //writing the string to the file
    if(fprintf(file,"%s\n",writestr)<0 )
    {
        syslog(LOG_ERR,"Error: %s writing the string to file %s",strerror(errno),writefile);
        printf("Error writing the string to file\n");
        free(directory);
        closelog();
        return 1;
    }

    syslog(LOG_DEBUG,"Writing %s to %s",writestr,writefile);

    fclose(file);
    free(directory);
    closelog();
    return 0;

 }