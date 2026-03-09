#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"


int CountKey(char *path, char key) {
    int count = 0;
    while(*path != '\0') {
        if(*path == key) count++;
        path++;
    }
    return count;
}


char* fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  buf[sizeof(buf)-1] = '\0';
  return buf;
}

void ls(char *path, char key, int *d_count, int *f_count)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, O_RDONLY)) < 0){
    printf("%s [error opening dir]\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    printf("%s [error opening dir]\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_DEVICE:
  case T_FILE:
    printf("%s [error opening dir]\n", path);
    close(fd);
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      close(fd);
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        continue;
      }
    if (st.type == T_DIR) {
        printf("%s %d\n", buf, CountKey(buf, key));
        (*d_count)++;
        ls(buf, key, d_count, f_count);
    } else if (st.type == T_FILE) {
        (*f_count)++;
        printf("%s %d\n", buf, CountKey(buf, key));
    }
    break;
  }
  }
  close(fd);
}


void mp0(char *root, char key)
{
    // TODO: implement mp0
    // Hint: Use pipe, fork, and walk
    int p[2];
    pipe(p);
    int pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
        exit(1);
    }

    if(pid == 0) {
        close(p[0]);
        int d_count = 0;
        int f_count = 0;
        printf("%s %d\n", root, CountKey(root, key));
        ls(root, key, &d_count, &f_count);
        int results[2] = {d_count, f_count};
        write(p[1], results, sizeof(results));
        close(p[1]);
        exit(0);
    }
    else {
        close(p[1]);
        int results[2];
        wait(0);
        read(p[0], results, sizeof(results));
        printf("\n%d directories, %d files\n", results[0], results[1]);
        close(p[0]);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage: mp0 <root_directory> <key>\n");
        exit(1);
    }
    mp0(argv[1], *argv[2]);
    exit(0);
}
