#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h> 
#include <unistd.h>

void processDirectory( char *dir_path, int output, int i) {
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
            sprintf(buffer, "Directory %d, nume: %s\n", i, path);
            if (write(output, buffer, strlen(buffer)) == -1) {
                perror("Error writing to output file\n");
                exit(EXIT_FAILURE);
            }
        } 
        else if (S_ISDIR(filestat.st_mode)) {
            processDirectory(path, output, i);
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
    
    int output;
    
    if ((output = open(argv[argc - 1], O_CREAT | O_WRONLY | O_TRUNC, S_IWGRP | S_IWUSR | S_IXGRP | S_IRUSR)) < 0) {
        perror("Can't open output file\n");
        exit(EXIT_FAILURE);
    }
    
    write(output, "hello world\n", strlen("hello world\n"));
    
    for (int i = 1; i < argc - 1; i++) {
        struct stat dirstat;
        if (stat(argv[i], &dirstat) == -1) {
            perror("Bad status\n");
            exit(EXIT_FAILURE);
        }
        
        if (!(S_ISDIR(dirstat.st_mode))) {
            perror("It's not a directory\n");
            exit(EXIT_FAILURE);
        }
        
        processDirectory(argv[i], output, i);
    }
    
    close(output);
    
    return 0;
}

