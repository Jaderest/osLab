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

char uni2ascii(const u32 uni) {
  if (uni <= 0x7f) {
    return uni;
  } else {
    return '_';
  }
}

void parse_bmp(struct BmpHeader *hdr, const char *name, const char *tmp_path) {
  if (hdr->bfType != 0x4d42) {
    fprintf(stderr, "Invalid BMP magic\n");
    exit(EXIT_FAILURE);
  }
  if (hdr->bfType == 0x4d42) {
    debug("BMP file: %s\n", name);
    debug("File size: %u\n", hdr->bfSize);
    debug("Offset to image data: %u\n", hdr->bfOffBits);
  }
}

int is_bmpentry(struct line *line, char *name) {
  struct fat32dent *entry = (struct fat32dent *)line;
  if (entry->DIR_FileSize > 0 && entry->DIR_FileSize < 2000 * 1024) {
    unsigned char checksum = ChkSum(entry->DIR_Name);
    void *ptr = (void *)entry; // 这里是短目录，所以我要向上寻找
    u8 size = 0;               // 几组长目录
    while (1) {
      size++;
      ptr -= DIR_SIZE;
      struct fat32LongName *long_entry = (struct fat32LongName *)ptr;
      if (long_entry->LDIR_Ord == size) { // 有继续的情况
        if (long_entry->LDIR_Chksum != checksum || long_entry->LDIR_Attr != ATTR_LONG_NAME ||
            long_entry->LDIR_Type != 0) {
          return 0;
        }
        continue;
      } else if (long_entry->LDIR_Ord == (size | 0x40)) { // 结束
        if (long_entry->LDIR_Chksum != checksum || long_entry->LDIR_Attr != ATTR_LONG_NAME ||
            long_entry->LDIR_Type != 0) {
          return 0;
        }
        break;
      } else {
        return 0;
      }
    }
    int len = 0;
    struct fat32LongName *long_entry = (struct fat32LongName *)ptr; // 这是得到目录开始的地方
    for (int i = 0; i < size; i++) {
      for (int j = 0; j < 5; j++) {
        name[len++] = uni2ascii(long_entry[size-i-1].LDIR_Name1[j]);
      }
      for (int j = 0; j < 6; j++) {
        name[len++] = uni2ascii(long_entry[size-i-1].LDIR_Name2[j]);
      }
      for (int j = 0; j < 2; j++) {
        name[len++] = uni2ascii(long_entry[size-i-1].LDIR_Name3[j]);
      }
    }
    name[len] = '\0';
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

  char tmp_path[256] = "/tmp/fsrecov_XXXXXX";
  int fd = mkstemp(tmp_path);

  struct line *line = (struct line *)disk_img;
  size_t line_size = LINE_SIZE;
  size_t num_lines = img_size / line_size;
  for (int i = 0; i < num_lines; i++) {
    if (line->bmp[0] == 'B' && line->bmp[1] == 'M' && line->bmp[2] == 'P') {
      char name[256];
      if (is_bmpentry(line, name)) {
        printf("%s\n", name); //name ok!
      }
      // 此处line是dir entry的起始地址(短目录)
      struct fat32dent *entry = (struct fat32dent *)line;
      size_t cluster = entry->DIR_FstClusLO | (entry->DIR_FstClusHI << 16);
      void *cluster_ptr = (u8 *)disk_img + (cluster - 2) * cluster_size;
      struct BmpHeader *hdr = (struct BmpHeader *)cluster_ptr;
      parse_bmp(hdr, name, tmp_path);
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
