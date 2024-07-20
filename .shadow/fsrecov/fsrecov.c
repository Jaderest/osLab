#include "fat32.h"
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
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

int main(int argc, char *argv[]) {
  size_t image_size;
  void *disk_image = map_disk_image(argv[1], &image_size); //映射文件+获取文件大小
  struct fat32hdr *hdr = (struct fat32hdr *)disk_image;
  assert(hdr->Signature_word == 0xaa55);
  debug("Volume ID: %u\n", hdr->BS_VolID);
  debug("img size: %zu\n", image_size);
  debug("Bytes per sector: %u\n", hdr->BPB_BytsPerSec);
  debug("Sectors per cluster: %u\n", hdr->BPB_SecPerClus);
  debug("Reserved sectors: %u\n", hdr->BPB_RsvdSecCnt);
  debug("Number of FATs: %u\n", hdr->BPB_NumFATs);
  debug("Root entries: %u\n", hdr->BPB_RootEntCnt);
  debug("Root cluster: %u\n", hdr->BPB_RootClus);

  // 接下来就是遍历每个cluster，找到目录项，然后把可能的bmp文件恢复出来（加上文件名）
  return 0;
}
