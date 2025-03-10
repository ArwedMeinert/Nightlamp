#include <Adafruit_NeoPixel.h>
#include <Encoder.h>
#include <EEPROM.h>

// ----- Pin Definitions for ESP8266 (NodeMCU Example) -----
// NeoPixel data pin (using D5, which corresponds to GPIO14)
#define PIXEL_PIN D5
#define NUMPIXELS 115
Adafruit_NeoPixel pixels(NUMPIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Encoder pins (using D6 = GPIO12 and D7 = GPIO13)
Encoder myEnc(D6, D7);

// External button pins (example: D1 = GPIO5, D2 = GPIO4, D0 = GPIO16)
const int btnPins[3] = {D1, D2, D4};  // External buttons 1, 2, 3

// Encoder push button pin (example: D3 = GPIO0)
const int encoderButtonPin = D3;       

// ----- Other Definitions -----
const unsigned long longPressThreshold = 1000; // in milliseconds

unsigned long btnPressStart[3] = {0, 0, 0};
bool btnLongPressHandled[3] = {false, false, false};
bool btnPreviousState[3] = {HIGH, HIGH, HIGH};
bool eepromWritten[3] = {false, false, false};

bool encoderButtonPrevious = HIGH;

// For tracking encoder rotation
long oldPosition = -999;

// Define a simple Color structure for clarity
struct Color {
  byte r;
  byte g;
  byte b;
};

Color currentColor = {255, 0, 0};  // Default red
int currentChannel = 0;            // 0 = red, 1 = green, 2 = blue
int activeButton = 0;              // Which button's color is active

Color savedColors[3];      // Stored colors (recall on short press)
Color originalColors[3];   // Original (bright) colors used during dimming

// ----- Helper Functions -----

// Update every pixel to show the current color.
void updatePixels() {
  pixels.clear();
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(currentColor.r, currentColor.g, currentColor.b));
  }
  pixels.show();
}

// Load a color from EEPROM for the given button index (each uses 3 bytes).
Color loadColor(int index) {
  Color col;
  col.r = EEPROM.read(index * 3);
  col.g = EEPROM.read(index * 3 + 1);
  col.b = EEPROM.read(index * 3 + 2);
  return col;
}

// Save a color to EEPROM for the given button index.
void saveColorEEPROM(int index, Color col) {
  EEPROM.write(index * 3, col.r);
  EEPROM.write(index * 3 + 1, col.g);
  EEPROM.write(index * 3 + 2, col.b);
  EEPROM.commit();  // Ensure the data is saved on ESP8266
}

// Smoothly transition from one color to another.
void smoothTransition(Color start, Color end, int steps, int delayTime) {
  for (int i = 0; i <= steps; i++) {
    float ratio = i / (float)steps;
    Color temp;
    temp.r = start.r + (end.r - start.r) * ratio;
    temp.g = start.g + (end.g - start.g) * ratio;
    temp.b = start.b + (end.b - start.b) * ratio;
    currentColor = temp;
    updatePixels();
    delay(delayTime);
  }
}

// ----- Setup -----
void setup() {
  
  Serial.begin(9600);
  Serial.println("Advanced NeoPixel & Encoder Control (ESP8266) - Debug Mode");

  EEPROM.begin(512);  // Initialize EEPROM (size in bytes)

  // Initialize button pins with internal pull-ups.
  for (int i = 0; i < 3; i++) {
    pinMode(btnPins[i], INPUT_PULLUP);
  }
  pinMode(encoderButtonPin, INPUT_PULLUP);

  pixels.begin();
  pixels.clear();
  pixels.setBrightness(90);
  // Load saved colors from EEPROM; if empty (all zeros), set defaults.
  for (int i = 0; i < 3; i++) {
    savedColors[i] = loadColor(i);
    if (savedColors[i].r == 0 && savedColors[i].g == 0 && savedColors[i].b == 0) {
      if (i == 0) savedColors[i] = {255, 0, 0};      // Default red
      else if (i == 1) savedColors[i] = {0, 255, 0};   // Default green
      else if (i == 2) savedColors[i] = {0, 0, 255};   // Default blue
      saveColorEEPROM(i, savedColors[i]);
    }
    originalColors[i] = savedColors[i];
  }

  // At startup, display the saved color for button 1.
  activeButton = 0;
  currentColor = savedColors[0];
  updatePixels();
}

