#include <SPI.h>
#include <TFT_eSPI.h>
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include "i2ckbd.h"
#include "pwm_sound.h"
#include <hardware/watchdog.h>
#include "TomThumb.h"

TFT_eSPI tft = TFT_eSPI();

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 320

#define TERM_COLS 80
#define TERM_ROWS 24
#define CELL_W 4
#define CELL_H 6
#define STATUS_HEIGHT CELL_H
#define TOP_MARGIN STATUS_HEIGHT

struct Cell {
  char ch;
  uint8_t fg;
  uint8_t bg;
};

static Cell cells[TERM_ROWS][TERM_COLS];
static uint8_t cursor_x = 0, cursor_y = 0;
static uint8_t prev_cx = 0, prev_cy = 0;
static uint8_t cur_fg = 7; // white text
static uint8_t cur_bg = 4; // blue background

static uint16_t palette[16];

static char input_buffer[64];
static int input_length = 0;
static bool at_telnet_prompt = true;

static bool connected = false;
static uint32_t boot_ms;
static uint32_t connect_ms;

enum {STATE_NORMAL, STATE_ESC, STATE_CSI};
static int esc_state = STATE_NORMAL;
static int esc_params[4];
static int esc_param_count = 0;

void init_palette() {
  palette[0] = tft.color565(0,0,0);       // Black
  palette[1] = tft.color565(128,0,0);     // Red
  palette[2] = tft.color565(0,128,0);     // Green
  palette[3] = tft.color565(128,128,0);   // Yellow
  palette[4] = tft.color565(0,0,128);     // Blue
  palette[5] = tft.color565(128,0,128);   // Magenta
  palette[6] = tft.color565(0,128,128);   // Cyan
  palette[7] = tft.color565(192,192,192); // White
  palette[8] = tft.color565(128,128,128); // Bright black
  palette[9] = tft.color565(255,0,0);     // Bright red
  palette[10]= tft.color565(0,255,0);     // Bright green
  palette[11]= tft.color565(255,255,0);   // Bright yellow
  palette[12]= tft.color565(0,0,255);     // Bright blue
  palette[13]= tft.color565(255,0,255);   // Bright magenta
  palette[14]= tft.color565(0,255,255);   // Bright cyan
  palette[15]= tft.color565(255,255,255); // Bright white
}

void render_cell(int col, int row) {
  Cell &cell = cells[row][col];
  int px = col * CELL_W;
  int py = TOP_MARGIN + row * CELL_H;
  tft.fillRect(px, py, CELL_W, CELL_H, palette[cell.bg]);
  tft.setCursor(px, py + CELL_H - 1);
  tft.setTextColor(palette[cell.fg], palette[cell.bg]);
  tft.write(cell.ch);
}

void draw_cursor() {
  render_cell(prev_cx, prev_cy);
  prev_cx = cursor_x;
  prev_cy = cursor_y;
  Cell &cell = cells[cursor_y][cursor_x];
  int px = cursor_x * CELL_W;
  int py = TOP_MARGIN + cursor_y * CELL_H;
  tft.fillRect(px, py, CELL_W, CELL_H, palette[cell.fg]);
  tft.setCursor(px, py + CELL_H - 1);
  tft.setTextColor(palette[cell.bg], palette[cell.fg]);
  tft.write(cell.ch);
}

void clear_screen() {
  for (int y = 0; y < TERM_ROWS; ++y) {
    for (int x = 0; x < TERM_COLS; ++x) {
      cells[y][x].ch = ' ';
      cells[y][x].fg = cur_fg;
      cells[y][x].bg = cur_bg;
    }
  }
  tft.fillRect(0, TOP_MARGIN, TERM_COLS * CELL_W, TERM_ROWS * CELL_H, palette[cur_bg]);
  cursor_x = cursor_y = 0;
  draw_cursor();
}

