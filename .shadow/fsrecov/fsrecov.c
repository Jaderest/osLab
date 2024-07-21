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

static int is_dir_entry(const fat32dir *entry) { //正确性一般，少数几个出错的好像
  int entry_size = CLUSTER_SIZE / DIR_SIZE;
  // 未使用过
  if (entry->short_entry.DIR_Name[0] == 0x00) {
    return 0;
  }
  // 这里entry[i]自动计算了偏移量
  for (int i = 0; i < entry_size;) {
    if (entry[i].short_entry.DIR_Name[0] == 0xe5) {
      i++; // deleted entry
      continue;
    } else if (entry[i].short_entry.DIR_Name[0]) {
      if (entry[i].short_entry.DIR_NTRes != 0x0 ||
          entry[i].short_entry.DIR_Attr == 0x0) {
        return 0; // 非目录项了
      }
      i++; // 不退出的情况就继续扫
    } else if (entry[i].long_entry.LDIR_Type == ATTR_LONG_NAME) {
      int size = entry[i].long_entry.LDIR_Ord; // 这是长目录项的序号
      if (size & LAST_LONG_ENTRY) {
        size &= ~LAST_LONG_ENTRY;
      }
      // 检查长名目录项是否合规
      if (entry[i].long_entry.LDIR_Type != 0x0f ||
          entry[i].long_entry.LDIR_FstClusLO != 0x0) {
        return 0;
      }
      if (i + size < entry_size && (entry[size].short_entry.DIR_NTRes != 0x0 || entry[size].short_entry.DIR_Name[0] != 0x0)) {
        return 0;
      }
      i += size + 1;
    } else {
      u8 *begin = (u8 *)&entry[i];
      u8 *end = (u8 *)&entry[entry_size];
      for (u8 *p = begin; p < end; p++) {
        if (*p != 0x0) {
          return 0;
        }
      }
      break;
    }
  }
  return 1;
}

//TODO: 从目录项中提取文件名
//TODO：根据clusterid找到文件内容，然后写入文件
//TODO：需要验证这个算出来的clusterid是否正确（readfat已经验证过了）

int main(int argc, char *argv[]) {
  size_t image_size;
  void *disk_image =
      map_disk_image(argv[1], &image_size); // 映射文件+获取文件大小
  struct fat32hdr *hdr = (struct fat32hdr *)disk_image;
  assert(hdr->Signature_word == 0xaa55);
  printf("Volume ID: %u\n", hdr->BS_VolID);
  printf("img size: %zu\n", image_size);
  printf("Bytes per sector: %u\n", hdr->BPB_BytsPerSec);
  printf("Sectors per cluster: %u\n", hdr->BPB_SecPerClus);
  printf("Reserved sectors: %u\n", hdr->BPB_RsvdSecCnt);
  printf("Number of FATs: %u\n", hdr->BPB_NumFATs);
  printf("Root entries: %u\n", hdr->BPB_RootEntCnt);
  printf("Root cluster: %u\n", hdr->BPB_RootClus);
  printf("FAT size: %u\n", hdr->BPB_FATSz32);

  u32 firstDataSecOff = (hdr->BPB_RsvdSecCnt + (hdr->BPB_NumFATs * hdr->BPB_FATSz32)) * hdr->BPB_BytsPerSec;
  u8 *firstDataSec = (u8 *)disk_image + firstDataSecOff;

  char dir_tmp[] = "/tmp/fsrecov_XXXXXX";
  if (mkdtemp(dir_tmp) == NULL) {
    perror("mkdtemp");
    exit(EXIT_FAILURE);
  }
  debug("directory: %s\n", dir_tmp);

  // 接下来就是遍历每个cluster，找到目录项，然后把可能的bmp文件恢复出来（加上文件名）
  return 0;
}
