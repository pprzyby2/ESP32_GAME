#include <SPI.h>
#include <Adafruit_GFX.h>
#include <SSD1322_for_Adafruit_GFX.h>
#include <ESP32Encoder.h>
#include <IRremote.hpp>


// Pin definitions (adjust as needed based on your wiring)
#define OLED_CS   5      // Chip Select
#define OLED_DC   2      // Data/Command
#define OLED_RST  4      // Reset
#define OLED_MOSI 23     // Master Out Slave In (SDA in some conventions)
#define OLED_SCLK 18     // Serial Clock
#define ENCODER_PIN_A 32 // GPIO pin for encoder A
#define ENCODER_PIN_B 33 // GPIO pin for encoder B
#define IR_RECV_PIN 35   // GPIO pin for IR receiver 1838

Adafruit_SSD1322 display(256, 64, OLED_MOSI, OLED_SCLK, OLED_DC, OLED_RST, OLED_CS);
ESP32Encoder encoder;

// IR Remote codes for numeric keypad (you may need to adjust these codes)
// Press buttons on your remote and check Serial output to get actual codes
const uint32_t IR_KEY_0 = 0xE619FF00;
const uint32_t IR_KEY_1 = 0xBA45FF00;
const uint32_t IR_KEY_2 = 0xB946FF00;
const uint32_t IR_KEY_3 = 0xB847FF00;
const uint32_t IR_KEY_4 = 0xBB44FF00;
const uint32_t IR_KEY_5 = 0xBF40FF00;
const uint32_t IR_KEY_6 = 0xBC43FF00;
const uint32_t IR_KEY_7 = 0xF807FF00;
const uint32_t IR_KEY_8 = 0xEA15FF00;
const uint32_t IR_KEY_9 = 0xF609FF00;
const uint32_t IR_KEY_OK = 0xE31CFF00;
const uint32_t IR_KEY_STAR = 0xE916FF00;
const uint32_t IR_KEY_HASH = 0xF20DFF00;
const uint32_t IR_KEY_UP = 0xE718FF00;
const uint32_t IR_KEY_DOWN = 0xAD52FF00;
const uint32_t IR_KEY_LEFT = 0xF708FF00;
const uint32_t IR_KEY_RIGHT = 0xA55AFF00;

int lastIRKey = -1;  // Store last pressed key
unsigned long lastIRTime = 0;  // Debounce timing

void setup() {
  Serial.begin(115200);

  // Initialize SSD1322 display
  if ( ! display.begin(0x3D) ) {
     Serial.println("Unable to initialize OLED");
     while (1) yield();
  }

  display.display(); // Display initial buffer (blank)
  delay(2000);       // Pause

  // Clear the buffer
  display.clearDisplay();
  display.setCursor(27, 20);
  display.clearDisplay();
  display.setTextSize(3);
  display.println("Nacisnij OK");

  // Update the display to show all changes
  display.display();

  encoder.attachHalfQuad ( ENCODER_PIN_A, ENCODER_PIN_B );
  encoder.setCount (128);

  // Initialize IR receiver
  IrReceiver.begin(IR_RECV_PIN, DISABLE_LED_FEEDBACK);
  Serial.println("IR Receiver initialized on pin " + String(IR_RECV_PIN));
  Serial.println("Press remote buttons to see their codes...");
}

int positionX = 0;
int positionY = 0;
bool directionX = true; // true for right, false for left
bool directionY = true; // true for down, false for up
const int encScale = 4;
bool dead = false;
int loopNum = 0;
int speed = 30;

