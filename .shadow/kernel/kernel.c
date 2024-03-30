#include <am.h>
#include <amdev.h>
#include <klib.h>
#include <klib-macros.h>

#define SIDE 40

static int w, h;  // Screen size

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

typedef struct {
  int x, y;
} Point;

typedef struct {
  Point start, end;
} Edge;

void swapInt(int *a, int *b) {
  int t = *a;
  *a = *b;
  *b = t;
}

static void draw_tile(int x, int y, int w, int h, uint32_t color) { //画地砖
  uint32_t pixels[w * h]; // WARNING: large stack-allocated memory, 45以上就会栈溢出
  AM_GPU_FBDRAW_T event = {
    .x = x, .y = y, .w = w, .h = h, .sync = 1,
    .pixels = pixels,
  };
  for (int i = 0; i < w * h; i++) {
    pixels[i] = color; //设置像素点
  }
  ioe_write(AM_GPU_FBDRAW, &event);
}

void draw_background(uint32_t color) { // 设置背景颜色
  for (int x = 0; x <= w; x ++) {
    for (int y = 0; y <= h; y++) {
      draw_tile(x, y, 1, 1, color); // white
    }
  }
}

void draw_horizontal_line(int x1, int x2, int y, uint32_t color) { // 画横线
  if (x1 > x2) swapInt(&x1, &x2);
  for (int x = x1; x <= x2; x++) {
    draw_tile(x, y, 1, 1, color);
  }
}

void draw_line(int x1, int y1, int x2, int y2, int width, uint32_t color) { // 画线
  int dx = abs(x2 - x1), dy = abs(y2 - y1);
  int sx = x1 < x2 ? 1 : -1, sy = y1 < y2 ? 1 : -1;
  int err = (dx > dy ? dx : -dy) / 2, e2;

  int x = x1, y = y1;
  int endX = x2, endY = y2;

  for (int i = 0; i < width; i++) {
    x = x1, y = y1;
    endX = x2, endY = y2;

    if (dy > dx) {
      y += i;
      endY += i;
    } else {
      x += i;
      endX += i;
    }

    while(1) {
      draw_tile(x, y, 1, 1, color);
      if (x == endX && y == endY) break;
      e2 = err;
      if (e2 > -dx) { err -= dy; x += sx; }
      if (e2 < dy) { err += dx; y += sy; }
    }
  }
}

void fill_triangle(Point p1, Point p2, Point p3, uint32_t color) { // 填充三角形
  if (p1.y > p2.y) swapInt(&p1.y, &p2.y), swapInt(&p1.x, &p2.x);
  if (p1.y > p3.y) swapInt(&p1.y, &p3.y), swapInt(&p1.x, &p3.x);
  if (p2.y > p3.y) swapInt(&p2.y, &p3.y), swapInt(&p2.x, &p3.x);

  Edge e1 = {p1, p2}, e2 = {p1, p3}, e3 = {p2, p3};

  for (int y = p1.y; y <= p2.y; y++) {
    int x1 = e1.start.x + (e1.end.x - e1.start.x) * (y - e1.start.y) / (e1.end.y - e1.start.y);
    int x2 = e2.start.x + (e2.end.x - e2.start.x) * (y - e2.start.y) / (e2.end.y - e2.start.y);
    draw_horizontal_line(x1, x2, y, color);

    if (y == e1.end.y) {
      e1.start = e2.start;
    }
    if (y == e2.end.y) {
      e2.start = e3.start;
    }
  }
}

void splash() {
  AM_GPU_CONFIG_T info = {0};
  ioe_read(AM_GPU_CONFIG, &info);
  w = info.width;
  h = info.height;
  putint(w);
  putch('\n');
  putint(h);
  putch('\n');

  draw_background(0xffffff); // 设置背景颜色
  draw_line(0, 0, w, h, 5, 0xff0000); // 画一条线
  draw_line(w, 0, 0, h, 5, 0x00ff00);
  fill_triangle((Point){0, h}, (Point){0, 0}, (Point){w/2, h/2}, 0x0000ff); // 填充三角形
}

// Operating system is a C program!
int main(const char *args) {
  ioe_init();

  puts("mainargs = \"");
  puts(args);  // make run mainargs=xxx
  puts("\"\n");

  int len = strlen("oonp");
  char s[len + 1];
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
