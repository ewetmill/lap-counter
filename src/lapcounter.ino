/* 
 *  Lap Counter
 *  
 *  3 two digit displays with 7-segment number using NeoPixels
 *  
 *  Written by Eric Wetmiller
 *  
 *  MIT License
 *  
 *  Hardware:
 *  NeoPixels 60/meter, 5 meters
 *  1000uF Capacitors (1 per digit)
 *  RGB IR remote
 *  Insignia NS-RC4NA-17 Remote Control
 *  IR Sensor(Connected to pin 11
 *  Itsy Bitsy M0
 */
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

#include "IRLibAll.h"
#include <IRLibDecodeBase.h> // First include the decode base
#include <IRLib_P01_NEC.h>   // Now include only the protocols you wish
#include <IRLib_P02_Sony.h>  // to actually use. The lowest numbered
#include <IRLib_P07_NECx.h>  // must be first but others can be any order.
#include <IRLib_P09_GICable.h>
#include <IRLib_P11_RCMM.h>
#include <IRLibCombo.h>

#define PIXELS_PER_SEGMENT 6
#define SEGMENTS_PER_DIGIT 7
#define DIGITS_PER_DISPLAY 2
#define NUM_DISPLAYS 3

const int totalPixels = PIXELS_PER_SEGMENT * SEGMENTS_PER_DIGIT * DIGITS_PER_DISPLAY * NUM_DISPLAYS;

IRdecode decoder;

