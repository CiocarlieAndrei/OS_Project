#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h> 
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#define SHELL_SCRIPT_PATH "/home/andrei/C_Programms/OS_Project/check_file.sh "  // CHANGE THIS PATH TO THE PATH OF THE SHELL SCRIPT PROGRAM THAT CHECKS IF THE FILE IS CORRUPTED

void processDirectory(char *dir_path, char *output_dir, char *isolated_dir, int* cfc) { // i = counts the number of directories at input, cfc = counts the no of corrupted files
    DIR *dir;
    struct dirent *in;
    struct stat filestat;
    char path[4096];
    if ((dir = opendir(dir_path)) == NULL) { // Open the directory
        perror("Can't open directory\n");
        exit(EXIT_FAILURE);
    }
    while ((in = readdir(dir)) != NULL) { // Loop through the directory
        if (strcmp(in->d_name, ".") == 0 || strcmp(in->d_name, "..") == 0) {
            continue;
        }

        sprintf(path, "%s/%s", dir_path, in->d_name); // Construct the path of the current entry

        if (stat(path, &filestat) == -1) {
            perror("Error getting file status\n");
            exit(EXIT_FAILURE);
        }

        if (S_ISREG(filestat.st_mode)) { // if the current entry is a file
            int pipefd[2];
            if (pipe(pipefd) == -1) { // Create a pipe to communicate the grand child process with the child process
                perror("pipe");
                exit(EXIT_FAILURE);
            }
            // Construct the output file path
            char output_path[4096];
            sprintf(output_path, "%s/%s", output_dir, in->d_name);

            pid_t pid;
            if ((pid = fork()) < 0) {
                perror("error");
                exit(-1);
            }
            if (pid == 0){ // GRAND CHILD PROCESS to analyze the file with no permissions
                close(pipefd[0]); // close read end of the pipe for grand child process since it has no use in here
                char shell_script[4096];
                strcpy(shell_script, SHELL_SCRIPT_PATH); //path of the shell script program that checks if the file is corrupted
                strcat(shell_script, path); //path of the file to be checked
                int exit_status = system (shell_script); // calling the shell script to check if the file is corrupted
                int command_exit_status = WEXITSTATUS(exit_status); // get the exit status of the shell script
                if (command_exit_status == 1){ // the file is corrupted
                    write (pipefd[1], in->d_name, sizeof  (in->d_name) ); //write the name of the file to the pipe
                }
                else if (command_exit_status == 0){ // the file is not corrupted
                    write (pipefd[1], "SAFE", sizeof ("SAFE")); //write the SAFE keyword to the pipe
                }
                else if (command_exit_status == 2){
                    perror ("File does not exist or is not a regular file\n");
                    write (pipefd[1], "ERROR", sizeof ("ERROR")); //write the error keyword to the pipe
                    exit (EXIT_FAILURE);
                }
                close(pipefd[1]); // close write end of the pipe for the grand child process
                exit(0);
            }
            //sleep (1); // child process sleeps for 1 second before creating the next  grand child  //  ***** NOT NEEDED PROBABLY ******
            // CHILD PROCESS
            close(pipefd[1]); // close write end of the pipe for the child process since it has no use in here
            char pipe_buffer[1000];
            if ( read (pipefd[0], pipe_buffer, sizeof (pipe_buffer)) == -1 ){ // Read the content of the pipe inside the pipe_buffer variable
                perror("read from pipe error \n");
                exit(EXIT_FAILURE);
            }
            close(pipefd[0]); // close read end of the pipe for the child process
            if (strcmp (pipe_buffer, "SAFE") == 0){ //if the file is safe we write the information about the file in the output file inside the output directory
                int output;
                if ((output = open(output_path, O_CREAT | O_WRONLY | O_TRUNC, S_IWGRP | S_IWUSR | S_IRUSR)) < 0) { // open the output file
                    perror("Can't open output file\n");
                    exit(EXIT_FAILURE);
                }
                write(output, "Entry: ", strlen("Entry: "));
                if (write(output, in->d_name, strlen(in->d_name)) == -1) {  // information about the name of the file
                    perror("Error writing to output file\n");
                    exit(EXIT_FAILURE);
                }
                write (output, "\nSize : ", strlen("\nSize : ")); //information about size
                char str_get_something[100];
                sprintf (str_get_something, "%ld", filestat.st_size);
                write (output, str_get_something, strlen (str_get_something));
                write (output, " bytes", strlen (" bytes"));
                write (output, "\nInode number :", strlen ("\nInode number :"));// information about inode number
                sprintf (str_get_something, "%ld", filestat.st_ino);
                write (output, str_get_something, strlen (str_get_something));
                write (output, "\nPermissions: ", strlen ("\nPermissions: ")); // information about permissions
                sprintf (str_get_something, "%o", filestat.st_mode);
                sprintf (str_get_something, str_get_something+3, strlen (str_get_something+3));
                for (int i = 0 ; i < strlen (str_get_something); i++){
                    switch (str_get_something[i]) {
                        case '0':
                            write (output, "---", 3);
                            break;
                        case '1':
                            write(output, "--x", 3);
                            break;
                        case '2':
                            write(output, "-w-", 3);
                            break;
                        case '3':
                            write(output, "-wx", 3);
                            break;
                        case '4':
                            write(output, "r--", 3);
                            break;
                        case '5':
                            write(output, "r-x", 3);
                            break;
                        case '6':
                            write(output, "rw-", 3);
                            break;
                        case '7':
                            write(output, "rwx", 3);
                            break;
                    }
                }
                write(output, "\nLast Modified: ", strlen("\nLast Modified: ")); // Information about last modified time
                char *last_modified_time = ctime(&filestat.st_mtime);
                write(output, last_modified_time, strlen(last_modified_time));
                close(output); // close the output file
            }
            else{
                if (strcmp (pipe_buffer, "ERROR") == 0){ //if the pipe is not working properly
                    perror ("Pipe is not working !\n");
                    exit (EXIT_FAILURE);
                }
                else{// if the file is corrupted
                    (*cfc)++;
                    char isolated_file[4096] = "";
                    strcat(isolated_file, isolated_dir);
                    strcat(isolated_file, "/");
                    strcat(isolated_file, in->d_name);
                    if (rename(path, isolated_file) == 0) { // move the file to the isolated directory
                        printf("Corrupted file moved successfully.\n");
                    } else {
                        perror("Error renaming file");
                        exit(EXIT_FAILURE);
                    }
                }
            } 
        }
        else if (S_ISDIR(filestat.st_mode))  //if the current thing is a directory
            processDirectory(path, output_dir, isolated_dir, cfc); //recursive call to process the directory
    }
    closedir(dir);
}

