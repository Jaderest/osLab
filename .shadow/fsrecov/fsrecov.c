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

// void *map_disk_image(const char *path, size_t *size) {
//   int fd = open(path, O_RDONLY);
//   if (fd < 0) {
//     perror("open");
//     exit(EXIT_FAILURE);
//   }
//   *size = lseek(fd, 0, SEEK_END);
//   void *disk_image = mmap(NULL, *size, PROT_READ, MAP_PRIVATE, fd, 0);
//   if (disk_image == MAP_FAILED) {
//     perror("mmap");
//     return NULL;
//   }
//   close(fd);
//   return disk_image;
// }

int main(int argc, char argv[]) {
  return 0;
}