void scroll_up() {
  for (int y = 0; y < TERM_ROWS - 1; ++y) {
    for (int x = 0; x < TERM_COLS; ++x) {
      cells[y][x] = cells[y + 1][x];
    }
  }
  for (int x = 0; x < TERM_COLS; ++x) {
    cells[TERM_ROWS - 1][x].ch = ' ';
    cells[TERM_ROWS - 1][x].fg = cur_fg;
    cells[TERM_ROWS - 1][x].bg = cur_bg;
  }
  tft.fillRect(0, TOP_MARGIN, TERM_COLS * CELL_W, TERM_ROWS * CELL_H, palette[cur_bg]);
  for (int y = 0; y < TERM_ROWS; ++y)
    for (int x = 0; x < TERM_COLS; ++x)
      render_cell(x, y);
}

void clear_line_from_cursor() {
  for (int x = cursor_x; x < TERM_COLS; ++x) {
    cells[cursor_y][x].ch = ' ';
    cells[cursor_y][x].fg = cur_fg;
    cells[cursor_y][x].bg = cur_bg;
    render_cell(x, cursor_y);
  }
}

void handle_csi(char final) {
  int p1 = esc_param_count > 0 ? esc_params[0] : 0;
  int p2 = esc_param_count > 1 ? esc_params[1] : 0;
  switch (final) {
    case 'm':
      if (esc_param_count == 0) {
        cur_fg = 7;
        cur_bg = 4;
      }
      for (int i = 0; i < esc_param_count; ++i) {
        int p = esc_params[i];
        if (p == 0) { cur_fg = 7; cur_bg = 4; }
        else if (p >= 30 && p <= 37) cur_fg = p - 30;
        else if (p >= 40 && p <= 47) cur_bg = p - 40;
        else if (p >= 90 && p <= 97) cur_fg = p - 90 + 8;
        else if (p >= 100 && p <= 107) cur_bg = p - 100 + 8;
      }
      break;
    case 'H':
    case 'f':
      cursor_y = p1 ? p1 - 1 : 0;
      cursor_x = p2 ? p2 - 1 : 0;
      break;
    case 'A':
      cursor_y = (cursor_y >= (p1 ? p1 : 1)) ? cursor_y - (p1 ? p1 : 1) : 0;
      break;
    case 'B':
      cursor_y = cursor_y + (p1 ? p1 : 1);
      if (cursor_y >= TERM_ROWS) cursor_y = TERM_ROWS - 1;
      break;
    case 'C':
      cursor_x = cursor_x + (p1 ? p1 : 1);
      if (cursor_x >= TERM_COLS) cursor_x = TERM_COLS - 1;
      break;
    case 'D':
      cursor_x = (cursor_x >= (p1 ? p1 : 1)) ? cursor_x - (p1 ? p1 : 1) : 0;
      break;
    case 'J':
      if (p1 == 2) clear_screen();
      break;
    case 'K':
      clear_line_from_cursor();
      break;
  }
  esc_state = STATE_NORMAL;
  draw_cursor();
}

void term_putc(char c) {
  if (esc_state == STATE_NORMAL) {
    if (c == 27) { esc_state = STATE_ESC; return; }
    if (c == '\r') { cursor_x = 0; draw_cursor(); return; }
    if (c == '\n') { cursor_y++; if (cursor_y >= TERM_ROWS) { scroll_up(); cursor_y = TERM_ROWS - 1; } draw_cursor(); return; }
    if (c == 8) { if (cursor_x > 0) { cursor_x--; cells[cursor_y][cursor_x].ch = ' '; render_cell(cursor_x, cursor_y); } draw_cursor(); return; }
    cells[cursor_y][cursor_x].ch = c;
    cells[cursor_y][cursor_x].fg = cur_fg;
    cells[cursor_y][cursor_x].bg = cur_bg;
    render_cell(cursor_x, cursor_y);
    cursor_x++;
    if (cursor_x >= TERM_COLS) { cursor_x = 0; cursor_y++; if (cursor_y >= TERM_ROWS) { scroll_up(); cursor_y = TERM_ROWS - 1; } }
    draw_cursor();
  } else if (esc_state == STATE_ESC) {
    if (c == '[') {
      esc_state = STATE_CSI;
      esc_param_count = 0;
      esc_params[0] = 0;
    } else {
      esc_state = STATE_NORMAL;
    }
  } else if (esc_state == STATE_CSI) {
    if (c >= '0' && c <= '9') {
      esc_params[esc_param_count] = esc_params[esc_param_count] * 10 + (c - '0');
    } else if (c == ';') {
      esc_param_count++;
      if (esc_param_count >= 4) esc_param_count = 3;
      esc_params[esc_param_count] = 0;
    } else {
      esc_param_count++;
      handle_csi(c);
    }
  }
}

