#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "bitmaps.h"
#include "qdec.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
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
  AX_X = 0,
  AX_Y,
  AX_Z
} axisEnum;

#define RES_LEVELS 4
int currentRes = 0; // res level,
// linResVals[currentRes] determines step size
int linResVals[] = {8,4,2,1};
int angResVals[] = {8,4,2,1};

volatile unsigned int encoderCount = 32;
volatile boolean aSet = false;
volatile boolean bSet = false;

axisEnum currentAxis = AX_X;
bool isAngle = false;

void setup() {
  Serial.begin(38400);
  
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
  attachInterrupt(digitalPinToInterrupt(SWA), quadratureInt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SWB), quadratureInt, CHANGE);
  
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
  //Serial.println("x");
}

void handleYint() {
  if(currentAxis != AX_Y) {
    currentAxis = AX_Y;
    updateScreen();
  }
  //Serial.println("y");
}

void handleZint() {
  if(currentAxis != AX_Z) {
    currentAxis = AX_Z;
    updateScreen();
  }
  //Serial.println("z");
}

void handleThetaint() {
  isAngle = !isAngle;
  updateScreen();
  //Serial.println("theta");
}

void handleResint() {
  currentRes = (currentRes+1)%RES_LEVELS;
  updateScreen();
  //Serial.println("theta");
}

void handleClickint() {
  encoderCount = SCREEN_WIDTH/2;
  updateBar();
}

void quadratureInt(void) {
  using namespace ::SimpleHacks;
  QDECODER_EVENT event = qdec.update();
  int stepSize = isAngle ? angResVals[currentRes] : linResVals[currentRes];
  char writeStr[40]; // sloppy
  if (event & QDECODER_EVENT_CW) {
    encoderCount += stepSize;
    writeMoveCommand(stepSize);
  } else if (event & QDECODER_EVENT_CCW) {
    encoderCount -= stepSize;
    writeMoveCommand(-stepSize);
  }
  encoderCount = encoderCount % SCREEN_WIDTH;
  return;
}

void writeMoveCommand(int delta) {
  Serial.print("STEP"); Serial.print(' ');
  Serial.print(currentAxis); Serial.print(' ');
  Serial.print(isAngle); Serial.print(' ');
  Serial.println(delta);
}

void updateBar() {
  setTopBar(encoderCount);
  //Serial.println(encoderCount);
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
