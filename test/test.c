#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int run_test(char *filename) {
    char *cmd;
    int status;
    
    cmd = malloc(256);
    strcpy(cmd, "./build/c4 ");
    strcat(cmd, filename);
    
    printf("\n=== Running Test: %s ===\n", filename);
    status = system(cmd);
    
    // Status handling needs to be robust. 
    // system() usually returns exit status in high byte
    // But failing system() (e.g. command not found) returns non-zero too.
    // Let's assume non-zero status means failure for now, 
    // unless expected otherwise.
    
    printf("Result: %d\n", status);
    free(cmd);
    
    if (status != 0) {
        // Special case: meta.c returns 42 on success... wait, meta.c returns 0 from main, 
        // buy runs a command that returns 42. So main of meta.c returns 0.
        // system.c returns 0.
        // io.c returns 0.
        // args.c returns 0.
        // If c4 itself fails (segfault, error), it returns -1 or signal.
        return 0; // Fail
    }
    
    return 1; // Success
}

int main(int argc, char **argv) {
    int passed;
    int total;
    int fp;
    char *buf;
    char *ptr;
    char *filename;
    int n;
    int i;
    int len;
    int is_self;
    char *end;
    
    passed = 0;
    total = 0;

    if (argc > 1) {
        i = 1;
        while (i < argc) {
            total++;
            if (run_test(argv[i])) passed++;
            i++;
        }
    } else {
        // Auto-discover tests
        printf("Discovering tests in test/ directory...\n");
        fp = popen("ls test/*.c", "r");
        if (!fp) {
            printf("Failed to list tests\n");
            return -1;
        }
        
        buf = malloc(1024);
        memset(buf, 0, 1024);
        // Read file list. It might be large, doing simple read for now.
        n = fread(buf, 1, 1023, fp);
        pclose(fp);
        
        if (n <= 0) {
            printf("No tests found\n");
            return 0;
        }
        
        // Parse filenames (separated by newline)
        ptr = buf;
        filename = buf;
        while (*ptr) {
            if (*ptr == '\n') {
                *ptr = 0; // Null-terminate string
                
                // Check if it is test.c (ourselves)
                // We need to compare filename (paths could vary: "test/test.c")
                // Simple check for ending in "test.c"
                // ... implementing strstr or similar would be good, but lazy checking:

                len = strlen(filename);
                is_self = 0;
                if (len >= 6) {
                    // Check for "test.c" at end
                    end = filename + len;
                    end = end - 6;
                    
                    if (strcmp(end, "test.c") == 0) {
                        // Further check to avoid matching "mytest.c"
                        if (len == 6) is_self = 1;
                        else if (*(end-1) == '/') is_self = 1;
                    }
                }
                
                if (!is_self && len > 0) {
                    total++;
                    if (run_test(filename)) passed++;
                }
                
                filename = ptr + 1;
            }
            ptr++;
        }
    }
    
    printf("\nExpected failures notes:\n");
    printf("- meta.c prints 'System returned status: 10752' (exit 42) which is correct behavior for that test.\n");
    
    printf("\nSUMMARY: %d/%d tests passed.\n", passed, total);
    
    if (passed == total) return 0;
    return -1;
}
