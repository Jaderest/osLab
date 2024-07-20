#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

struct fat32hdr { //fat32 header
    u8  BS_jmpBoot[3];
    u8  BS_OEMName[8];
    u16 BPB_BytsPerSec;
    u8  BPB_SecPerClus;
    u16 BPB_RsvdSecCnt;
    u8  BPB_NumFATs;
    u16 BPB_RootEntCnt;
    u16 BPB_TotSec16;
    u8  BPB_Media;
    u16 BPB_FATSz16;
    u16 BPB_SecPerTrk;
    u16 BPB_NumHeads;
    u32 BPB_HiddSec;
    u32 BPB_TotSec32;
    u32 BPB_FATSz32;
    u16 BPB_ExtFlags;
    u16 BPB_FSVer;
    u32 BPB_RootClus;
    u16 BPB_FSInfo;
    u16 BPB_BkBootSec;
    u8  BPB_Reserved[12];
    u8  BS_DrvNum;
    u8  BS_Reserved1;
    u8  BS_BootSig;
    u32 BS_VolID;
    u8  BS_VolLab[11];
    u8  BS_FilSysType[8];
    u8  __padding_1[420];
    u16 Signature_word;
} __attribute__((packed));

struct fat32dent {
    u8  DIR_Name[11]; // 11个ascii字符，看看怎么读（8+3）
    u8  DIR_Attr;   // ATTR_READ_ONLY,
                    // ATTR_HIDDEN, 
                    // ATTR_SYSTEM, 
                    // ATTR_VOLUME_ID, 
                    // ATTR_DIRECTORY, 
                    // ATTR_ARCHIVE,
                    // ATTR_LONG_NAME
    u8  DIR_NTRes; // Reserved 0
    u8  DIR_CrtTimeTenth;
    u16 DIR_CrtTime;
    u16 DIR_CrtDate;
    u16 DIR_LastAccDate; // Last access date, must be updated on 
    u16 DIR_FstClusHI; // High 2 bytes of cluster number，和LO一起存储文件或目录的第一个数据簇号
    u16 DIR_WrtTime;
    u16 DIR_WrtDate;
    u16 DIR_FstClusLO; // Low 2 bytes of cluster number
    u32 DIR_FileSize; // 文件大小, size in bytes of file/directory described by entry
} __attribute__((packed)); // 防止对齐 

struct fat32LongName {
    u8  LDIR_Ord;
    u16  LDIR_Name1[5]; // 1-5 这是unicode编码，所以占用2字节
    u8  LDIR_Attr;
    u8  LDIR_Type; // 0x0F
    u8  LDIR_Chksum; // Checksum of name in the
    u16  LDIR_Name2[6]; // 6-11
    u16 LDIR_FstClusLO;
    u16  LDIR_Name3[2]; // 12-13
} __attribute__((packed));

typedef union {
    struct fat32dent short_entry;
    struct fat32LongName long_entry;
} fat32dir;


struct BmpHeader {
    u16 bfType; // "BM"
    u32 bfSize; // File size
    u16 bfReserved1;
    u16 bfReserved2;
    u32 bfOffBits; // Offset to image data
} __attribute__((packed));

struct BmpInfoHeader {
    u32 biSize; // Size of this header, 40
    u32 biWidth; // Width of image
    u32 biHeight; // Height of image
    u16 biPlanes; // Number of color planes, 1
    u16 biBitCount; // Number of bits per pixel
    u32 biCompression; // Compression type
    u32 biSizeImage; // Size of image data
    u32 biXPelsPerMeter; // Horizontal resolution（分辨率）
    u32 biYPelsPerMeter; // Vertical resolution
    u32 biClrUsed; // Number of colors used
    u32 biClrImportant; // Number of important colors
} __attribute__((packed));



#define CLUS_INVALID   0xffffff7

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20
#define ATTR_LONG_NAME (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)
#define LAST_LONG_ENTRY 0x40
#define ATTR_NULL 0x00
