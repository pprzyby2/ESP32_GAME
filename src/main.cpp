#include <SPI.h>
#include <Adafruit_GFX.h>
#include <SSD1322_for_Adafruit_GFX.h>
#include <ESP32Encoder.h>
#include <IRremote.hpp>
#include <vector>


// Pin definitions (adjust as needed based on your wiring)
#define OLED_CS   5      // Chip Select
#define OLED_DC   2      // Data/Command
#define OLED_RST  4      // Reset
#define OLED_MOSI 23     // Master Out Slave In (SDA in some conventions)
#define OLED_SCLK 18     // Serial Clock
#define ENCODER_PIN_A 32 // GPIO pin for encoder A
#define ENCODER_PIN_B 33 // GPIO pin for encoder B
#define IR_RECV_PIN 34   // GPIO pin for IR receiver 1838
#define LM393_PIN 17     // GPIO pin for LM393 noise sensor
#define BUZZER_PIN 14    // GPIO pin for Buzzer

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

const int C4_NOTE = 261; // C4 note frequency
const int D4_NOTE = 294; // D4 note frequency
const int E4_NOTE = 329; // E4 note frequency
const int F4_NOTE = 349; // F4 note frequency
const int G4_NOTE = 392; // G4 note frequency
const int A4_NOTE = 440; // A4 note frequency
const int B4_NOTE = 494; // B4 note frequency  
const int C5_NOTE = 523; // C5 note frequency
const int D5_NOTE = 587; // D5 note frequency
const int E5_NOTE = 659; // E5 note frequency
const int F5_NOTE = 698; // F5 note frequency
const int G5_NOTE = 784; // G5 note frequency
const int A5_NOTE = 880; // A5 note frequency
const int B5_NOTE = 988; // B5 note frequency
const int C6_NOTE = 1047; // C6 note frequency
const int D6_NOTE = 1175; // D6 note frequency
const int E6_NOTE = 1319; // E6 note frequency
const int F6_NOTE = 1397; // F6 note frequency
const int G6_NOTE = 1568; // G6 note frequency
const int A6_NOTE = 1760; // A6 note frequency
const int B6_NOTE = 1976; // B6 note frequency

int lastIRKey = -1;  // Store last pressed key
unsigned long lastIRTime = 0;  // Debounce timing

// LM393 sensor variables
volatile bool soundDetected = false;
volatile unsigned long lastSoundTime = 0;
const unsigned long SOUND_DEBOUNCE = 100;  // Debounce time in ms

std::vector<uint32_t> soundEvents;
uint32_t observationStartTime = 0;

void resetSoundDetection() {
  observationStartTime = millis();
  soundEvents.clear();
  soundDetected = false;
}

bool wasSoundDetectedAt(uint32_t timestamp, uint32_t margin) {
  for (uint32_t t : soundEvents) {
    if (t > timestamp - margin && t < timestamp + margin) {
      return true;
    }
  }
  return false;
}

void IRAM_ATTR soundISR() {
  unsigned long currentTime = millis();
  if (currentTime - lastSoundTime > SOUND_DEBOUNCE) {
    soundEvents.push_back(currentTime - observationStartTime);
    soundDetected = true;
    lastSoundTime = currentTime;
  }
}

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

// Play buzzer tone
void playBuzzer(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
  delay(duration);
  noTone(BUZZER_PIN);
}

// Play game start melody
void playGameStartSound() {
  playBuzzer(C4_NOTE, 150);  // C note
  delay(50);
  playBuzzer(D4_NOTE, 150);  // D note
  delay(50);
  playBuzzer(E4_NOTE, 200);  // E note
}

void playConfirmSound() {
  tone(BUZZER_PIN, C5_NOTE, 100);
  tone(BUZZER_PIN, E5_NOTE, 100);  // C note
  tone(BUZZER_PIN, G5_NOTE, 100);  // G note
  //delay(100);
}

// Handle LM393 sound sensor input
bool handleSoundSensor() {
  if (soundDetected) {
    soundDetected = false;
    Serial.println("Sound detected!");
    //playConfirmSound();
    return true;
  }
  return false;
}

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

enum GameState {
  WAITING_TO_START,
  RUNNING,
  GAME_OVER,
  CODE_ENTRY,
  CLAP_GAME,
  END_GAME
};

GameState state = CODE_ENTRY;
int soundGameLevel = 0;

