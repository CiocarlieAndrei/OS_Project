#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h> 
#include <unistd.h>
#include <time.h>

void processDirectory(char *dir_path, char *output_dir, char *isolated_dir, int i) {
    DIR *dir;
    struct dirent *in;
    struct stat filestat;
    char path[4096];
    char buffer[4096];

    if ((dir = opendir(dir_path)) == NULL) {
        perror("Can't open directory\n");
        exit(EXIT_FAILURE);
    }

    while ((in = readdir(dir)) != NULL) {
        if (strcmp(in->d_name, ".") == 0 || strcmp(in->d_name, "..") == 0) {
            continue;
        }

        sprintf(path, "%s/%s", dir_path, in->d_name);

        if (stat(path, &filestat) == -1) {
            perror("Error getting file status\n");
            exit(EXIT_FAILURE);
        }

        if (S_ISREG(filestat.st_mode)) {
            // Construct the output file path
            char output_path[4096];
            sprintf(output_path, "%s/%s", output_dir, in->d_name);

            char octal_string[100];//convert octal to string to check file permissions
            sprintf (octal_string, "%o", filestat.st_mode);
            sprintf (octal_string, octal_string+3, strlen (octal_string+3));

            if (strcmp (octal_string, "000") == 0){ //check if the file has no permissions
               pid_t pid;
                if ((pid = fork()) < 0) {
                    perror("error");
                    exit(-1);
                }
                if (pid == 0){ // child process to analyze the file with no permissions
                    printf ("File %s has no permissions\n", in->d_name);
                    char shell_script[4096];
                    strcpy(shell_script, "/home/andrei/C_Programms/OS_Project/check_file.sh ");
                    strcat(shell_script, path);
                    int exit_status = system (shell_script); // calling the shell script to check if the file is corrupted
                    int command_exit_status = WEXITSTATUS(exit_status);
                    if (command_exit_status == 1){ // the file is corrupted
                        printf ("Fisier corupt \n");
                        char isolated_file[4096] = "";
                        strcat(isolated_file, isolated_dir);
                        strcat(isolated_file, "/");
                        strcat(isolated_file, in->d_name);
                        if (rename(path, isolated_file) == 0) { // move the file to the isolated directory
                            printf("File moved successfully.\n");
                        } else {
                            perror("Error renaming file");
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (command_exit_status == 0){ // the file is not corrupted
                        printf ("Fisierul nu este corupt\n");
                    }
                    else if (command_exit_status == 2){
                        perror ("File does not exist or is not a regular file\n");
                        exit (EXIT_FAILURE);
                    }
                    exit(0);
                }
                sleep (1); // Parent process sleeps for 1 second before creating the next child
            }

            // Open the output file
            int output;
            if ((output = open(output_path, O_CREAT | O_WRONLY | O_TRUNC, S_IWGRP | S_IWUSR | S_IRUSR)) < 0) {
                perror("Can't open output file\n");
                exit(EXIT_FAILURE);
            }
            write(output, "Entry: ", strlen("Entry: "));

            if (write(output, in->d_name, strlen(in->d_name)) == -1) {
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

            close(output);
        } else if (S_ISDIR(filestat.st_mode)) {
            processDirectory(path, output_dir, isolated_dir, i);
        }
    }

    closedir(dir);
}


int main(int argc, char* argv[]) {
    printf("Hello here!\n");
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
    if (output_dir == NULL){
        perror ("Output directory not found");
        exit( EXIT_FAILURE);
    }
    if (isolated_dir == NULL){
        perror ("Izolated directory not found");
        exit (EXIT_FAILURE);
    }
        
    for (int i = 1; i < argc - 1; i++) { 
        if (strcmp (argv[i], "-o") == 0 || strcmp (argv[i], "-s") == 0)
            i = i + 2;
        else{
            struct stat dirstat;
            if (stat(argv[i], &dirstat) == -1) {
                perror("Bad status...\n");
                exit(EXIT_FAILURE);
            }
            if (!(S_ISDIR(dirstat.st_mode))) {
                perror("It's not a directory\n");
                exit(EXIT_FAILURE);
            }


            pid_t pid;
            if ((pid = fork()) < 0) {
                perror("error");
                exit(-1);
            }
            if (pid == 0){
                processDirectory(argv[i], output_dir, isolated_dir, i);
                printf("Snapshot of directory %s has been created\n", argv[i]);
                exit(0);
            }
        }
    }

    sleep (1); // Parent process sleeps for 1 second before creating the next child
    pid_t pid_fiu;
    int status;
    for (int i = 0 ; i < argc-5; i++){        
        pid_fiu = wait(&status);
        if (WIFEXITED (status) > 0){
            printf("Child process %d terminated with pid %d and exit code %d\n", i+1, pid_fiu, WEXITSTATUS(status));
        }
    }

    free (output_dir);
    
    return 0;
}

