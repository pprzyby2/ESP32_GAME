#include <SPI.h>
#include <Adafruit_GFX.h>
#include <SSD1322_for_Adafruit_GFX.h>
#include <ESP32Encoder.h>
#include <IRremote.hpp>
#include <vector>
#include "consts.h"
#include "sound_game.hpp"

Adafruit_SSD1322 display(256, 64, OLED_MOSI, OLED_SCLK, OLED_DC, OLED_RST, OLED_CS);
ESP32Encoder encoder;

int lastIRKey = -1;  // Store last pressed key
unsigned long lastIRTime = 0;  // Debounce timing



GameState state = WAITING_TO_START;

void setup() {
  Serial.begin(115200);

  // Initialize SSD1322 display
  if ( ! display.begin(0x3D) ) {
     Serial.println("Unable to initialize OLED");
     while (1) yield();
  }

  display.display(); // Display initial buffer (blank)
  delay(2000);       // Pause

  encoder.attachHalfQuad ( ENCODER_PIN_A, ENCODER_PIN_B );
  encoder.setCount (128);

  // Initialize IR receiver
  IrReceiver.begin(IR_RECV_PIN, DISABLE_LED_FEEDBACK);
  Serial.println("IR Receiver initialized on pin " + String(IR_RECV_PIN));
  Serial.println("Press remote buttons to see their codes...");

  // Initialize LM393 noise sensor
  pinMode(LM393_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(LM393_PIN), soundISR, FALLING);
  Serial.println("LM393 Sound Sensor initialized on pin " + String(LM393_PIN));

  // Initialize buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.println("Buzzer initialized on pin " + String(BUZZER_PIN));
}

int positionX = 0;
int positionY = 0;
bool directionX = true; // true for right, false for left
bool directionY = true; // true for down, false for up
const int encScale = 4;
bool dead = false;
int loopNum = 0;
int speed = 30;
bool gameStarted = false;

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

int secretCode[] = {1, 2, 3, 4};
int codeIndex = 0;

void startCodeEntry() {
  codeIndex = 0;
  state = CODE_ENTRY;
}

void codeEntryLoop() {
  int irKey = handleIRKeypad();
  if (irKey >= 0 && irKey <= 9) {
    if (irKey == secretCode[codeIndex]) {
      codeIndex++;
      playConfirmSound();
      if (codeIndex >= sizeof(secretCode)/sizeof(secretCode[0])) {
        // Code entered successfully
        display.clearDisplay();
        display.setTextSize(2);
        display.setCursor(20, 20);
        display.println("Kod poprawny!");
        display.display();
        delay(2000);
        state = CLAP_GAME;
        codeIndex = 0;
      }
    } else {
      // Incorrect code
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(20, 20);
      display.println("Kod niepoprawny!");
      display.display();
      delay(2000);
      codeIndex = 0; // Reset code index
    }
  } else {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(20, 20);
    display.printf("Kod: %d %d %d %d", 
      codeIndex > 0 ? secretCode[0] : 0,
      codeIndex > 1 ? secretCode[1] : 0,
      codeIndex > 2 ? secretCode[2] : 0,
      codeIndex > 3 ? secretCode[3] : 0);
    display.display();    
  }
}

void printCentered(const String &text, int y, int textSize) {
  int16_t x1, y1;
  uint16_t w, h;
  display.setTextSize(textSize);
  display.getTextBounds(text, 0, y, &x1, &y1, &w, &h);
  int16_t x = (display.width() - w) / 2;
  display.setCursor(x, y);
  display.println(text);
}

void waitForStart() {
  display.clearDisplay();
  printCentered("Nacisnij OK by zaczac", 20, 2);
  display.display();
  int irKey = handleIRKeypad();
  if (irKey == 10) { // OK button
    playGameStartSound();
    state = STARTING_MESSAGE;
    Serial.println("Game started!");
  }
}

void startFirstGame() {
  dead = false;
  positionX = 0;
  positionY = 0;
  directionX = true;
  directionY = true;
  speed = 30;
  loopNum = 0;
  gameStarted = true;  
  state = RUNNING;
  Serial.println("First game started!");
}

int startingMessagePart = 0;
void showStartingMessage() {
  display.clearDisplay();
  if (startingMessagePart == 0) {
    printCentered("Czesc, tu Maly Mikolaj!", 10, 1);
    printCentered("Przygotowalem dla Ciebie gre.", 20, 1);
    printCentered("", 30, 1);
    printCentered("Nacisnij OK by kontynuowac.", 40, 1);
  } else if (startingMessagePart == 1) {
    printCentered("Steruj pilka", 10, 1);
    printCentered("uzywajac pokretla.", 20, 1);
    printCentered("Unikaj spadniecia", 30, 1);
    printCentered("ponizej ekranu.", 40, 1);
  } else if (startingMessagePart == 2) {
    printCentered("Zaczynasz z predkoscia 1.", 10, 1);
    printCentered("Co 100 ruchow,", 20, 1);
    printCentered("predkosc sie zwieksza.", 30, 1);
    printCentered("Musisz dojsc do 30!", 40, 1);
  } else if (startingMessagePart == 3) {
    printCentered("Gdy wygrasz, dostaniesz", 10, 1);
    printCentered("kod do kolejnej gry.", 20, 1);
    printCentered("Powodzenia!", 30, 1);
    printCentered("Nacisnij OK by zaczac.", 40, 1);
  }
  display.display();
  int irKey = handleIRKeypad();
  if (irKey == 10) { // OK button
    playGameStartSound();
    startingMessagePart++;
    if (startingMessagePart >= 4) {
      startFirstGame();
      startingMessagePart = 0;
    }
    Serial.println("Game started!");
  }  
}

void loop() {
  if (state == WAITING_TO_START) {
    waitForStart();
  } else if (state == STARTING_MESSAGE) {
    showStartingMessage();
  } else if (state == GAME_OVER) {
    // Check for IR remote input
    int irKey = handleIRKeypad();
    if (irKey >= 0) {
      // Handle IR key press (example: restart game with OK button)
      if (irKey == 10) {
        if (dead) {  // OK button restarts game
          startFirstGame();
          return;
        }
        playGameStartSound();
        state = RUNNING;
      }
    }
  } else if (state == CLAP_GAME) {
    // Check for sound sensor input
    soundGame();
  } else if (state == RUNNING) {
    if (dead) {
      display.setCursor(57, 15);
      display.clearDisplay();
      display.setTextSize(2);
      display.println("Game Over"); 
      display.setCursor(47, 40);
      display.println("Nacisnij OK"); 
      display.display();
      state = GAME_OVER;
      return;
    } 
    
    if (!gameStarted) {
      // Wait for game to start
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
      startCodeEntry();
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
    display.println("Spd: " + String(31-speed));

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
  } else if (state == CODE_ENTRY) {
    codeEntryLoop();
    return;
  } else if (state == END_GAME) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(20, 20);
    display.println("GRATULACJE!");
    display.setCursor(10, 40);
    display.println("Koniec gry.");
    display.display();
  }
}