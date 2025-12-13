#ifndef SOUND_GAME_HPP
#define SOUND_GAME_HPP

#include <vector>
#include <SSD1322_for_Adafruit_GFX.h>
#include "Arduino.h"
#include "consts.h"

extern Adafruit_SSD1322 display;
extern GameState state;

// LM393 sensor variables
const unsigned long SOUND_DEBOUNCE = 100;  // Debounce time in ms
int soundGameLevel = 0;
volatile bool soundDetected = false;
volatile unsigned long lastSoundTime = 0;
std::vector<uint32_t> soundEvents;
uint32_t observationStartTime = 0;

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

void showSuccessScreen(String message, int showTimeMs = 10000) {
    tone(BUZZER_PIN, C5_NOTE, 500);
    tone(BUZZER_PIN, E5_NOTE, 500);
    tone(BUZZER_PIN, G5_NOTE, 500);
    tone(BUZZER_PIN, C6_NOTE, 500);
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 20);
    display.println(message);
    display.display();
    delay(showTimeMs);
}

void soundGame() {
  if (soundGameLevel == 0) {        
    if (playSoundGame({3, 3}, 200)) {
      showSuccessScreen("Udalo sie! Poziom 2", 3000);
      soundGameLevel = 1;
    }
  } else if (soundGameLevel == 1) {
    if (playSoundGame({3, 3, 2}, 150)) {
      showSuccessScreen("Udalo sie! Poziom 3", 3000);
      soundGameLevel = 2;
    }    
  } else if (soundGameLevel == 2) {
    if (playSoundGame({3, 4, 4, 3}, 130)) {
      showSuccessScreen("Udalo sie! Poziom 4", 3000);
      soundGameLevel = 3;
    }    
  } else if (soundGameLevel == 3) {
    if (playSoundGame({4, 4, 4, 4, 4, 4, 4, 4}, 110)) {
      showSuccessScreen("Udalo sie! Poziom 5", 3000);
      soundGameLevel = 4;
    }    
  } else if (soundGameLevel == 4) {
    if (playSoundGame({1, 2, 3, 4}, 100)) {
      showSuccessScreen("Udalo sie! Poziom 6", 3000);
      soundGameLevel = 5;
    }    
  } else if (soundGameLevel == 5) {
    if (playSoundGame({3, 4, 4, 3, 2, 3, 2}, 100)) {
      showSuccessScreen("Udalo sie! Kod do walizki: 042", 20000);
      state = END_GAME;
    }    
  }
}

#endif // SOUND_GAME_HPP