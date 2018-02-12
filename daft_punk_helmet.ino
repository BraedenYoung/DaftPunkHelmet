#include <FastLED.h>

#define LED_PIN 12
#define BRIGHTNESS_PIN A7
#define CHANGE_EFFECT_PIN 2

#define FAN_PIN 11
#define FAN_SPEED_PIN A0

#define RIGHT_LED_PIN 3
#define RIGHT_LED_WAIT 2
#define RIGHT_MAX_BRIGHTNESS 120

#define LEFT_FRONT_LED_PIN 5
#define LEFT_BACK_LED_PIN 4

#define COLOR_ORDER GRB
#define CHIPSET     WS2811
#define NUM_LEDS    64
#define FRAMES_PER_SECOND 60
#define DEBOUNCE_DELAY 50 
#define NUMBER_OF_PATTERNS 2

#define SPACE_DELAY 15
#define TONE_DELAY 10
#define SHORT_TONE 3
#define LONG_TONE  8

String DAFT = "-..#.-#..-.#-#";
String PUNK = ".--.#..-#-.#-.-#";

unsigned long lastDebounceTime = 0;

int effectButtonState; 
int lastEffectButtonState = LOW;

// Control variable for fire pattern
bool gReverseDirection = false;

// The brightness of all of the LEDs
int8_t brightness = 20; // Normal 200

int rightLEDBrightness = 0; 
int rightLEDFade = 5;
int rightLEDLastChange = 0;

int leftFrontStage = 0;
int leftFrontDelay = 0;
bool leftFrontState = false;

int leftBackStage = 0;
int leftBackDelay = 0;
bool leftBackState = false;

// Index number of which pattern is current
int8_t gCurrentPatternNumber = 0; 

CRGB RAINBOW[8]={
  CRGB::Purple,
  CRGB::Blue, 
  CRGB::Aqua,
  CRGB::SeaGreen,
  CRGB::Green,
  CRGB::Yellow,
  CRGB::Orange,
  CRGB::Red,
};

CRGB leds[NUM_LEDS];


void setup() {
  delay(3000); // sanity delay
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( brightness );

  pinMode(CHANGE_EFFECT_PIN, INPUT);

  pinMode(RIGHT_LED_PIN, OUTPUT);
}


// List of patterns to cycle through. Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { 
  rainbow, 
  fire,
};


void loop()
{
  checkBrightness();
  checkPatternChangePressed();

  adjustFanSpeed();

  setSideLedsBrightnesses();
  
  // Call the current pattern function once, updating the 'leds' array
  gPatterns[gCurrentPatternNumber]();

  FastLED.show(); // display this frame
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}

void setSideLedsBrightnesses () 
{
  // RIGHT SIDE

  if (rightLEDLastChange < RIGHT_LED_WAIT)
  {
    rightLEDLastChange += 1;
    return;
  }
  
  rightLEDLastChange = 0;
  
  analogWrite(RIGHT_LED_PIN, rightLEDBrightness);
  rightLEDBrightness = rightLEDBrightness + rightLEDFade;

  if (rightLEDBrightness <= 0 || rightLEDBrightness > RIGHT_MAX_BRIGHTNESS) {
    rightLEDFade = -rightLEDFade;
  }

  // LEFT SIDE

  if (leftFrontDelay == 0) {
    
     if (leftFrontState == true){
        leftFrontDelay = TONE_DELAY;
        leftFrontState = false;
     } else {
      if (DAFT.charAt(leftFrontStage) == '.') {
        leftFrontDelay = SHORT_TONE;
        leftFrontState = true;
      } else if (DAFT.charAt(leftFrontStage) == '-') {
        leftFrontDelay = LONG_TONE;
        leftFrontState = true;
      } else {
        // Space
        leftFrontDelay = SPACE_DELAY;
        leftFrontState = false;
      }
      leftFrontStage = (leftFrontStage + 1) % DAFT.length();
     }
     digitalWrite(LEFT_FRONT_LED_PIN, leftFrontState);
  }
  
  leftFrontDelay = leftFrontDelay - 1;
  
  if (leftBackDelay == 0) {

   if (leftBackState == true){
      leftBackDelay = TONE_DELAY;
      leftBackState = false;
   } else {
    if (PUNK.charAt(leftBackStage) == '.') {
      leftBackDelay = SHORT_TONE;
      leftBackState = true;
      
    } else if (PUNK.charAt(leftBackStage) == '-') {
      leftBackDelay = LONG_TONE;
      leftBackState = true;
      
    } else {
      // Space
      leftBackDelay = SPACE_DELAY;
      leftBackState = false;
    }
    leftBackStage = (leftBackStage + 1) % PUNK.length();
   }
   digitalWrite(LEFT_BACK_LED_PIN, leftBackState);
  }
  
  leftBackDelay = leftBackDelay - 1;
}


void checkBrightness()
{
  // Check brightness level
  int potentiometerReading = analogRead(BRIGHTNESS_PIN);
  int newBrightness = map(potentiometerReading, 0, 1023, 0, 255);
  if (newBrightness != brightness) {
    brightness = newBrightness;
    FastLED.setBrightness(newBrightness);
  }
}


void adjustFanSpeed()
{
  // Check brightness level
  int potentiometerReading = analogRead(FAN_SPEED_PIN);
  
  int currSpeed = map(potentiometerReading, 0, 1023, 0, 255);
  analogWrite(FAN_PIN, currSpeed);
  
}

void checkPatternChangePressed()
{
  int effectButtonReading = digitalRead(CHANGE_EFFECT_PIN);
  if (effectButtonReading != lastEffectButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    // Check change in pattern level
    
    if (effectButtonState != effectButtonReading) {
      effectButtonState = effectButtonReading;
      
      if (effectButtonReading == HIGH) {
        gCurrentPatternNumber = (gCurrentPatternNumber + 1) % NUMBER_OF_PATTERNS;
      }
    }
  }
  lastEffectButtonState = effectButtonReading;
}


// ===============================================
//
//             LIGHT DISPLAY PATTERNS
//
// ===============================================


// Default color rainbow color display 
void rainbow()
{
    for( int pixelnumber = 0; pixelnumber < NUM_LEDS/2; pixelnumber++) {
      leds[pixelnumber] = RAINBOW[(pixelnumber / 4) % 8];
      leds[NUM_LEDS - pixelnumber - 1] = RAINBOW[(pixelnumber / 4) % 8];
    }
}


// Duplicated from Fire2012 by Mark Kriegsman slightly modified

#define COOLING  55
#define SPARKING 120

void fire()
{
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for( int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }
  
  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if( random8() < SPARKING ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160,255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for( int j = 0; j < NUM_LEDS/2 + 1; j++) {
    CRGB color = HeatColor( heat[j]);
    int pixelnumber;
    if( gReverseDirection ) {
      pixelnumber = (NUM_LEDS-1) - j;
    } else {
      pixelnumber = j;
    }
    
    leds[(NUM_LEDS/2) - pixelnumber] = color;
    leds[(NUM_LEDS/2) + pixelnumber - 1] = color; 
  }
}

