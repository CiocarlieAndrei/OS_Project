#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h> 
#include <unistd.h>
#include <time.h>

void processDirectory(char *dir_path, char *output_dir, int i) {
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
            //printf  ("%s\n", output_path);//test

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
            processDirectory(path, output_dir, i);
        }
    }

    closedir(dir);
}


int main(int argc, char* argv[]) {
    printf("Hello there!\n");
    if (argc < 3 || argc > 12) { 
        perror("Invalid number of arguments\n");
        exit(EXIT_FAILURE);
    }

    char *output_dir = NULL;

    for (int i = 1 ; i < argc - 1; i++){
        if (strcmp(argv[i], "-o") == 0)
            output_dir = strdup (argv[i+1]);
    }
    if (output_dir == NULL){
        perror ("Output directory not found");
        exit( EXIT_FAILURE);
    }
    
/*    
    if ((output = open(argv[argc - 1], O_CREAT | O_WRONLY | O_TRUNC, S_IWGRP | S_IWUSR | S_IXGRP | S_IRUSR)) < 0) {
        perror("Can't open output file\n");
        exit(EXIT_FAILURE);
    }
*/
    
    //write(output, "hello world\n", strlen("hello world\n"));
    
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp (argv[i], "-o") == 0)
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
            processDirectory(argv[i], output_dir, i);
        }
    }

    free (output_dir);
    
    return 0;
}