// ----- Main Loop -----
void loop() {

  Serial.println(digitalRead(btnPins[0]));
  Serial.println(digitalRead(btnPins[1]));
  Serial.println(digitalRead(btnPins[2]));
  // --- Encoder Rotation for Color Adjustment ---
  long newPosition = myEnc.read();
  if (newPosition != oldPosition) {
    int diff = newPosition - oldPosition;
    oldPosition = newPosition;
    if (currentChannel == 0) {
      int newVal = currentColor.r + diff;
      currentColor.r = constrain(newVal, 0, 255);
    } else if (currentChannel == 1) {
      int newVal = currentColor.g + diff;
      currentColor.g = constrain(newVal, 0, 255);
    } else if (currentChannel == 2) {
      int newVal = currentColor.b + diff;
      currentColor.b = constrain(newVal, 0, 255);
    }
    updatePixels();

  }

  // --- Encoder Push Button to Cycle Color Channel ---
  bool encButtonState = digitalRead(encoderButtonPin);
  if (encButtonState == LOW && encoderButtonPrevious == HIGH) {
    currentChannel = (currentChannel + 1) % 3;
    Serial.print("Encoder button pressed. New channel: ");
    if (currentChannel == 0) Serial.println("Red");
    else if (currentChannel == 1) Serial.println("Green");
    else if (currentChannel == 2) Serial.println("Blue");
    delay(200);  // debounce delay
  }
  encoderButtonPrevious = encButtonState;

  // --- External Buttons: Long Press for Dimming/Save and Short Press for Recall ---
  for (int i = 0; i < 3; i++) {
    bool btnState = digitalRead(btnPins[i]);

    // When the button is first pressed.
    if (btnState == LOW && btnPreviousState[i] == HIGH) {
      btnPressStart[i] = millis();
      btnLongPressHandled[i] = false;
      eepromWritten[i] = false;  // reset EEPROM flag for this press
    }
    // If the button is held down.
    if (btnState == LOW) {
      unsigned long pressDuration = millis() - btnPressStart[i];
      
      // Check for long press.
      if (!btnLongPressHandled[i] && pressDuration > longPressThreshold) {
        btnLongPressHandled[i] = true;
        Serial.print("Button ");
        Serial.print(i);
        Serial.println(" long press detected.");
        if (activeButton != i) {
          activeButton = i;
          currentColor = savedColors[i];
          updatePixels();
        }
        originalColors[i] = currentColor;
      }
      // During long press mode, gradually dim the pixels.
      if (btnLongPressHandled[i]) {
        if (currentColor.r > 0 || currentColor.g > 0 || currentColor.b > 0) {
          currentColor.r = (currentColor.r > 5 ? currentColor.r - 5 : 0);
          currentColor.g = (currentColor.g > 5 ? currentColor.g - 5 : 0);
          currentColor.b = (currentColor.b > 5 ? currentColor.b - 5 : 0);
          updatePixels();
          delay(50);
        } else {
          if (!eepromWritten[i]) {
            savedColors[i] = originalColors[i];
            saveColorEEPROM(i, savedColors[i]);
            Serial.print("Saved color for button ");
            Serial.print(i + 1);
            Serial.println(" to EEPROM.");
            eepromWritten[i] = true;
          }
        }
      }
    }
    // On button release...
    if (btnState == HIGH && btnPreviousState[i] == LOW) {
      unsigned long pressDuration = millis() - btnPressStart[i];
      if (pressDuration < longPressThreshold) {
        if (activeButton == i) {
          if (currentColor.r != 0 || currentColor.g != 0 || currentColor.b != 0)
            smoothTransition(currentColor, {0, 0, 0}, 20, 20);
          else
            smoothTransition(currentColor, savedColors[i], 20, 20);
          activeButton = i;
        } else {

          smoothTransition(currentColor, savedColors[i], 20, 20);
          activeButton = i;
        }
      }
      btnLongPressHandled[i] = false;
    }
    btnPreviousState[i] = btnState;
  }
}



