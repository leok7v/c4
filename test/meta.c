#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

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
    // O_WRONLY | O_CREAT | O_TRUNC, mode 0644 = 420
    // Linux:  1 | 64  | 512  = 577
    // macOS:  1 | 512 | 1024 = 1537
    fd = open(filename, 577, 420);
    if (fd < 0) fd = open(filename, 1537, 420);
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
