#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "your ssid";
const char* password = "layers given dozen tongue";

// URL of the JSON API
const char* apiUrl = "https://dunder-mifflin.avocado.workers.dev/json";

// how often should the quote be updated
#define uS_TO_S_FACTOR 1000000ULL /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 3600        /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;

// Display Library example for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// Display Library based on Demo Example from Good Display: http://www.e-paper-display.com/download_list/downloadcategoryid=34&isMode=false.html
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2

// Supporting Arduino Forum Topics:
// Waveshare e-paper displays with SPI: http://forum.arduino.cc/index.php?topic=487007.0
// Good Display ePaper for Arduino: https://forum.arduino.cc/index.php?topic=436411.0

// see GxEPD2_wiring_examples.h for wiring suggestions and examples

// NOTE for use with Waveshare ESP32 Driver Board:
// **** also need to select the constructor with the parameters for this board in GxEPD2_display_selection_new_style.h ****
//
// The Wavehare ESP32 Driver Board uses uncommon SPI pins for the FPC connector. It uses HSPI pins, but SCK and MOSI are swapped.
// To use HW SPI with the ESP32 Driver Board, HW SPI pins need be re-mapped in any case. Can be done using either HSPI or VSPI.
// Other SPI clients can either be connected to the same SPI bus as the e-paper, or to the other HW SPI bus, or through SW SPI.
// The logical configuration would be to use the e-paper connection on HSPI with re-mapped pins, and use VSPI for other SPI clients.
// VSPI with standard VSPI pins is used by the global SPI instance of the Arduino IDE ESP32 package.

// uncomment next line to use HSPI for EPD (and e.g VSPI for SD), e.g. with Waveshare ESP32 Driver Board
#define USE_HSPI_FOR_EPD

// base class GxEPD2_GFX can be used to pass references or pointers to the display instance as parameter, uses ~1.2k more code
// enable or disable GxEPD2_GFX base class
#define ENABLE_GxEPD2_GFX 0

// uncomment next line to use class GFX of library GFX_Root instead of Adafruit_GFX
//#include <GFX.h>
// Note: if you use this with ENABLE_GxEPD2_GFX 1:
//       uncomment it in GxEPD2_GFX.h too, or add #include <GFX.h> before any #include <GxEPD2_GFX.h>

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_7C.h>
#include <Fonts/FreeMonoBold9pt7b.h>


// or select the display class and display driver class in the following file (new style):
#include "GxEPD2_display_selection_new_style.h"


#if defined(ARDUINO_ARCH_RP2040) && defined(ARDUINO_RASPBERRY_PI_PICO)
// SPI pins used by GoodDisplay DESPI-PICO. note: steals standard I2C pins PIN_WIRE_SDA (6), PIN_WIRE_SCL (7)
// uncomment next line for use with GoodDisplay DESPI-PICO.
arduino::MbedSPI SPI0(4, 7, 6);  // need be valid pins for same SPI channel, else fails blinking 4 long 4 short
#endif

#if defined(ESP32) && defined(USE_HSPI_FOR_EPD)
SPIClass hspi(HSPI);
#endif

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason() {
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0: Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1: Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER: Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP: Serial.println("Wakeup caused by ULP program"); break;
    default: Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason); break;
  }
}


