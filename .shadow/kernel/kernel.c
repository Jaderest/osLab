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
} // 画出去暂时没看到什么问题

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

void draw_line(Point p1, Point p2, int width, uint32_t color) { // 画线
  int dx = abs(p2.x - p1.x), dy = abs(p2.y - p1.y);
  int sx = p1.x < p2.x ? 1 : -1, sy = p1.y < p2.y ? 1 : -1;
  int err = (dx > dy ? dx : -dy) / 2, e2;

  int x = p1.x, y = p1.y;
  int endX = p2.x, endY = p2.y;

  for (int i = 0; i < width; i++) {
    x = p1.x, y = p1.y;
    endX = p2.x, endY = p2.y;

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

void draw_cubic_bezier(Point p1, Point p2, Point p3, uint32_t color) { // 画三次贝塞尔曲线
  for (int t = 0; t <= 1000; t++) {
    int nt = 1000 - t;
    int x = nt * nt * p1.x + 2 * nt * t * p2.x + t * t * p3.x;
    x = x / 1000000;
    int y = nt * nt * p1.y + 2 * nt * t * p2.y + t * t * p3.y;
    y = y / 1000000;
    draw_tile(x, y, 1, 1, color);
  }
}

void draw_triangle(Point p1, Point p2, Point p3, uint32_t color) { // 画三角形
  draw_line(p1, p2, 1, color);
  draw_line(p2, p3, 1, color);
  draw_line(p3, p1, 1, color);
}

void fill_triangle(Point p1, Point p2, Point p3, uint32_t color) { // 填充三角形
  if (p1.y > p2.y) swapInt(&p1.y, &p2.y), swapInt(&p1.x, &p2.x);
  if (p1.y > p3.y) swapInt(&p1.y, &p3.y), swapInt(&p1.x, &p3.x);
  if (p2.y > p3.y) swapInt(&p2.y, &p3.y), swapInt(&p2.x, &p3.x);

  Edge e1 = {p1, p2}, e2 = {p1, p3}, e3 = {p2, p3};

  if (e1.start.y != e1.end.y && e2.start.y != e2.end.y && e3.start.y != e3.end.y) {
    for (int y = p1.y; y <= p3.y; y++) {
      int x1 = e1.start.x + (y - e1.start.y) * (e1.end.x - e1.start.x) / (e1.end.y - e1.start.y); //y相等即出现bug
      int x2 = e2.start.x + (y - e2.start.y) * (e2.end.x - e2.start.x) / (e2.end.y - e2.start.y);
      draw_horizontal_line(x1, x2, y, color);
      if (y == p2.y) e1 = e3;
    }
  } else if (p1.y == p2.y && p2.y == p3.y) {
    draw_horizontal_line(p1.x, p3.x, p1.y, color);
  } else if (p1.y == p2.y) {
    for (int y = p1.y; y <= p3.y; y++) {
      int x1 = e3.start.x + (y - e3.start.y) * (e3.end.x - e3.start.x) / (e3.end.y - e3.start.y);
      int x2 = e2.start.x + (y - e2.start.y) * (e2.end.x - e2.start.x) / (e2.end.y - e2.start.y);
      draw_horizontal_line(x1, x2, y, color);
    }
  } else { // p2.y == p3.y
    for (int y = p1.y; y <= p3.y; y++) {
      int x1 = e1.start.x + (y - e1.start.y) * (e1.end.x - e1.start.x) / (e1.end.y - e1.start.y);
      int x2 = e2.start.x + (y - e2.start.y) * (e2.end.x - e2.start.x) / (e2.end.y - e2.start.y);
      draw_horizontal_line(x1, x2, y, color);
    }
  }
}

void draw_circle(int x0, int y0, int r, uint32_t color) { // 画圆
  int x = r;
  int y = 0;
  int err = 0;

  while (x >= y) {
    draw_tile(x0 + x, y0 + y, 1, 1, color);
    draw_tile(x0 + y, y0 + x, 1, 1, color);
    draw_tile(x0 - y, y0 + x, 1, 1, color);
    draw_tile(x0 - x, y0 + y, 1, 1, color);
    draw_tile(x0 - x, y0 - y, 1, 1, color);
    draw_tile(x0 - y, y0 - x, 1, 1, color);
    draw_tile(x0 + y, y0 - x, 1, 1, color);
    draw_tile(x0 + x, y0 - y, 1, 1, color);

    y++;
    err += 1 + 2 * y;
    if (2 * (err - x) + 1 > 0) {
      x--;
      err += 1 - 2 * x;
    }
  }
}

int sqrt(int x) {
  if (x == 0 || x == 1) {
    return x;
  }
  int left = 1, right = x, ans = 0;
  while (left <= right) {
    int mid = left + (right - left) / 2;
    if (mid <= x / mid) {
      left = mid + 1;
      ans = mid;
    } else {
      right = mid - 1;
    }
  }
  return ans;
}

void fill_circle(int x0, int y0, int r, uint32_t color) { // 填充圆
  int y1 = y0 - r, y2 = y0 + r;
  for (int y = y1; y <= y2; y++) {
    int x = sqrt(r * r - (y - y0) * (y - y0));
    draw_horizontal_line(x0 - x, x0 + x, y, color);
  }
}

void splash43(int w, int h) {
  draw_background(0xffffff); // white
  for (int i = 0; i <= 5; i++) {
    draw_cubic_bezier((Point){0, h-5+i}, (Point){w / 2, h/2+10-2*i}, (Point){w, h-5+i}, 0xffa500); // orange
  }
  draw_line((Point){0, 0}, (Point){w / 2, 2 * h}, 1, 0x0000ff); // blue
  draw_line((Point){w, 0}, (Point){w / 2, 2 * h}, 1, 0x00ff00); // green
  fill_triangle((Point){w / 4, h/2}, (Point){w/2, 0}, (Point){3*w/4, h/2}, 0x008b8b); // darkcyan
  fill_triangle((Point){w / 4, h/4}, (Point){w/2, 3*h/4}, (Point){3*w/4, h/4}, 0x00ffff); // aqua
  fill_circle(w / 2, h, 50, 0xff0000); // red
}

void splash85(int w, int h) {
  draw_background(0x000000);
}

void splash() {
  AM_GPU_CONFIG_T info = {0};
  ioe_read(AM_GPU_CONFIG, &info);
  w = info.width;
  h = info.height;

  if (w == h / 3 * 4) {
    splash43(w, h);
  } else {
    splash85(w, h);
  }
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
