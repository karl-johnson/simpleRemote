#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "bitmaps.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels


#include "qdec.h"

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// encoder
#define SWA 0
#define SWB 1
#define SWCLICK 2
::SimpleHacks::QDecoder qdec(SWA, SWB, true);

#define SWRES 3
#define SWTHETA 6
#define SWX 8
#define SWY 9
#define SWZ 10

typedef enum {
  AX_X,
  AX_Y,
  AX_Z
} axisEnum;

#define RES_LEVELS 4
int currentRes = 0; // res level, higher is more resolution

volatile unsigned int encoderCount = 32;
volatile boolean aSet = false;
volatile boolean bSet = false;

axisEnum currentAxis = AX_X;
bool isAngle = false;

void setup() {
  Serial.begin(115200);
  
  pinMode(SWX, INPUT);
  pinMode(SWY, INPUT);
  pinMode(SWZ, INPUT);
  pinMode(SWTHETA, INPUT);
  pinMode(SWRES, INPUT);
  pinMode(SWCLICK, INPUT);
  

  attachInterrupt(digitalPinToInterrupt(SWX), handleXint, FALLING);
  attachInterrupt(digitalPinToInterrupt(SWY), handleYint, FALLING);
  attachInterrupt(digitalPinToInterrupt(SWZ), handleZint, FALLING);
  attachInterrupt(digitalPinToInterrupt(SWTHETA), handleThetaint, FALLING);
  attachInterrupt(digitalPinToInterrupt(SWRES), handleResint, FALLING);
  attachInterrupt(digitalPinToInterrupt(SWCLICK), handleClickint, FALLING);
  
  
  qdec.begin();
  attachInterrupt(digitalPinToInterrupt(SWA), IsrForQDEC, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SWB), IsrForQDEC, CHANGE);
  

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  updateScreen();
}

int lastLoopDisplayedRotaryCount = 0;

void loop() {
  int newValue = encoderCount;
  if (newValue != lastLoopDisplayedRotaryCount) {
  updateBar();
    lastLoopDisplayedRotaryCount = newValue;
  }
}

void handleXint() {
  if(currentAxis != AX_X) {
    currentAxis = AX_X;
    updateScreen();
  }
  Serial.println("x");
}

void handleYint() {
  if(currentAxis != AX_Y) {
    currentAxis = AX_Y;
    updateScreen();
  }
  Serial.println("y");
}

void handleZint() {
  if(currentAxis != AX_Z) {
    currentAxis = AX_Z;
    updateScreen();
  }
  Serial.println("z");
}

void handleThetaint() {
  isAngle = !isAngle;
  updateScreen();
  Serial.println("theta");
}

void handleResint() {
  currentRes = (currentRes+1)%RES_LEVELS;
  updateScreen();
  Serial.println("theta");
}
/*
void handleInterruptA() {
  if (digitalRead(SWB)) {
    encoderCount++;
  } 
  else {
    encoderCount--;
  }
  updateBar();
}
*/
/*
void handleInterruptB() {
  bSet = digitalRead(SWB);
  if (bSet) {
    aSet = digitalRead(SWA);
    if (aSet) {
      encoderCount--;
    } else {
      encoderCount++;
    }
  } else {
    aSet = digitalRead(SWA);
    if (aSet) {
      encoderCount++;
    } else {
      encoderCount--;
    }
  }
  updateBar();
}
*/
void updateBar() {
  setTopBar(encoderCount*2);
  Serial.println(encoderCount);
}

void updateScreen() {
  display.clearDisplay();
  if(isAngle) {
    switch(currentAxis) {
      case AX_X:
        displayAxis(txBmp);
        break;
      case AX_Y:
        displayAxis(tyBmp);
        break;
      case AX_Z:
        displayAxis(tzBmp);
        break;
    }
    switch(currentRes) {
      case 0:
        displayMult(r10Bmp);
        displayUnit(mradBmp);
        break;
      case 1:
        displayMult(r1Bmp);
        displayUnit(mradBmp);
        break;
      case 2:
        displayMult(r1000Bmp);
        displayUnit(uradBmp);
        break;
      case 3:
        displayMult(r100Bmp);
        displayUnit(uradBmp);
        break;
    }
  }
  else {
    switch(currentAxis) {
      case AX_X:
        displayAxis(xBmp);
        break;
      case AX_Y:
        displayAxis(yBmp);
        break;
      case AX_Z:
        displayAxis(zBmp);
        break;
    }
    switch(currentRes) {
      case 0:
        displayMult(r1000Bmp);
        break;
      case 1:
        displayMult(r100Bmp);
        break;
      case 2:
        displayMult(r10Bmp);
        break;
      case 3:
        displayMult(r1Bmp);
        break;
    }
    displayUnit(umBmp);
  }
  updateBar();
  display.display();
}

void displayAxis(const unsigned char* bitmap) {
  display.drawBitmap(0, 16, bitmap, AXIS_WIDTH, AXIS_HEIGHT, SSD1306_WHITE);
  //display.display();
}

void displayMult(const unsigned char* bitmap) {
  display.drawBitmap(AXIS_WIDTH, 16, bitmap, MULT_WIDTH, MULT_HEIGHT, SSD1306_WHITE);
  //display.display();
}

void displayUnit(const unsigned char* bitmap) {
  display.drawBitmap(AXIS_WIDTH, 16+MULT_HEIGHT, bitmap, UNIT_WIDTH, UNIT_HEIGHT, SSD1306_WHITE);
  //display.display();
}

void setTopBar(int thisWidth) {
  
  display.fillRect(0,0,SCREEN_WIDTH,16, SSD1306_BLACK);
  display.fillRect(0,0,thisWidth,16, SSD1306_WHITE);
  display.display();
}

void IsrForQDEC(void) {
  using namespace ::SimpleHacks;
  QDECODER_EVENT event = qdec.update();
  if (event & QDECODER_EVENT_CW) {
    encoderCount = encoderCount + 1;
  } else if (event & QDECODER_EVENT_CCW) {
    encoderCount = encoderCount - 1;
  }
  encoderCount = encoderCount % 64;
  return;
}
