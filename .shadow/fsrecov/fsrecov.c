#include "fat32.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

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
unsigned char ChkSum (unsigned char *pFcbName) { 
  short FcbNameLen; 
  unsigned char Sum; 
  Sum = 0; 
  for (FcbNameLen=11; FcbNameLen!=0; FcbNameLen--) { 
  // NOTE: The operation is an unsigned char rotate right 
    Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *pFcbName++; 
  } 
  return (Sum); 
}
int is_bmpentry(struct line *line, char *name) {
  //TODO：判断是否是bmp文件，即往上继续检测long entry
  struct fat32dent *entry = (struct fat32dent *)((void *)line - LINE_SIZE);
  unsigned char checksum = ChkSum(entry->DIR_Name);
  debug("check sum = %c\n", checksum);
  return 1;
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
  debug("BPB_BytsPerSec: %d\n", hdr->BPB_BytsPerSec);
  debug("BPB_SecPerClus: %d\n", hdr->BPB_SecPerClus);
  debug("BPB_RsvdSecCnt: %d\n", hdr->BPB_RsvdSecCnt);
  debug("BPB_NumFATs: %d\n", hdr->BPB_NumFATs);
  debug("BPB_FATSz32: %d\n", hdr->BPB_FATSz32);

  struct line *line = (struct line *)disk_img;
  size_t line_size = LINE_SIZE;
  size_t num_lines = img_size / line_size;
  for (int i = 0; i < num_lines; i++) {
    if (line->bmp[0] == 'B' && line->bmp[1] == 'M' && line->bmp[2] == 'P') {
      char name[256]; // 如果是bmp文件，那么就是文件名，这里我顺便处理了，然后如果可以的话就if内把bmp恢复出来，然后sha1sum
      if(is_bmpentry(line, name)) {
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
 

// 两个选择，创造数据结构存储名字(这里存储)，或者根据目录项恢复文件并且输出sha1sum



// void scan(void *firstDataSec, size_t img_size) {
//   struct line *line = (struct line *)firstDataSec;
//   int line_size = LINE_SIZE;

// }

// void scan_clusters(void *disk_img, size_t img_size, size_t cluster_size) {
//   size_t num_clusters = img_size / cluster_size; // fat文件的总簇数，这样的话我的first是不是可以不要了

//   for (size_t i = 0; i < num_clusters; i++) {
//     void *cluster = disk_img + i * cluster_size;
    
//   }
// }
