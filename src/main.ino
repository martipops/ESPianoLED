#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#include <FastLED.h>
#include "OneButton.h"

TFT_eSPI tft = TFT_eSPI();
#define TEXT_HEIGHT 16
#define BOT_FIXED_AREA 0
#define TOP_FIXED_AREA 16
#define YMAX 320
uint16_t yStart = TOP_FIXED_AREA;
uint16_t yArea = YMAX-TOP_FIXED_AREA-BOT_FIXED_AREA;
uint16_t yDraw = YMAX - BOT_FIXED_AREA - TEXT_HEIGHT;
uint16_t xPos = 0;
byte data = 0;
int dataInt = 0;
bool change_colour = 1;
bool selected = 1;

byte commandByte;
byte noteByte;
byte velocityByte;

#define NUM_LEDS 178
CRGB leds[NUM_LEDS];  
CRGB color;
CRGB defColor;
uint8_t hueIter = 0;
boolean pedal;


int ledLoc;
int ledCorrector;

int blank[19]; // We keep all the strings pixel lengths to optimise the speed of the top line blanking

#define PIN_BUTTON_1 0
#define PIN_BUTTON_2 14
int butLastState = HIGH;
int butNewState;
OneButton left_button(PIN_BUTTON_1, true);
OneButton right_button(PIN_BUTTON_2, true);

void setup() {
  Serial.begin(9600);

  left_button.attachClick([] { defColor.setHSV(hueIter+=5,255,255); });
  right_button.attachClick([] { defColor.setHSV(hueIter-=5,255,255); });

  defColor = CRGB::Amethyst;
  FastLED.addLeds<NEOPIXEL, 1>(leds, NUM_LEDS);
  FastLED.setBrightness(200);
  FastLED.show();

  tft.init();
  tft.setRotation(0); // Must be setRotation(0)
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.fillRect(0,0,170,16, TFT_BLUE);
  tft.drawCentreString(" MIDI TIME ",85,0,2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  setupScrollArea(TOP_FIXED_AREA, BOT_FIXED_AREA);
  // Zero the array
  for (byte i = 0; i<18; i++) blank[i]=0;

}

void printTFT(byte in) {
  if (xPos>160) {
      xPos = 0;
      yDraw = scroll_line(); // It can take 13ms to scroll and blank 16 pixel lines
    }
    xPos += tft.drawNumber(in,xPos,yDraw,2);
    blank[(18+(yStart-TOP_FIXED_AREA)/TEXT_HEIGHT)%19]=xPos; // Keep a record of line lengths
    change_colour = 1; // Line to indicate buffer is being emptied
}

void noteCommand() {
  while (Serial.available()<2);
  noteByte = Serial.read();//read next byte
  velocityByte = Serial.read();//read final byte
  ledLoc = (noteByte - 20)*2;
  ledCorrector = (ledLoc<34)?1:((ledLoc>104)?-1:0);
  ledLoc += ledCorrector;
  color = (velocityByte || pedal) ? defColor : CRGB::Black;
  leds[ledLoc] = leds[ledLoc+1] = color;
  FastLED.show();
  printTFT(ledLoc);
}

void controllerCommand() {
  while (Serial.available()<2);
  noteByte = Serial.read();//read next byte
  velocityByte = Serial.read();//read final byte
  pedal = (noteByte == 64)?velocityByte:false;
}

void loop(void) {
  if (Serial.available()) {
    commandByte = Serial.read();
    switch(int(commandByte>>4)) {
      case 8 :case 9: //note on/off
        noteCommand();
        defColor.setHSV(hueIter+=5,255,255);
        break;
      case 11: //controller
        // controllerCommand();
        break;
    }
  } else {
    left_button.tick();
    right_button.tick();
  }
}

int scroll_line() {
  int yTemp = yStart; // Store the old yStart, this is where we draw the next line
  tft.fillRect(0,yStart,blank[(yStart-TOP_FIXED_AREA)/TEXT_HEIGHT],TEXT_HEIGHT, TFT_BLACK);
  yStart+=TEXT_HEIGHT;
  if (yStart >= YMAX - BOT_FIXED_AREA) yStart = TOP_FIXED_AREA + (yStart - YMAX + BOT_FIXED_AREA);
  scrollAddress(yStart);
  return  yTemp;
}

void setupScrollArea(uint16_t tfa, uint16_t bfa) {
  tft.writecommand(ST7789_VSCRDEF); // Vertical scroll definition
  tft.writedata(tfa >> 8);           // Top Fixed Area line count
  tft.writedata(tfa);
  tft.writedata((YMAX-tfa-bfa)>>8);  // Vertical Scrolling Area line count
  tft.writedata(YMAX-tfa-bfa);
  tft.writedata(bfa >> 8);           // Bottom Fixed Area line count
  tft.writedata(bfa);
}

void scrollAddress(uint16_t vsp) {
  tft.writecommand(ST7789_VSCRDEF); // Vertical scrolling pointer
  tft.writedata(vsp>>8);
  tft.writedata(vsp);
}
