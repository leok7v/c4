#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int strlen(char *s) {
    int n;
    n = 0;
    while (*s) {
        n++;
        s++;
    }
    return n;
}

int main() {
    int fd;
    char *code;
    char *filename;
    int status;
    char *cmd;
    int len;
    
    filename = "build/generated_hello.c";
    code = "int main() { printf(\"Hello from generated code!\\n\"); return 42; }";
    
    printf("--- 1. Writing generated code to %s ---\n", filename);
    fd = open(filename, 1537, 420); // O_CREAT|O_TRUNC|O_WRONLY, 0644
    if (fd < 0) {
        printf("Failed to create file\n");
        return -1;
    }
    
    len = strlen(code);
    printf("Code length: %d\n", len);
    
    write(fd, code, len);
    close(fd);
    
    printf("--- 2. Compiling and running with ./build/c4 ---\n");
    cmd = "./build/c4 build/generated_hello.c";
    printf("Command: %s\n", cmd);
    
    status = system(cmd);
    printf("System returned status: %d\n", status);
    
    return 0;
}
