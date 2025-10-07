#include <SPI.h>
#include <Adafruit_GFX.h>
#include <SSD1322_for_Adafruit_GFX.h>
#include <ESP32Encoder.h>


// Pin definitions (adjust as needed based on your wiring)
#define OLED_CS   5      // Chip Select
#define OLED_DC   2      // Data/Command
#define OLED_RST  4      // Reset
#define OLED_MOSI 23     // Master Out Slave In (SDA in some conventions)
#define OLED_SCLK 18     // Serial Clock
#define ENCODER_PIN_A 32 // GPIO pin for encoder A
#define ENCODER_PIN_B 33 // GPIO pin for encoder B

Adafruit_SSD1322 display(256, 64, OLED_MOSI, OLED_SCLK, OLED_DC, OLED_RST, OLED_CS);
ESP32Encoder encoder;

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

  // Draw text
  display.setTextSize(1);
  display.setTextColor(SSD1322_WHITE);
  display.setCursor(0,0);
  display.println("Hello ESP32!");

  // Draw a rectangle
  display.drawRect(10, 20, 100, 30, SSD1322_WHITE);

  // Draw a filled circle
  display.fillCircle(64, 40, 10, SSD1322_WHITE);

  // Update the display to show all changes
  display.display();

  encoder.attachHalfQuad ( ENCODER_PIN_A, ENCODER_PIN_B );
  encoder.setCount (128);
}

int positionX = 0;
int positionY = 0;
bool directionX = true; // true for right, false for left
bool directionY = true; // true for down, false for up
const int encScale = 4;
bool dead = false;
int loopNum = 0;
int speed = 30;

void loop() {
  if (dead) {
    display.setCursor(47, 20);
    display.clearDisplay();
    display.setTextSize(3);
    display.println("Game Over");
    display.display();
    return;
  }

  long encoderValue = encoder.getCount() / encScale;
  if (encoderValue > 216) encoder.setCount ( 216 * encScale );
  if (encoderValue < 0) encoder.setCount ( 0 );
  // Clear the display
  display.clearDisplay();

  display.setCursor(0,0);
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