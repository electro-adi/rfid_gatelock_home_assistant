#ifndef _DEFINITIONS_H
#define _DEFINITIONS_H

const char* ssid = "ssid";
const char* password = "password"; 

#define SPI_MOSI 23
#define SPI_MISO 19
#define SPI_SCLK 18

#define DF_RX    26
#define DF_TX    14

#define KEYPAD_SDA 27
#define KEYPAD_SCL 32

#define TRIG_SONAR 33
#define ECHO_SONAR 34

#define REED_PIN 35
#define SERVO_PIN 17

#define RFID_CS   5

#define BUZZER_PIN 15

uint8_t card1[4] = {99, 10, 72, 167};
uint8_t card2[4] = {3, 207, 174, 254};
uint8_t card3[4] = {163, 49, 148, 3};
uint8_t tag1[4] = {176, 17, 180, 19};
uint8_t tag2[4] = {176, 79, 59, 88};


#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

#define MAC_ADDR_LENGTH 6

int frame_delay = 50;  
int frame_counter = 0;
unsigned long last_frame_time;

byte Key;

#define Password_Length 7

int data_count = 0;

char Master[Password_Length] = "388578";
char Data[Password_Length]; 

unsigned long sonar_millis = 0;

unsigned long auto_lock_time = 0;
bool auto_lock_tmr_started = false;

unsigned long terminal_shutdown_time = 0;
bool terminal_shutdown_tmr_started = false;

unsigned long unlocked_at_time = 0;

bool terminal_active = false;
bool unlock_now = false;
bool lock_now = false;
bool gate_locked = false;
bool gate_is_open = false;
bool movement = false;

#endif