void setup() {
  Serial.begin(115200);
  delay(1000);  //Take some time to open up the Serial Monitor
  Serial.println();
  Serial.println("setup");

  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));

  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("DNS 1: ");
  Serial.println(WiFi.dnsIP(0));
  Serial.print("DNS 2: ");
  Serial.println(WiFi.dnsIP(1));
  delay(100);

  // define doc variable here already
  StaticJsonDocument<200> doc;

  WiFiClientSecure* client = new WiFiClientSecure;
  if (client) {
    // ignore certificate check
    // client -> setCACert(root_ca);
    client->setInsecure();
    {
      // Make the HTTP GET request
      HTTPClient https;
      https.begin(*client, apiUrl);
      int httpCode = https.GET();
      if (httpCode > 0) {
        String json = https.getString();
        // Serial.println(json);

        // Parse the JSON data
        // StaticJsonDocument<200> doc;
        deserializeJson(doc, json);
        const char* quote = doc["quote"];
        const char* author = doc["author"];

      } else {
        Serial.printf("Error on HTTP request: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    }
    delete client;
  } else {
    Serial.println("Unable to create client");
  }

#if defined(ARDUINO_ARCH_RP2040) && defined(ARDUINO_RASPBERRY_PI_PICO)
  // uncomment next line for use with GoodDisplay DESPI-PICO, or use the extended init method
  //display.epd2.selectSPI(SPI0, SPISettings(4000000, MSBFIRST, SPI_MODE0));
  // uncomment next 2 lines to allow recovery from configuration failures
  pinMode(15, INPUT_PULLUP);            // safety pin
  while (!digitalRead(15)) delay(100);  // check safety pin for fail recovery
#endif
#if defined(ESP32) && defined(USE_HSPI_FOR_EPD)
  hspi.begin(13, 12, 14, 15);  // remap hspi for EPD (swap pins)
  display.epd2.selectSPI(hspi, SPISettings(4000000, MSBFIRST, SPI_MODE0));
#endif
  //display.init(115200); // default 10ms reset pulse, e.g. for bare panels with DESPI-C02
  display.init(115200, true, 2, false);  // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  //display.init(115200, true, 10, false, SPI0, SPISettings(4000000, MSBFIRST, SPI_MODE0)); // extended init method with SPI channel and/or settings selection
  printQuote(doc);
  display.powerOff();
  delay(1000);
  Serial.println("setup done");
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
  Serial.println("Going to sleep now");
  Serial.flush();
  esp_deep_sleep_start();
}

void loop() {
}


int getStringLength(const char* str, int strlength = 0) {
  char buff[1024];
  int16_t x, y;
  uint16_t w, h;
  if (strlength == 0) {
    strcpy(buff, str);
  } else {
    strncpy(buff, str, strlength);
    buff[strlength] = '\0';
  }
  display.getTextBounds(buff, 0, 0, &x, &y, &w, &h);
  return (w);
}

// word wrap routine
// first time send string to wrap
// 2nd and additional times: use empty string
// returns substring of wrapped text.
char* wrapWord(const char* str, int linesize) {
  static char buff[1024];
  int linestart = 0;
  static int lineend = 0;
  static int bufflen = 0;
  if (strlen(str) == 0) {
    // additional line from original string
    linestart = lineend + 1;
    lineend = bufflen;
    Serial.println("existing string to wrap, starting at position " + String(linestart) + ": " + String(&buff[linestart]));
  } else {
    Serial.println("new string to wrap: " + String(str));
    memset(buff, 0, sizeof(buff));
    // new string to wrap
    linestart = 0;
    strcpy(buff, str);
    lineend = strlen(buff);
    bufflen = strlen(buff);
  }
  uint16_t w;
  int lastwordpos = linestart;
  int wordpos = linestart + 1;
  while (true) {
    while (buff[wordpos] == ' ' && wordpos < bufflen)
      wordpos++;
    while (buff[wordpos] != ' ' && wordpos < bufflen)
      wordpos++;
    if (wordpos < bufflen)
      buff[wordpos] = '\0';
    w = getStringLength(&buff[linestart]);
    if (wordpos < bufflen)
      buff[wordpos] = ' ';
    if (w > linesize) {
      buff[lastwordpos] = '\0';
      lineend = lastwordpos;
      return &buff[linestart];
    } else if (wordpos >= bufflen) {
      // first word too long or end of string, send it anyway
      buff[wordpos] = '\0';
      lineend = wordpos;
      return &buff[linestart];
    }
    lastwordpos = wordpos;
    wordpos++;
  }
}

// return # of lines created from word wrap
int getLineCount(const char* str, int scrwidth) {
  int linecount = 0;
  String line = wrapWord(str, scrwidth);

  while (line.length() > 0) {
    linecount++;
    line = wrapWord("", scrwidth);
  }
  return linecount;
}

int getLineHeight(const GFXfont* font = NULL) {
  int height;
  if (font == NULL) {
    height = 12;
  } else {
    height = (uint8_t)pgm_read_byte(&font->yAdvance);
  }
  return height;
}

void printQuote(const JsonDocument& doc) {
  const char* quote = doc["quote"];
  const char* author = doc["author"];
  int x = 200;
  int y = 0;
  bool bsmallfont = false;
  display.setFont(&FreeMonoBold9pt7b);
  // on e-papers black on white is more pleasant to read
  display.setTextColor(GxEPD_BLACK);

  // int scrwidth = display.width() - 8;
  int scrwidth = 400;
  Serial.println("Screen width is " + String(scrwidth));
  Serial.println("Screen height is " + String(display.height()));
  int linecount = getLineCount(quote, scrwidth);
  int lineheightquote = getLineHeight(&FreeMonoBold9pt7b);
  int lineheightauthor = getLineHeight(&FreeMonoBold9pt7b);
  int lineheightother = getLineHeight(&FreeMonoBold9pt7b);
  int maxlines = (display.height() - (lineheightauthor + lineheightother)) / lineheightquote;
  Serial.println("maxlines is " + String(maxlines));
  Serial.println("line height is " + String(lineheightquote));
  Serial.println("linecount is " + String(linecount));
  int topmargin = 0;
  if (linecount > maxlines) {
    // too long for default font size
    // next attempt, reduce lineheight to .8 size
    lineheightquote = .8 * lineheightquote;
    maxlines = (display.height() - (lineheightauthor + lineheightother)) / lineheightquote;
    if (linecount > maxlines) {
      // next attempt, use small font
      display.setFont(&FreeMonoBold9pt7b);
      bsmallfont = true;
      // display.setTextSize(1);
      lineheightquote = getLineHeight(&FreeMonoBold9pt7b);
      maxlines = (display.height() - (lineheightauthor + lineheightother)) / lineheightquote;
      linecount = getLineCount(quote, scrwidth);
      if (linecount > maxlines) {
        // final attempt, last resort is to reduce the lineheight to make it fit
        lineheightquote = (display.height() - (lineheightauthor + lineheightother)) / linecount;
      }
    }
    Serial.println("maxlines has changed to " + String(maxlines));
    Serial.println("line height has changed to " + String(lineheightquote));
    Serial.println("linecount has changed to " + String(linecount));
  }
  if (linecount <= maxlines) {

    topmargin = (display.height() - (lineheightauthor + lineheightother) - linecount * lineheightquote) / 2;
    if (!bsmallfont)
      topmargin += lineheightquote - 4;
    //Serial.println("topmargin = " + String(topmargin));
  }
  String line = wrapWord(quote, scrwidth);
  Serial.println("line length " + String(line.length()));

  int counter = 0;
  Serial.println("counter " + String(counter));
  // full window mode is the initial mode, set it anyway
  display.setFullWindow();
  display.firstPage();
  // here we use paged drawing, even if the processor has enough RAM for full buffer
  // so this can be used with any supported processor board.
  // the cost in code overhead and execution time penalty is marginal
  // tell the graphics class to use paged drawing mode
  display.fillScreen(GxEPD_WHITE);  // set the background to white (fill the buffer with value for white)

  do {
    do {
      counter++;
      Serial.println("printing line " + String(counter) + ": '" + line + String("'"));
      display.setCursor(x + 4, y + topmargin);
      display.print(line);
      y += lineheightquote;
      line = wrapWord("", scrwidth);
    } while (line.length() > 0);
    display.setCursor(x + 4, y + topmargin);
    display.print("- " + String(author));
  } while (display.nextPage());
}
