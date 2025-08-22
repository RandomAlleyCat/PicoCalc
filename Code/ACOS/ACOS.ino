#include <SPI.h>
#include <SD.h>
#include <TFT_eSPI.h>       // Hardware-specific library
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include "i2ckbd.h"
#include "pwm_sound.h"


TFT_eSPI tft = TFT_eSPI();  // Invoke custom library

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 320

const int SD_MISO = 16;  // AKA SPI RX
const int SD_MOSI = 19;  // AKA SPI TX
const int SD_CS = 17;
const int SD_SCK =  18;

const uint LEDPIN = 25;


File root;

static int cursor_x = 0;
static int cursor_y = 0;
static char input_buffer[64];
static int input_length = 0;

void simulate_modem_dial();
void show_login_screen();

void setup(void) {
//  Serial.begin(115200);  

  tft.init();

  tft.invertDisplay( true ); // Where i is true or false
  tft.setTextColor(TFT_WHITE);

  gpio_init(LEDPIN);
  gpio_set_dir(LEDPIN,GPIO_OUT);
  gpio_put(LEDPIN,1);
  sleep_ms(5000);
  gpio_put(LEDPIN,0);

  // Ensure the SPI pinout the SD card is connected to is configured properly
  // Select the correct SPI based on _MISO pin for the RP2040
  bool sdInitialized = false;
  SPI.setRX(SD_MISO);
  SPI.setTX(SD_MOSI);
  SPI.setSCK(SD_SCK);
  sdInitialized = SD.begin(SD_CS);


  if (!sdInitialized) {
    Serial.println("initialization failed!");
    
  } else
  {
    Serial.println("initialization done.");

    root = SD.open("/");

    printDirectory(root, 0);

    Serial.println("done!");
  }

  init_i2c_kbd();
  init_pwm();
  simulate_modem_dial();
}

void simulate_modem_dial() {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);

  const char *dial = "ATDT5551212";
  for (const char *p = dial; *p; ++p) {
    // Animate the modem command one character at a time
    tft.setCursor(tft.getCursorX(), tft.getCursorY());
    tft.print(*p);
    if (*p >= '0' && *p <= '9') {
      play_dtmf_sequence(*p);
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
  show_login_screen();
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

void loop() {
  int key = read_i2c_kbd();
  if (key < 0) return;

  if (key >= 32 && key <= 126) {
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
    tft.print("Login: ");
    cursor_x = tft.getCursorX();
    cursor_y = tft.getCursorY();
    input_length = 0;
  }
}

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry = dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.print(entry.size(), DEC);
      time_t cr = entry.getCreationTime();
      time_t lw = entry.getLastWrite();
      struct tm* tmstruct = localtime(&cr);
      Serial.printf("\tCREATION: %d-%02d-%02d %02d:%02d:%02d", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
      tmstruct = localtime(&lw);
      Serial.printf("\tLAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    }
    entry.close();
  }
}