int main(int argc, char* argv[]) {
    if (argc < 5 || argc > 14) { // Check if the number of arguments is valid
        perror("Invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }
    char *output_dir = NULL;
    char *isolated_dir = NULL;
    for (int i = 1 ; i < argc - 1; i++){ // Check and create the output directory and isolated directory
        if (strcmp(argv[i], "-o") == 0)
            output_dir = strdup (argv[i+1]);
        if (strcmp(argv[i], "-s") == 0)
            isolated_dir = strdup (argv[i+1]);   
    }
    if (output_dir == NULL){ // Check if the output directory is found
        perror ("Output directory not found");
        exit( EXIT_FAILURE);
    }
    if (isolated_dir == NULL){ // Check if the isolated directory is found
        perror ("Izolated directory not found");
        exit (EXIT_FAILURE);
    }
    int pipe_cfc[2];
    if (pipe(pipe_cfc) == -1) { // Create a pipe to communicate the number of corrupted files from the child processes to the parent process
    perror("pipe");
    exit(EXIT_FAILURE);
    }       
    for (int i = 1; i < argc - 1; i++) {  //loop through the directories
        if (strcmp (argv[i], "-o") == 0 || strcmp (argv[i], "-s") == 0) {//if the argument is -o or -s, skip it
            i = i + 2;
            if (i >= argc)
                break;
        } 
        if (strcmp (argv[i], "-o") == 0 || strcmp (argv[i], "-s") == 0){ //if the arguments are right after each other, skip them
            i = i + 2;
            if (i >= argc)
                break;
        } 
        struct stat dirstat;
        if (stat(argv[i], &dirstat) == -1) {
            perror("Bad status...\n");
            exit(EXIT_FAILURE);
        }
        if (!(S_ISDIR(dirstat.st_mode))) {  // Check if the input is a directory
            perror("It's not a directory\n");
            exit(EXIT_FAILURE);
        }

        pid_t pid;
        if ((pid = fork()) < 0) { // Create a process for each directory
            perror("error");
            exit(-1);
        }
        if (pid == 0){ // child process // the function processDirectory () will execute in the child process
            close (pipe_cfc[0]); //close the read end of the pipe in the child process
            int corrupted_files_count = 0;
            processDirectory(argv[i], output_dir, isolated_dir, &corrupted_files_count);
            write(pipe_cfc[1], &corrupted_files_count, sizeof(int)); // Write corrupted_files_count to the pipe
            close(pipe_cfc[1]); // Close the write end of the pipe in the child process
            printf("Snapshot of directory %s has been created\n", argv[i]);
            exit(0);
            }
    }
    //PARENT PROCESS
    sleep (1); // Parent process sleeps for 1 second before creating the next child
    close (pipe_cfc[1]); //close the write end of the pipe in the parent process
    pid_t pid_fiu;
    int status;
    for (int i = 0 ; i < argc-5; i++){       
        pid_fiu = wait(&status);
        if (WIFEXITED (status) > 0){ // Check if the child process has terminated normally
            int danger_files;
            if (read(pipe_cfc[0], &danger_files, sizeof(int)) == -1){ // Read the number of corrupted files from the pipes in the parent process
                perror("read from pipe error.. \n");
                exit(EXIT_FAILURE);
            }
            printf("Child process %d terminated with pid %d, exit code %d and %d files with potential danger.\n", i+1, pid_fiu, WEXITSTATUS(status), danger_files );
        }
    }
    close (pipe_cfc[0]); //close the read end of the pipe in the parent process
    free (output_dir); //free the memory allocated for the output directory
    return 0;
}