bool detectClap(int timeoutMs) {
  unsigned long startTime = millis();
  delay(timeoutMs);
  return handleSoundSensor();
}

void drawNotes(std::vector<int> notes, int startX, int totalTime) {
  int widthX = (256 / 2);
  int currentTime = 0;  
  for (const int note : notes) {
    if (note == 1) {
      display.drawCircle(startX + int(widthX * currentTime / totalTime), 55, 5, SSD1322_WHITE);
      currentTime += 1600;
    }
    if (note == 2) {
      display.fillCircle(startX + int(widthX * currentTime / totalTime), 50, 5, SSD1322_WHITE);
      currentTime += 800;
    }
    if (note == 3) {
      display.fillCircle(startX + int(widthX * currentTime / totalTime), 45, 5, SSD1322_WHITE);
      display.drawFastVLine(startX + 5 + int(widthX * currentTime / totalTime), 20, 25, SSD1322_WHITE);
      currentTime += 400;
    }
    if (note == 4) {
      display.fillCircle(startX + int(widthX * currentTime / totalTime), 40, 5, SSD1322_WHITE);
      display.drawFastVLine(startX + 5 + int(widthX * currentTime / totalTime), 15, 25, SSD1322_WHITE);
      display.drawLine(startX + 5 + int(widthX * currentTime / totalTime), 15, startX + 5 + int(widthX * currentTime / totalTime) + 4, 19, SSD1322_WHITE);
      currentTime += 200;
    }
  }
}

void drawTimeLine(int startX, int widthX, int currentTime, int totalTime) {
  display.drawFastVLine(startX + (widthX * currentTime / totalTime), 15, 40, SSD1322_WHITE);
}

void drawTimeEvents(int startX, int widthX, int totalTime) {
  uint32_t currentTime = millis() - observationStartTime;
  for (const int t : soundEvents) {
    int x = startX + (widthX * t / totalTime);
    display.drawTriangle(x-4, 60, x+4, 60, x, 55, SSD1322_WHITE);
  }
}

void updateDisplay(std::vector<int> notes, int offsetX, int currentTime, int totalTime) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 5);
  display.printf("Poziom %d: Powtorz dzwieki", soundGameLevel + 1);
  display.drawFastHLine(0, 15, 256, SSD1322_WHITE);
  display.drawFastHLine(0, 25, 256, SSD1322_WHITE);
  display.drawFastHLine(0, 35, 256, SSD1322_WHITE);
  display.drawFastHLine(0, 45, 256, SSD1322_WHITE);
  display.drawFastHLine(0, 55, 256, SSD1322_WHITE);
  display.drawFastVLine(0, 15, 40, SSD1322_WHITE);
  display.drawFastVLine(127, 15, 40, SSD1322_WHITE);
  display.drawFastVLine(255, 15, 40, SSD1322_WHITE);

  drawNotes(notes, 10, totalTime);
  drawNotes(notes, 137, totalTime);

  drawTimeLine(10, 246, currentTime, totalTime * 2);
  drawTimeEvents(10, 246, totalTime * 2);

  display.display();
}



