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

void scan_clusters(const void *disk_image, size_t image_size,
                   size_t cluster_size) {
  size_t cluster_count = image_size / cluster_size;

  for (size_t cluster_num = 0; cluster_num < cluster_count; cluster_count++) {
    void *cluster_data = (u8 *)disk_image + cluster_num * cluster_size;
    struct BmpHeader *bmp_hdr = (struct BmpHeader *)cluster_data;
    if (bmp_hdr->bfType == 0x4d42) { // "BM"
      // TODO：恢复文件，然后调用sha1sum
    }
  }
}

int main(int argc, char *argv[]) {
  size_t image_size;
  void *disk_image = map_disk_image(argv[1], &image_size);
  size_t cluster_size = CLUSTER_SIZE;
  scan_clusters(disk_image, image_size, cluster_size);
  munmap(disk_image, image_size);
  return 0;
}
