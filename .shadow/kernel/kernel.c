#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>

#define SIDE 16

static int w, h;  // Screen size

// typedef struct pictureTypedPNG {
//   char *path;
//   int w, h;
//   long long pictureSize;
//   // 图像深度、颜色类型、压缩方法、滤波方法、隔行扫描方法
//   uintptr_t depth, colorType, compressionMethod, filterMethod, interlaceMethod;
//   uintptr_t *body;
// } png;

// png *loadPNG(char *path) {
//   png *p = (png *)malloc(sizeof(png));
//   strcpy(p->path, path);

//   return p;
// }

#define KEYNAME(key) \
  [AM_KEY_##key] = #key,
static const char *key_names[] = { AM_KEYS(KEYNAME) };

static inline void puts(const char *s) {
  for (; *s; s++) putch(*s);
}

void print_key() {
  AM_INPUT_KEYBRD_T event = { .keycode = AM_KEY_NONE };
  ioe_read(AM_INPUT_KEYBRD, &event);
  if (event.keycode != AM_KEY_NONE && event.keydown) {
    if (event.keycode == AM_KEY_ESCAPE) halt(0);
    puts("Key pressed: ");
    puts(key_names[event.keycode]);
    puts("\n");
  }
}

static void draw_tile(int x, int y, int w, int h, uint32_t color) {
  uint32_t pixels[w * h]; // WARNING: large stack-allocated memory,用这个像素板来显示像素
  AM_GPU_FBDRAW_T event = {
    .x = x, .y = y, .w = w, .h = h, .sync = 1,
    .pixels = pixels,
  };
  for (int i = 0; i < w * h; i++) {
    pixels[i] = color; //设置像素点
  }
  ioe_write(AM_GPU_FBDRAW, &event);
}

void splash() {
  /*
  显示图片有几种方案：
  1. 读取图片中的每一个像素点，并且通过本函数赋值显示在屏幕上
  2. 在.c文件中直接定义一个数组，然后通过本函数显示在屏幕上
  困难在如何适应屏幕分辨率，有点难开工了真的
  */
  AM_GPU_CONFIG_T info = {0};
  ioe_read(AM_GPU_CONFIG, &info);
  w = info.width;
  h = info.height;

  for (int x = 0; x * SIDE <= w; x ++) {
    for (int y = 0; y * SIDE <= h; y++) {
      if ((x & 1) ^ (y & 1)) { // 这里是画棋盘的
        draw_tile(x * SIDE, y * SIDE, SIDE, SIDE, 0xffffff); // white
      }
    }
  }
}

// Operating system is a C program!
int main(const char *args) {
  ioe_init();

  puts("mainargs = \"");
  puts(args);  // make run mainargs=xxx
  puts("\"\n");

  int len = strlen("oonp");
  char *s = "oooo";
  strcpy(s, "oonp");
  puts(s);
  putch('\n');
  putint(len);
  putch('\n');

  splash();

  puts("Press any key to see its key code...\n");
  while (1) {
    print_key();
  }
  return 0;
}