bool playSoundGame(std::vector<int> notes, int tolerance) {
  int totalTime = 0;
  for (const int note : notes) {
    if (note == 1) totalTime += 1600;
    if (note == 2) totalTime += 800;
    if (note == 3) totalTime += 400;
    if (note == 4) totalTime += 200;
  }  
  
  updateDisplay(notes, 10, 0, totalTime);
  resetSoundDetection();

  int lastNoteTime = 0;
  int currentTime = 0;
  for (int note : notes) {
    if (note == 1) {
      tone(BUZZER_PIN, E4_NOTE, 400);
      lastNoteTime = 1600;
      int startTime = millis();
      for (int t = startTime; t < startTime + 1600; t = millis()) {
        delay(10);
        updateDisplay(notes, 10, currentTime + t - startTime, totalTime);
      }
      currentTime += millis() - startTime;
    }
    if (note == 2) {
      tone(BUZZER_PIN, F4_NOTE, 200);
      lastNoteTime = 800;
      int startTime = millis();
      for (int t = startTime; t < startTime + 800; t = millis()) {
        delay(10);
        updateDisplay(notes, 10, currentTime + t - startTime, totalTime);
      }
      currentTime += millis() - startTime;
    }
    if (note == 3) {
      tone(BUZZER_PIN, G4_NOTE, 100);
      lastNoteTime = 400;
      int startTime = millis();
      for (int t = startTime; t < startTime + 400; t = millis()) {
        delay(10);
        updateDisplay(notes, 10, currentTime + t - startTime, totalTime);
      }
      currentTime += millis() - startTime;

    }
    if (note == 4) {
      tone(BUZZER_PIN, A4_NOTE, 50);
      lastNoteTime = 200;
      int startTime = millis();
      for (int t = startTime; t < startTime + 200; t = millis()) {
        delay(10);
        updateDisplay(notes, 10, currentTime + t - startTime, totalTime);
      }
      currentTime += millis() - startTime;

    }
  }
  int startTime = millis();
  for (int t = startTime; t < startTime + totalTime; t = millis()) {
    delay(10);
    updateDisplay(notes, 10, currentTime + t - startTime, totalTime);
  }
  currentTime += millis() - startTime;

  // Check detected sounds
  // Remove sound events that are outside the valid time window
  soundEvents.erase(
    std::remove_if(soundEvents.begin(), soundEvents.end(), 
      [totalTime](uint32_t t) { return t < totalTime - 100; }),
    soundEvents.end()
  );

  if (notes.size() != soundEvents.size()) {
    Serial.println("Sound game failed: incorrect number of sounds detected");
    return false;
  }
  for (size_t i = 0; i < notes.size() - 1; i++) {
    int expectedTime = totalTime;
    if (!wasSoundDetectedAt(expectedTime, tolerance)) {
      Serial.println("Sound game failed at note " + String(i+1));
      return false;
    }
    if (notes[i] == 1) expectedTime += 1600;
    if (notes[i] == 2) expectedTime += 800;
    if (notes[i] == 3) expectedTime += 400;
    if (notes[i] == 4) expectedTime += 200;
  }

  return true;
}

void showSuccessScreen(String message) {
    tone(BUZZER_PIN, C5_NOTE, 500);
    tone(BUZZER_PIN, E5_NOTE, 500);
    tone(BUZZER_PIN, G5_NOTE, 500);
    tone(BUZZER_PIN, C6_NOTE, 500);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 20);
    display.println(message);
    display.display();
    delay(3000);
}

void soundGame() {
  if (soundGameLevel == 0) {        
    if (playSoundGame({3, 3}, 200)) {
      showSuccessScreen("Udalo sie!");
      soundGameLevel = 5;
    }
  } else if (soundGameLevel == 1) {
    if (playSoundGame({3, 3, 2}, 150)) {
      showSuccessScreen("Udalo sie!");
      soundGameLevel = 2;
    }    
  } else if (soundGameLevel == 2) {
    if (playSoundGame({3, 4, 4, 3}, 130)) {
      showSuccessScreen("Udalo sie!");
      soundGameLevel = 3;
    }    
  } else if (soundGameLevel == 3) {
    if (playSoundGame({4, 4, 4, 4, 4, 4, 4, 4}, 110)) {
      showSuccessScreen("Udalo sie!");
      soundGameLevel = 4;
    }    
  } else if (soundGameLevel == 4) {
    if (playSoundGame({1, 2, 3, 4}, 100)) {
      showSuccessScreen("Udalo sie!");
      soundGameLevel = 5;
    }    
  } else if (soundGameLevel == 5) {
    if (playSoundGame({3, 4, 4, 3, 2, 3, 2}, 100)) {
      showSuccessScreen("Udalo sie!");
      state = END_GAME;
    }    
  }
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


void loop() {
  if (state == WAITING_TO_START) {
    display.clearDisplay();
    display.setCursor(27, 20);
    display.clearDisplay();
    display.setTextSize(3);
    display.println("Nacisnij OK");
    display.display();  
  }
  if (state == WAITING_TO_START || state == GAME_OVER) {
    // Check for IR remote input
    int irKey = handleIRKeypad();
    if (irKey >= 0) {
      // Handle IR key press (example: restart game with OK button)
      if (irKey == 10) {
        if (dead) {  // OK button restarts game
          dead = false;
          positionX = 0;
          positionY = 0;
          directionX = true;
          directionY = true;
          speed = 30;
          loopNum = 0;
          gameStarted = true;
          Serial.println("Game restarted!");
        } else if (!gameStarted) {  // OK button starts game
          gameStarted = true;
          Serial.println("Game started!");
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