// Handle IR remote keypad input
int handleIRKeypad() {
  if (IrReceiver.decode()) {
    uint32_t code = IrReceiver.decodedIRData.decodedRawData;
    
    // Print received code for debugging
    if (code != 0) {
      Serial.print("IR Code received: 0x");
      Serial.println(code, HEX);
    }
    
    int key = -1;
    
    // Decode numeric keypad
    if (code == IR_KEY_0) key = 0;
    else if (code == IR_KEY_1) key = 1;
    else if (code == IR_KEY_2) key = 2;
    else if (code == IR_KEY_3) key = 3;
    else if (code == IR_KEY_4) key = 4;
    else if (code == IR_KEY_5) key = 5;
    else if (code == IR_KEY_6) key = 6;
    else if (code == IR_KEY_7) key = 7;
    else if (code == IR_KEY_8) key = 8;
    else if (code == IR_KEY_9) key = 9;
    else if (code == IR_KEY_OK) key = 10;    // OK/Enter button
    else if (code == IR_KEY_STAR) key = 11;  // * button
    else if (code == IR_KEY_HASH) key = 12;  // # button
    else if (code == IR_KEY_UP) key = 13;    // Up button
    else if (code == IR_KEY_DOWN) key = 14;  // Down button
    else if (code == IR_KEY_LEFT) key = 15;  // Left button
    else if (code == IR_KEY_RIGHT) key = 16; // Right button
    
    if (key != -1) {
      Serial.print("Key pressed: ");
      if (key <= 9) Serial.println(key);
      else if (key == 10) Serial.println("OK");
      else if (key == 11) Serial.println("*");
      else if (key == 12) Serial.println("#");
      else if (key == 13) Serial.println("UP");
      else if (key == 14) Serial.println("DOWN");
      else if (key == 15) Serial.println("LEFT");
      else if (key == 16) Serial.println("RIGHT");
      
      lastIRKey = key;
      lastIRTime = millis();
    }
    
    IrReceiver.resume(); // Ready to receive next value
    return key;
  }
  return -1;
}

void loop() {
  // Check for IR remote input
  int irKey = handleIRKeypad();
  if (irKey >= 0) {
    // Handle IR key press (example: restart game with OK button)
    if (irKey == 10 && dead) {  // OK button restarts game
      dead = false;
      positionX = 0;
      positionY = 0;
      directionX = true;
      directionY = true;
      speed = 30;
      loopNum = 0;
      Serial.println("Game restarted!");
    }
  }

  if (dead) {
    display.setCursor(47, 20);
    display.clearDisplay();
    display.setTextSize(3);
    display.println("Game Over");
    display.display();
    return;
  }
  if (speed <= 5) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(37, 10);
    display.println("ZWYCIEZTWO!");
    display.setCursor(37, 30);
    display.println("Uzyj kodu: 1234");
    display.display();
    return;
  }

  long encoderValue = encoder.getCount() / encScale;
  if (encoderValue > 216) encoder.setCount ( 216 * encScale );
  if (encoderValue < 0) encoder.setCount ( 0 );
  // Clear the display
  display.clearDisplay();

  display.setCursor(0,0);
  display.setTextSize(1);
  display.println("Pos X: " + String(positionX));
  display.println("Pos Y: " + String(positionY));
  display.println("Enc: " + String(encoderValue));
  display.println("Spd: " + String(speed));

  // Draw the moving object
  //display.fillRect(positionX, positionY, 20, 20, SSD1322_WHITE);
  display.drawCircle(positionX + 10, positionY + 10, 10, SSD1322_WHITE); // Draw circle outline
  display.drawCircle(positionX + 10, positionY + 10, 8, SSD1322_WHITE); // Draw circle outline
  display.drawCircle(positionX + 10, positionY + 10, 6, SSD1322_WHITE); // Draw circle outline
  display.fillCircle(positionX + 10, positionY + 10, 4, SSD1322_WHITE); // Draw circle outline
  display.fillRect(encoderValue, 60, 40, 4, SSD1322_WHITE);

  // Update the display
  display.display();

  // Move the object
  if (directionX) {
    if (speed < 10) {
      positionX += int(20/speed);
    } else {
      positionX += 2;
    }
    
    if (positionX > 240) {
      positionX = 240;
      directionX = false;
    }
  } else {
    if (speed < 10) {
      positionX -= int(20/speed);
    } else {
      positionX -= 2;
    }
    if (positionX < 0) {
      positionX = 0;
      directionX = true;
    }
  }
  if (directionY) {
    positionY += 1;
    if (positionY == 40 && positionX + 10 >= encoderValue && positionX - 10 <= encoderValue + 40) {
      directionY = false; // Reverse direction if hitting the "paddle"
    }
    if (positionY > 44) {
      positionY = 44;
      dead = true;
    }
  } else {
    positionY -= 1;
    if (positionY < 0) {
      positionY = 0;
      directionY = true;
    }
  }

  if (loopNum % 100 == 0 && speed > 1) {
    speed -= 1; // Increase speed every 100 loops, down to a minimum delay of 10ms
  }

  delay(speed);
  loopNum++;
}