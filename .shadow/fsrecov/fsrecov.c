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

static int
is_dir_entry(const fat32dir *entry) { // 正确性一般，少数几个出错的好像
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
      if (i + size < entry_size &&
          (entry[size].short_entry.DIR_NTRes != 0x0 ||
           entry[size].short_entry.DIR_Name[0] != 0x0)) {
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

// TODO: 从目录项中提取文件名
// TODO：根据clusterid找到文件内容，然后写入文件
// TODO：需要验证这个算出来的clusterid是否正确（readfat已经验证过了）
static const fat32dir *parse_dir_entry(const fat32dir *entry, char *name,
                                       u8 *attr, uint64_t *clus, size_t *size) {
  assert(entry != NULL && name != NULL && attr != NULL && clus != NULL &&
         size != NULL);
  if (entry->short_entry.DIR_Name[0] == 0xe5) {
    *attr = ATTR_NULL;
    return &entry[1];
  } else if (entry->short_entry.DIR_Name[0] == 0x00) {
    *attr = ATTR_NULL;
    return NULL;
  } else if (entry->short_entry.DIR_Attr == ATTR_LONG_NAME) { // 长目录项
    if (entry->long_entry.LDIR_Ord < 0x40) {                  // 说明它跨簇
      attr = ATTR_NULL;
      return &entry[entry->long_entry.LDIR_Ord + 1];
    }

    int size = entry->long_entry.LDIR_Ord & ~LAST_LONG_ENTRY; // 恢复size
    for (int i = 1; i < size; ++i) {
      if (entry[i].long_entry.LDIR_Ord != size - i ||
          entry[i].long_entry.LDIR_Type != ATTR_LONG_NAME ||
          entry[i].long_entry.LDIR_FstClusLO != 0x0 ||
          entry[i].long_entry.LDIR_Type != 0x0f) {
        attr = ATTR_NULL;
        return NULL;
      }
    }
    if (entry[size].short_entry.DIR_Name[0] == LAST_LONG_ENTRY ||
        entry[size].short_entry.DIR_NTRes != 0x0) {
      attr = ATTR_NULL;
      return NULL;
    }

    // TODO: 拼接长目录项的名称(unicode)，小心0xffff的填充
    int len = 0;
    for (int i = 0; i < size; ++i) {
      int flag = 1;
      for (int j = 0; j < 10 && flag; j += 2) {
        if (entry[size - 1 - i].long_entry.LDIR_Name1[j] == 0xffff) {
          flag = 0;
          break;
        }
        name[len++] = entry[size - 1 - i].long_entry.LDIR_Name1[j];
      }
      for (int j = 0; j < 12 && flag; j += 2) {
        if (entry[size - 1 - i].long_entry.LDIR_Name2[j] == 0xffff) {
          flag = 0;
          break;
        }
        name[len++] = entry[size - 1 - i].long_entry.LDIR_Name2[j];
      }
      for (int j = 0; j < 4 && flag; j += 2) {
        if (entry[size - 1 - i].long_entry.LDIR_Name3[j] == 0xffff) {
          break;
        }
        name[len++] = entry[size - 1 - i].long_entry.LDIR_Name3[j];
      }
    }
    name[len] = '\0';

    if (entry[size].short_entry.DIR_Attr == ATTR_DIRECTORY) {
      *attr = ATTR_DIRECTORY;
    } else {
      *attr = ATTR_ARCHIVE;
    }
    *clus = entry[size].short_entry.DIR_FstClusLO |
            (((u_int64_t)entry[size].short_entry.DIR_FstClusHI) << 16);
    size = entry->short_entry.DIR_FileSize;

    if (entry[size + 1].short_entry.DIR_Name[0] == 0)
      return NULL;

    return &entry[size + 1];

  } else { // 短目录项
    int len = 0;
    for (int i = 0; i < sizeof(entry->short_entry.DIR_Name); i++) {
      if (entry->short_entry.DIR_Name[i] == ' ') {
        break;
      }
      name[len++] = entry->short_entry.DIR_Name[i];
    }
    name[len] = '\0';
    if (entry->short_entry.DIR_Attr == ATTR_DIRECTORY) {
      *attr = ATTR_DIRECTORY;
    } else {
      *attr = ATTR_ARCHIVE;
    }
    *clus = entry->short_entry.DIR_FstClusLO |
            (entry->short_entry.DIR_FstClusHI << 16);
    *size = entry->short_entry.DIR_FileSize;

    if (entry[1].short_entry.DIR_Name[0] == 0x00) {
      return NULL;
    } else {
      return &entry[1];
    }
  }
}

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

  u32 firstDataSecOff =
      (hdr->BPB_RsvdSecCnt + (hdr->BPB_NumFATs * hdr->BPB_FATSz32)) *
      hdr->BPB_BytsPerSec;
  u8 *firstDataSec = (u8 *)disk_image + firstDataSecOff;
  u8 *endDataSec = (u8 *)disk_image + image_size;

  char dir_tmp[] = "/tmp/fsrecov_XXXXXX";
  if (mkdtemp(dir_tmp) == NULL) {
    perror("mkdtemp");
    exit(EXIT_FAILURE);
  }
  debug("directory: %s\n", dir_tmp);
  // TODO: 担心这个遍历有点问题
  for (u8 *clus = firstDataSec; clus < endDataSec; clus += CLUSTER_SIZE) {
    if (is_dir_entry((const fat32dir *)clus)) {
      const fat32dir *entry = (const fat32dir *)clus;
      debug("cluster: %p\n", clus);
      while (((uintptr_t)entry - (uintptr_t)clus) < CLUSTER_SIZE) {
        char name[256] = "";
        u8 attr = ATTR_NULL;
        uint64_t clus = 0;
        size_t size = 0;

        entry = parse_dir_entry(entry, name, &attr, &clus,
                                &size); // 要计算clus然后赋值给它
        debug("entry: %p\n", entry);
        assert(entry != NULL);
        printf("name: %s, attr: %x, size: %zu\n", name, attr, size);
        // u8 *addr =
        //     (u8 *)disk_image + firstDataSecOff + (clus - 2) * CLUSTER_SIZE;
        // debug("name: %s, attr: %x, clus: %lu, size: %lu\n", name, attr, clus,
        //       size);
        // if (attr == ATTR_FILE && is_bmp((const struct bmp_hdr *)addr, size))
        // {
        //   // char path[256];
        //   // snprintf(path, sizeof(path), "%s/%s.bmp", dir_tmp, name);
        //   // FILE *fp = fopen(path, "wb");
        //   // if (fp == NULL) {
        //   //   perror("fopen");
        //   //   exit(EXIT_FAILURE);
        //   // }
        //   // fwrite(addr, size, 1, fp);
        //   // fclose(fp);
        //   parse_bmp(name, addr, size, dir_tmp);
        // }
      }
    }
  }

  // 接下来就是遍历每个cluster，找到目录项，然后把可能的bmp文件恢复出来（加上文件名）
  return 0;
}
