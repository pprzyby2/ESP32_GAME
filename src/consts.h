#ifndef CONSTS_H
#define CONSTS_H

#include "Arduino.h"

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

enum GameState {
  WAITING_TO_START,
  STARTING_MESSAGE,
  RUNNING,
  GAME_OVER,
  CODE_ENTRY,
  CLAP_GAME,
  END_GAME
};

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

#endif // CONSTS_H