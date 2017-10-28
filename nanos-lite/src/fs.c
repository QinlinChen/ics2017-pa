#include "fs.h"

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
  off_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void ramdisk_read(void *buf, off_t offset, size_t len);
void ramdisk_write(const void *buf, off_t offset, size_t len);
void dispinfo_read(void *buf, off_t offset, size_t len);
void fb_write(const void *buf, off_t offset, size_t len);
size_t events_read(void *buf, size_t len);

void init_fs() {
  // initialize the size of /dev/fb
  file_table[FD_FB].size = _screen.width * _screen.height * sizeof(uint32_t);
}

int fs_open(const char *pathname, int flags, int mode) {
  int fd;
  for (fd = 0; fd < NR_FILES; ++fd) 
    if (strcmp(file_table[fd].name, pathname) == 0) {
      file_table[fd].open_offset = 0;
      return fd;
    }
  assert(0);
  return -1;
}

ssize_t fs_read(int fd, void *buf, size_t len) {
  assert(fd != FD_STDOUT && fd != FD_STDERR && fd != FD_STDIN);   // cases to ignore
  assert(fd < NR_FILES);
  
  off_t fd_open_offset, offset;
  size_t fd_size;

  if (fd == FD_EVENTS) 
    return events_read(buf, len);
  
  fd_open_offset = file_table[fd].open_offset;
  fd_size = file_table[fd].size;
  
  if (fd_open_offset >= fd_size)
    return 0;
  if (fd_open_offset + len > fd_size)
    len = fd_size - fd_open_offset;
  offset = file_table[fd].disk_offset + fd_open_offset;
  
  if (fd == FD_DISPINFO)
    dispinfo_read(buf, offset, len);
  else
    ramdisk_read(buf, offset, len);
    
  file_table[fd].open_offset += len;
  return len;
}

ssize_t fs_write(int fd, const void *buf, size_t len) {
  assert(fd != FD_STDIN);
  assert(fd < NR_FILES);
  
  int i;
  off_t fd_open_offset, offset;
  size_t fd_size;
  switch (fd) {
    case FD_STDOUT:
    case FD_STDERR:
      for (i = 0; i < len; ++i)
        _putc(((char *)buf)[i]);
      return i;
      
    default:
      fd_open_offset = file_table[fd].open_offset;
      fd_size = file_table[fd].size;
  
      if (fd_open_offset >= fd_size)
        return 0;
      if (fd_open_offset + len > fd_size)
        len = fd_size - fd_open_offset;
      offset = file_table[fd].disk_offset + fd_open_offset;
      if (fd == FD_FB) 
        fb_write(buf, offset, len);
      else
        ramdisk_write(buf, offset, len);
      file_table[fd].open_offset += len;
      return len;
  }  
}

off_t fs_lseek(int fd, off_t offset, int whence) {
  assert(fd < NR_FILES);
  switch (whence) {
    case SEEK_SET: file_table[fd].open_offset = offset; break;
    case SEEK_CUR: file_table[fd].open_offset += offset; break;
    case SEEK_END: file_table[fd].open_offset = file_table[fd].size + offset; break;
    default: return -1;
  }
  return file_table[fd].open_offset;
}

int fs_close(int fd) {
  return 0;
}

size_t fs_filesz(int fd) {
  return file_table[fd].size;
}