void term_print(const char *s) {
  while (*s) term_putc(*s++);
}

void draw_status() {
  char buf[TERM_COLS + 1];
  uint32_t now = millis();
  uint32_t uptime = (now - boot_ms) / 1000;
  uint32_t ctime = connected ? (now - connect_ms) / 1000 : 0;
  snprintf(buf, sizeof(buf), "Conn:%s Up:%lus Con:%lus", connected ? "ON" : "OFF", uptime, ctime);
  int len = strlen(buf);
  if (len > TERM_COLS) len = TERM_COLS;
  for (int i = len; i < TERM_COLS; ++i) buf[i] = ' ';
  buf[TERM_COLS] = '\0';
  tft.fillRect(0, 0, TERM_COLS * CELL_W, STATUS_HEIGHT, palette[4]);
  tft.setCursor(0, STATUS_HEIGHT - 1);
  tft.setTextColor(palette[7], palette[4]);
  tft.print(buf);
}

void update_status() {
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last > 1000) {
    last = now;
    draw_status();
  }
}

void simulate_modem_dial(const char *number) {
  clear_screen();
  term_print("ATDT");
  term_print(number);
  term_print("\r\nCONNECT 9600\r\n");
  play_modem_handshake();
  connected = true;
  connect_ms = millis();
  draw_status();
}

void show_login_screen() {
  clear_screen();
  term_print("=== ACOS BBS ===\r\n\r\n");
  term_print("Login: ");
}

void handle_command(const char *line) {
  term_print("telnet> ");
}

void setup(void) {
  tft.init();
  tft.invertDisplay(true);
  tft.setRotation(0);
  tft.setFreeFont(&TomThumb);

  gpio_init(25);
  gpio_set_dir(25, GPIO_OUT);
  gpio_put(25, 1);
  gpio_put(25, 0);

  init_i2c_kbd();
  init_pwm();

  init_palette();
  tft.fillScreen(palette[cur_bg]);
  boot_ms = millis();
  draw_status();
  clear_screen();
  term_print("Connected.\r\n");
  term_print("telnet> ");
  input_length = 0;
}

void loop() {
  update_status();
  int key = read_i2c_kbd();
  if (key < 0) return;

  if (key == KEY_CTRL_ALT_DEL) {
    watchdog_reboot(0, 0, 0);
  } else if (key >= 32 && key <= 126) {
    if (at_telnet_prompt && (key < '0' || key > '9')) {
      return;
    }
    if (input_length < (int)sizeof(input_buffer) - 1) {
      char ch = (char)key;
      input_buffer[input_length++] = ch;
      term_putc(ch);
      play_click();
    }
  } else if (key == KEY_BACKSPACE) {
    if (input_length > 0) {
      input_length--;
      term_putc('\b');
    }
  } else if (key == KEY_ENTER) {
    input_buffer[input_length] = '\0';
    term_putc('\r');
    term_putc('\n');
    if (at_telnet_prompt) {
      simulate_modem_dial(input_buffer);
      show_login_screen();
      at_telnet_prompt = false;
    } else {
      handle_command(input_buffer);
    }
    input_length = 0;
  }
}