// Include a receiver either this or IRLibRecvPCI or IRLibRecvLoop
#include <IRLibRecv.h>
int RECV_PIN = 11;
#define PIXEL_PIN 5
IRrecv irrecv(RECV_PIN);  //pin number for the receiver

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (Pin 5 on the Itsy Bitsy M0 puts out 5V for the Pixel logic)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(totalPixels, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

enum PowerState {
  POWER_ON,
  POWER_OFF
};

struct Color {
  int r;
  int g;
  int b;
};

// Colors for the red shades on the color remote
Color reds[5] = {
  {255,  0,  0},
  {255, 64,  0},
  {255, 128,  0},
  {255, 192,  0},
  {255, 255,  0}
};

// Colors for the green shades on the color remote
Color greens[5] = {
  {  0, 255,  0},
  {  0, 255, 64},
  {  0, 255, 128},
  {  0, 255, 192},
  {  0, 255, 255}
};

// Colors for the blue shades on the color remote
Color blues[5] = {
  {  0,  0, 255},
  { 64,  0, 255},
  {128,  0, 255},
  {192,  0, 255},
  {255,  0, 255}
};

// Colors for the white shades on the color remote
Color brights[5] = {
  {255, 255, 255},
  {255, 192, 255},
  {255, 192, 192},
  {192, 255, 255},
  {192, 192, 255}
};


Color OFF   = {0,    0,    0};
Color GREEN = {0,  255,    0};
Color RED   = {255,  0,    0};
Color BLUE  = {0,    0,  255};
Color WHITE = {255, 255,  255};

// The light strip brightness
// adjust brightness for all pixels 0-255 range,
// 32 being pretty dim, 255 being full brightness
int bright = 200; 

// The power status of the displays
PowerState power = POWER_ON;

// The currently active color
Color activeColor = {255, 0, 0};

// The current lap count
int count = 0;

/**
 * A single Segment.  Each Segment is an array of pixels that illuminates a
 * single portion of a seven segment digit.
 */
class Segment {
  private:
    // The starting index of the first light in the segment
    const int startIndex;

    // The number of lights in the segment
    const int numLights;

  public:
    Segment(int start, int numLights) :
      startIndex(start),
      numLights(numLights) {
    }

    void draw(const Color color) const {
      for (int i = startIndex; i < startIndex + numLights; i++) {
        Serial.print("drawing pixel: "); Serial.println(i);
        strip.setPixelColor(i, color.r, color.g, color.b);
      }
    }

    void clear() const {
      for (int i = startIndex; i < startIndex + numLights; i++) {
        strip.setPixelColor(i, OFF.r, OFF.g, OFF.b);
      }
    }
};

/**
 * A seven segment digit.
 * 
 * Letters are the standard segment naming, as seen from the front,
   numbers are based upon the wiring sequence

                A 1
           ----------
          |          |
          |          |
      F 0 |          | B 2
          |          |
          |     G 6  |
           ----------
          |          |
          |          |
      E 5 |          | C 3
          |          |
          |     D 4  |
           ----------
 */
class SegmentedDigit {
  private:
    // A mapping of which segments to illuminate for each digit
    const bool segmentMap[10][7] = {
      // segments {F,A,B,C,D,E,G}
      {true, true, true, true, true, true, false}, // 0
      {false, false, true, true, false, false, false}, // 1
      {false, true, true, false, true, true, true}, // 2
      {false, true, true, true, true, false, true}, // 3
      {true, false, true, true, false, false, true}, // 4
      {true, true, false, true, true, false, true}, // 5
      {true, true, false, true, true, true, true}, // 6
      {false, true, true, true, false, false, false}, // 7
      {true, true, true, true, true, true, true}, // 8
      {true, true, true, true, true, false, true} // 9
    };

    // The index of the first light in the digit
    const int startIndex;

    // The array of segments in the digit
    const Segment segments[SEGMENTS_PER_DIGIT];

  public:
    SegmentedDigit(const int start) :
      startIndex(start),
      segments( {
      Segment(start + (0 * PIXELS_PER_SEGMENT), PIXELS_PER_SEGMENT),
              Segment(start + (1 * PIXELS_PER_SEGMENT), PIXELS_PER_SEGMENT),
              Segment(start + (2 * PIXELS_PER_SEGMENT), PIXELS_PER_SEGMENT),
              Segment(start + (3 * PIXELS_PER_SEGMENT), PIXELS_PER_SEGMENT),
              Segment(start + (4 * PIXELS_PER_SEGMENT), PIXELS_PER_SEGMENT),
              Segment(start + (5 * PIXELS_PER_SEGMENT), PIXELS_PER_SEGMENT),
              Segment(start + (6 * PIXELS_PER_SEGMENT), PIXELS_PER_SEGMENT)
    }) {
    }

    ////////////////////////////////////////////////////////////////////////////////
    void draw(const int val, const Color col) const {
      //use this to light up a digit
      //digit is which digit panel one (right to left, 0 indexed)
      //val is the character value to set on the digit
      //col is the predefined color to use, R,G,B or W
      //example:
      //        digitWrite(0, 4, 2);
      //would set the first digit
      //on the right to a "4" in green.

      //these are the digit panel character value definitions,
      //if color argument is a 0, the segment is off

      Serial.print("drawing digit: "); Serial.print(val); Serial.print(" Start index: "); Serial.println(startIndex);
      for (int i = 0; i < 7; i++) {
        if (segmentMap[val][i]) {
          segments[i].draw(col);
        }
        else {
          segments[i].clear();
        }
      }
    }

    static int getNumPixels() {
      return PIXELS_PER_SEGMENT * SEGMENTS_PER_DIGIT;
    }
};

/**
 * A display with multiple digits. Used to form larger integer values.
 */
class MultiDigitDisplay {
  private:
    // Any more than 10 digits will cause problems with power and memory on most boards.
    static const int MAX_DIGITS_PER_DISP = 10;

    // The number of digits on the display
    const int numDigits;

    // The array of digits on the display
    const SegmentedDigit digits[MAX_DIGITS_PER_DISP];

  public:
    MultiDigitDisplay(const int digits, const int offset) :
      numDigits(digits),
      digits( {
      SegmentedDigit((offset * digits * SegmentedDigit::getNumPixels()) + (0 * SegmentedDigit::getNumPixels())),
                     SegmentedDigit((offset * digits * SegmentedDigit::getNumPixels()) + (1 * SegmentedDigit::getNumPixels())),
                     SegmentedDigit((offset * digits * SegmentedDigit::getNumPixels()) + (2 * SegmentedDigit::getNumPixels())),
                     SegmentedDigit((offset * digits * SegmentedDigit::getNumPixels()) + (3 * SegmentedDigit::getNumPixels())),
                     SegmentedDigit((offset * digits * SegmentedDigit::getNumPixels()) + (4 * SegmentedDigit::getNumPixels())),
                     SegmentedDigit((offset * digits * SegmentedDigit::getNumPixels()) + (5 * SegmentedDigit::getNumPixels())),
                     SegmentedDigit((offset * digits * SegmentedDigit::getNumPixels()) + (6 * SegmentedDigit::getNumPixels())),
                     SegmentedDigit((offset * digits * SegmentedDigit::getNumPixels()) + (7 * SegmentedDigit::getNumPixels())),
                     SegmentedDigit((offset * digits * SegmentedDigit::getNumPixels()) + (8 * SegmentedDigit::getNumPixels())),
                     SegmentedDigit((offset * digits * SegmentedDigit::getNumPixels()) + (9 * SegmentedDigit::getNumPixels()))
    }) {
    }

    void draw(const int value, const Color color) const {
      Serial.print("drawing display: "); Serial.print(value); Serial.print(" Num Digits: "); Serial.println(numDigits);
      int placeValue = value;
      for (int i = numDigits; i > 0; i--) {
        digits[i - 1].draw(placeValue % 10, color);
        placeValue /= 10;
      }
    }
};

/**
 * A numeric display with multiple digits that is to be mirroed one or more times
 */
class MirroredDisplay {
  private:
    // The number of displays to be mirrored.  Should be at least one.
    const int displayCount;

    // The array of displays
    const MultiDigitDisplay* displayList;

  public:
    MirroredDisplay(const MultiDigitDisplay *displayList, const int displayCount) :
      displayCount(displayCount),
      displayList(displayList) {
    }

    int draw(const int value, const Color color) const {
      for (int i = 0; i < displayCount; i++) {
        Serial.print("Drawing Mirrored Display "); Serial.print(i); Serial.print(": value="); Serial.println(value);
        displayList[i].draw(value, color);
      }
      strip.show();
    }
};

/**
 * Expand this array to add more or less displays
 */
MultiDigitDisplay sides[NUM_DISPLAYS] = {
  MultiDigitDisplay(DIGITS_PER_DISPLAY, 0),
  MultiDigitDisplay(DIGITS_PER_DISPLAY, 1),
  MultiDigitDisplay(DIGITS_PER_DISPLAY, 2)
};

MirroredDisplay mirror(sides, NUM_DISPLAYS);

void decrement()
{
  if (count > 0) {
    count--;
    mirror.draw(8, OFF); //display it
    mirror.draw(count, activeColor); //display it
  }
}

void increment()
{
  if (count < pow(10, DIGITS_PER_DISPLAY) - 1) {
    count++;
    mirror.draw(8, OFF); //display it
    mirror.draw(count, activeColor); //display it
  }
}

void set(int value)
{
  int adjustedValue = count;
  if (count > pow(10, (DIGITS_PER_DISPLAY - 1)))
  {
    int hi = count, n = 0;
    while (hi > 9) {
      hi /= 10;
      ++n;
    }
    for (int i = 0; i < n; i++) hi *= 10;
    int leftRemoved = count -= hi;
    Serial.print("Left Removed: "); Serial.println(leftRemoved);
    adjustedValue = leftRemoved;
  }

  int shiftedLeft = adjustedValue * 10;
  Serial.print("Shifted Left: "); Serial.println(shiftedLeft);

  int valAdded = shiftedLeft + value;
  Serial.print("Value Added: "); Serial.println(valAdded);

  count = valAdded;
  mirror.draw(8, OFF); //display it
  mirror.draw(count, activeColor); //display it
}

void brighter()
{
  bright += 50;
  if (bright > 255)
  {
    bright = 255;
  }
  strip.setBrightness(bright);
  mirror.draw(8, OFF); //display it
  mirror.draw(count, activeColor); //display it
}

void dimmer()
{
  bright -= 50;
  if (bright < 25)
  {
    bright = 25;
  }
  strip.setBrightness(bright);
  mirror.draw(8, OFF); //display it
  mirror.draw(count, activeColor); //display it
}

void setColor(Color& color)
{
  activeColor.r = color.r;
  activeColor.g = color.g;
  activeColor.b = color.b;
  mirror.draw(8, OFF); //display it
  mirror.draw(count, activeColor); //display it
}

void off()
{
  power = POWER_OFF;
  mirror.draw(8, OFF); //display it
}

void changeRed(int amount) {
  activeColor.r += amount;
  if (activeColor.r > 255) {
    activeColor.r = 255;
  }
  if (activeColor.r < 0) {
    activeColor.r = 0;
  }
  mirror.draw(8, OFF); //display it
  mirror.draw(count, activeColor); //display it
}
void changeGreen(int amount) {
  activeColor.g += amount;
  if (activeColor.g > 255) {
    activeColor.g = 255;
  }
  if (activeColor.g < 0) {
    activeColor.g = 0;
  }
  mirror.draw(8, OFF); //display it
  mirror.draw(count, activeColor); //display it
}
void changeBlue(int amount) {
  activeColor.b += amount;
  if (activeColor.b > 255) {
    activeColor.b = 255;
  }
  if (activeColor.b < 0) {
    activeColor.b = 0;
  }
  mirror.draw(8, OFF); //display it
  mirror.draw(count, activeColor); //display it
}

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.
void setup() {
  delay(500); //pause a moment to let capacitors on board settle
  Serial.begin(9600);

  strip.begin();
  strip.setBrightness(bright);
  strip.show(); // Initialize all pixels to 'off'
  delay(200);

  //flash eights 3 times
  for (int i = 0; i < 3; i++) {
    mirror.draw(8, GREEN); //display it
    delay(500);
    mirror.draw(8, OFF); //clear it
    delay(500);
  }

  mirror.draw(count, RED); //display lap count


  // In case the interrupt driver crashes on setup, give a clue
  // to the user what's going on.
  Serial.println("Enabling IRin");
  irrecv.enableIRIn(); // Start the receiver
  Serial.println("Enabled IRin");
}

uint32_t lastValue = 0;
void loop() {
  if (irrecv.getResults()) {
    decoder.decode();
    
    //decoder.dumpResults(true); // Uncomment this to dump the IR Codes of a remote

    uint32_t doValue = decoder.value;
    if (0xFFFFFFFF == decoder.value && lastValue != 0) { // All F's if a button is held down
      doValue = lastValue;
    }
    else {
      lastValue = 0;
    }

    switch (doValue) {
      case 0x61A050AF: // Up arrow
        Serial.println("Up Arrow");
        increment();
        break;
      case 0x61A0D02F: // Down Arrow
        Serial.println("Down Arrow");
        decrement();
        break;
      case 0x61A000FF: // 1
        Serial.println("Number 1");
        set(1);
        break;
      case 0x61A0807F: // 2
        Serial.println("Number 2");
        set(2);
        break;
      case 0x61A040BF: // 3
        Serial.println("Number 3");
        set(3);
        break;
      case 0x61A0C03F: // 4
        Serial.println("Number 4");
        set(4);
        break;
      case 0x61A020DF: // 5
        Serial.println("Number 5");
        set(5);
        break;
      case 0x61A0A05F: // 6
        Serial.println("Number 6");
        set(6);
        break;
      case 0x61A0609F: // 7
        Serial.println("Number 7");
        set(7);
        break;
      case 0x61A0E01F: // 8
        Serial.println("Number 8");
        set(8);
        break;
      case 0x61A010EF: // 9
        Serial.println("Number 9");
        set(9);
        break;
      case 0x61A0906F: // 0
        Serial.println("Number 0");
        set(0);
        break;
      case 0x61A018E7: // Enter
        Serial.println("OK");
        decrement();
        break;
      case 0xFF3AC5: // Increase Brightness
        Serial.println("Increase Brightness");
        brighter();
        break;
      case 0xFFBA45: // Decrease Brightness
        Serial.println("Decrease Brightness");
        dimmer();
        break;
      case 0xFF1AE5: // Red
        Serial.println("Red");
        setColor(reds[0]);
        break;
      case 0xFF2AD5: // Red + little green
        Serial.println("Red + little green");
        setColor(reds[1]);
        break;
      case 0xFF0AF5: // Red + more green
        Serial.println("Red + more green");
        setColor(reds[2]);
        break;
      case 0xFF38C7: // Red + alot green
        Serial.println("Red + alot green");
        setColor(reds[3]);
        break;
      case 0xFF18E7: // Red + all green
        Serial.println("Red + all green");
        setColor(reds[4]);
        break;
      case 0xFF9A65: // Green
        Serial.println("Green");
        setColor(greens[0]);
        break;
      case 0xFFAA55: // Green + little Blue
        Serial.println("Green + little Blue");
        setColor(greens[1]);
        break;
      case 0xFF8A75: // Green + more Blue
        Serial.println("Green + more Blue");
        setColor(greens[2]);
        break;
      case 0xFFB847: // Green + alot Blue
        Serial.println("Green + alot Blue");
        setColor(greens[3]);
        break;
      case 0xFF9867: // Green + all Blue
        Serial.println("Green + all Blue");
        setColor(greens[4]);
        break;
      case 0xFFA25D: // Blue
        Serial.println("Blue");
        setColor(blues[0]);
        break;
      case 0xFF926D: // Blue + little Red
        Serial.println("Blue + little Red");
        setColor(blues[1]);
        break;
      case 0xFFB24D: // Blue + more Red
        Serial.println("Blue + more Red");
        setColor(blues[2]);
        break;
      case 0xFF7887: // Blue + alot Red
        Serial.println("Blue + alot Red");
        setColor(blues[3]);
        break;
      case 0xFF58A7: // Blue + all Red
        Serial.println("Blue + all Red");
        setColor(blues[4]);
        break;
      case 0xFF22DD: // White
        Serial.println("White");
        setColor(brights[0]);
        break;
      case 0xFF12ED: // White - little Red
        Serial.println("White - little Red");
        setColor(brights[1]);
        break;
      case 0xFF32CD: // White - little Orange
        Serial.println("White - little Orange");
        setColor(brights[2]);
        break;
      case 0xFFF807: // White - little Green
        Serial.println("White - little Green");
        setColor(brights[3]);
        break;
      case 0xFFD827: // White - little Blue
        Serial.println("White - little Blue");
        setColor(brights[4]);
        break;
      case 0xFF28D7: // Increase Red
        Serial.println("Increase Red");
        changeRed(5);
        lastValue = 0xFF28D7;
        break;
      case 0xFF08F7: // Decrease Red
        Serial.println("Decrease Red");
        changeRed(-5);
        lastValue = 0xFF08F7;
        break;
      case 0xFFA857: // Increase Green
        Serial.println("Increase Green");
        changeGreen(5);
        lastValue = 0xFFA857;
        break;
      case 0xFF8877: // Decrease Green
        Serial.println("Decrease Green");
        changeGreen(-5);
        lastValue = 0xFF8877;
        break;
      case 0xFF6897: // Increase Blue
        Serial.println("Increase Blue");
        changeBlue(5);
        lastValue = 0xFF6897;
        break;
      case 0xFF48B7: // Decrease Blue
        Serial.println("Decrease Blue");
        changeBlue(-5);
        lastValue = 0xFF48B7;
        break;
      case 0xFF02FD: // Power Toggle
      case 0x61A0F00F:
        Serial.println("Power Toggle");
        if (power == POWER_ON) {
          off();
        }
        else {
          setColor(activeColor);
          power = POWER_ON;
        }
        break;
      default:
        Serial.println(doValue, HEX);
        break;
    };

    irrecv.enableIRIn(); // Receive the next value
  }
}
