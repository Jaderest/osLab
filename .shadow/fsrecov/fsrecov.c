#include "fat32.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define SECTOR_SIZE 512
#define CLUSTER_SIZE 4096 // 8 sectors
#define DIR_SIZE (sizeof(fat32dir))

#define DEBUG
#ifdef DEBUG
#define debug(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

struct line {
  u8 blank[8];
  char bmp[3];
  u8 blank2[5];
} __attribute__((packed));

#define LINE_SIZE (sizeof(struct line))

void *map_disk_image(const char *path, size_t *size);
// void scan_clusters(void *disk_img, size_t img_size, size_t cluster_size);
unsigned char ChkSum(unsigned char *pFcbName) {
  short FcbNameLen;
  unsigned char Sum;
  Sum = 0;
  for (FcbNameLen = 11; FcbNameLen != 0; FcbNameLen--) {
    // NOTE: The operation is an unsigned char rotate right
    Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *pFcbName++;
  }
  return (Sum);
}
int is_bmpentry(struct line *line, char *name) {
  // TODO：判断是否是bmp文件，即往上继续检测long entry
  struct fat32dent *entry = (struct fat32dent *)line;
  if (entry->DIR_FileSize > 0 && entry->DIR_FileSize < 2000 * 1024) {
    // TODO：检测long entry
    unsigned char checksum = ChkSum(entry->DIR_Name);
    void *ptr = (void *)entry; // 这里是短目录，所以我要向上寻找
    u8 size = 0;               // 几组长目录
    ptr -= DIR_SIZE;
    struct fat32LongName *long_entry = (struct fat32LongName *)ptr; // 只往前一格看看
    if (long_entry->LDIR_Ord == 0x01 && long_entry->LDIR_Attr == ATTR_LONG_NAME
        && long_entry->LDIR_Chksum == checksum && long_entry->LDIR_Type == 0x00) {
      size++;
    }
    debug("size: %d\n", size);
  }
  return entry->DIR_FileSize > 0 && entry->DIR_FileSize < 2000 * 1024;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <disk_image>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  size_t img_size;
  void *disk_img = map_disk_image(argv[1], &img_size);
  size_t cluster_size = CLUSTER_SIZE;

  struct fat32hdr *hdr = (struct fat32hdr *)disk_img;

  struct line *line = (struct line *)disk_img;
  size_t line_size = LINE_SIZE;
  size_t num_lines = img_size / line_size;
  for (int i = 0; i < num_lines; i++) {
    if (line->bmp[0] == 'B' && line->bmp[1] == 'M' && line->bmp[2] == 'P') {
      char name[256];
      if (is_bmpentry(line, name)) {
        // printf("%s\n", name);
      }
    }
    line++;
  }

  return 0;
}

void *map_disk_image(const char *path, size_t *size) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    perror("open");
    exit(EXIT_FAILURE);
  }
  *size = lseek(fd, 0, SEEK_END);
  void *disk_image = mmap(NULL, *size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (disk_image == MAP_FAILED) {
    perror("mmap");
    return NULL;
  }
  close(fd);
  return disk_image;
}
