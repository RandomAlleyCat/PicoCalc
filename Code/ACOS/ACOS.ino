#include <SPI.h>
#include <TFT_eSPI.h>       // Hardware-specific library
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include "i2ckbd.h"
#include "pwm_sound.h"
#include <hardware/watchdog.h>


TFT_eSPI tft = TFT_eSPI();  // Invoke custom library

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 320

const uint LEDPIN = 25;



static int cursor_x = 0;
static int cursor_y = 0;
static char input_buffer[64];
static int input_length = 0;
static bool at_telnet_prompt = true;

void simulate_modem_dial(const char *number);
void show_login_screen();
void handle_command(const char *line);

void setup(void) {
//  Serial.begin(115200);  

  tft.init();

  tft.invertDisplay( true ); // Where i is true or false
  tft.setTextColor(TFT_WHITE);

  gpio_init(LEDPIN);
  gpio_set_dir(LEDPIN,GPIO_OUT);
  gpio_put(LEDPIN,1);
  gpio_put(LEDPIN,0);

  init_i2c_kbd();
  init_pwm();

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.println("Connected.");
  tft.print("telnet> ");
  cursor_x = tft.getCursorX();
  cursor_y = tft.getCursorY();
  input_length = 0;
}

void simulate_modem_dial(const char *number) {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);

  const char *prefix = "ATDT";
  for (const char *p = prefix; *p; ++p) {
    tft.print(*p);
    sleep_ms(150);
  }

  for (const char *p = number; *p; ++p) {
    tft.print(*p);
    if (*p >= '0' && *p <= '9') {
      char digit[2] = {*p, '\0'};
      play_dtmf_sequence(digit);
    }
    sleep_ms(150);
  }

  // Animate a simple "Dialing..." progress indicator
  for (int i = 0; i <= 3; ++i) {
    tft.setCursor(0, 16);
    tft.print("Dialing");
    for (int j = 0; j < i; ++j) {
      tft.print(".");
    }
    for (int j = i; j < 3; ++j) {
      tft.print(" ");
    }
    sleep_ms(300);
  }

  tft.setCursor(0, 32);
  tft.setTextColor(TFT_GREEN);
  tft.println("CONNECT 9600");
  play_modem_handshake();

  // Flash the display to indicate the connection
  tft.invertDisplay(true);
  sleep_ms(100);
  tft.invertDisplay(false);
  tft.setTextColor(TFT_WHITE);

  sleep_ms(500);
}

void show_login_screen() {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.println("=== ACOS BBS ===");
  tft.println();
  tft.print("Login: ");
  cursor_x = tft.getCursorX();
  cursor_y = tft.getCursorY();
  input_length = 0;
}

void handle_command(const char *line) {
  // Placeholder for future command routing or screen switching.
  tft.print("telnet> ");
  cursor_x = tft.getCursorX();
  cursor_y = tft.getCursorY();
}

void loop() {
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
      tft.print(ch);
      cursor_x = tft.getCursorX();
      play_click();
    }
  } else if (key == KEY_BACKSPACE) {
    if (input_length > 0) {
      input_length--;
      int16_t w = tft.textWidth(" ");
      cursor_x -= w;
      tft.setCursor(cursor_x, cursor_y);
      tft.print(" ");
      tft.setCursor(cursor_x, cursor_y);
    }
  } else if (key == KEY_ENTER) {
    input_buffer[input_length] = '\0';
    tft.println();
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

