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

void scan_clusters(const void *disk_image, size_t image_size, size_t cluster_size) {
  // 其实这里不止这个，还要恢复文件名，所以扫描clusters这里还要分类
  /**
   * 目录文件，以此恢复文件名（注意还得将它们文件名转化称ascii）
   * BMP文件头部
   * BMP实际数据
   * 未使用的cluster
   */
  size_t cluster_count = image_size / cluster_size;

  for (size_t cluster_num = 0; cluster_num < cluster_count; cluster_count++) {
    void *cluster_data = (u8 *)disk_image + cluster_num * cluster_size;
    struct BmpHeader *bmp_hdr = (struct BmpHeader *)cluster_data;
    if (bmp_hdr->bfType == 0x4d42) { // "BM"
      // TODO：恢复文件，文件名，然后调用sha1sum
    }
  }
}

int main(int argc, char *argv[]) {
  size_t image_size;
  void *disk_image = map_disk_image(argv[1], &image_size); //映射文件+获取文件大小
  struct fat32hdr *hdr = (struct fat32hdr *)disk_image;
  assert(hdr->Signature_word == 0xaa55);
  debug("Volume ID: %u\n", hdr->BS_VolID);
  debug("Volume Label: %s\n", hdr->BS_VolLab);
  debug("File System Type: %s\n", hdr->BS_FilSysType);
  debug("img size: %ld\n", image_size);
  // size_t cluster_size = CLUSTER_SIZE;
  // scan_clusters(disk_image, image_size, cluster_size);
  // munmap(disk_image, image_size);

  void *cluster_addr = (void *)((uintptr_t)hdr + (hdr->BPB_RsvdSecCnt + hdr->BPB_NumFATs * hdr->BPB_TotSec32 + hdr->BPB_HiddSec) * hdr->BPB_BytsPerSec);
  uintptr_t addr = (uintptr_t)cluster_addr; // cluster_addr是void*类型，不能直接加减

  

  return 0;
}
