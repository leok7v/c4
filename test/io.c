
int main() {
  int fd;
  char *buf;
  int n;
  char *filename;

  filename = "build/test_io_output.txt";

  // Test 1: File Write (Create + Write)
  printf("--- Testing File I/O ---\n");
  // O_CREAT (512) | O_TRUNC (1024) | O_WRONLY (1) = 1537
  // Mode 0644 = 420
  fd = open(filename, 1537, 420); 
  if (fd < 0) { printf("Failed to open file for write\n"); return -1; }
  
  n = write(fd, "File content test\n", 18);
  if (n != 18) { printf("Error: Short write\n"); }
  else { printf("Successfully wrote to file\n"); }
  close(fd);

  // Test 2: File Read
  fd = open(filename, 0); // O_RDONLY
  if (fd < 0) { printf("Failed to open file for read\n"); return -1; }
  
  buf = malloc(100);
  memset(buf, 0, 100);
  n = read(fd, buf, 100);
  close(fd);
  
  printf("Read from file: %s", buf);
  
  // Test 3: Stdout Write (using file descriptor 1)
  printf("\n--- Testing Stdout Write ---\n");
  write(1, "Stdout write works!\n", 20);

  return 0;
}
