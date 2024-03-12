#include <Arduino.h>
#include "SoftwareSerial.h"                                    
#include <DFPlayerMini_Fast.h>
#include <ESP32Servo.h>
#include <ArduinoHA.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Fonts/FreeMono9pt7b.h>
#include "definitions.h"
#include "icons.h"
#include "idle.h"

#define debug 1

SoftwareSerial mySoftwareSerial(DF_RX, DF_TX); // RX, TX
DFPlayerMini_Fast myDFPlayer;
Servo myservo;
WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
TaskHandle_t Task1;
Adafruit_PN532 nfc(SPI_SCLK, SPI_MISO, SPI_MOSI, RFID_CS);

HALock lock("Gatelock");
HASensorNumber sonar("sonar");
HABinarySensor reed("reed");
HASwitch buzzer("buzzer");

#include "rfid_functions.h"
#include "HA.h"
#include "mqtt.h"

byte Read_Keypad(void)
{
  byte Count;
  byte Key_State = 0;
  for(Count = 1; Count <= 16; Count++)
  {
    digitalWrite(KEYPAD_SCL, LOW); 
    if (!digitalRead(KEYPAD_SDA))
      Key_State = Count; 
    
    digitalWrite(KEYPAD_SCL, HIGH);
  }  
  
  return Key_State; 
}

void setup() 
{ 
  Serial.begin(115200);

  byte mac[MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);
  device.setUniqueId(mac, sizeof(mac));

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(200);
  }

  setup_mqtt();

  delay(500);

  nfc.begin();
  nfc.setPassiveActivationRetries(0xFF);

  pinMode(TRIG_SONAR, OUTPUT);
  pinMode(ECHO_SONAR, INPUT);

  pinMode(REED_PIN, INPUT);

  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(KEYPAD_SCL, OUTPUT);  
  pinMode(KEYPAD_SDA, INPUT);

  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500);
  
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	ESP32PWM::allocateTimer(2);
	ESP32PWM::allocateTimer(3);
	myservo.setPeriodHertz(50);
  myservo.attach(SERVO_PIN, 100, 3500);
  myservo.write(0);

  Wire.begin();

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setFont(&FreeMono9pt7b);
  delay(2000);
  display.clearDisplay(); 

  mySoftwareSerial.begin(9600);
  myDFPlayer.begin(mySoftwareSerial, true);
  myDFPlayer.volume(20);

  delay(500);
  myDFPlayer.volume(20);
  delay(500);
}

void sensor_functions() {
  if(millis() - sonar_millis > 2000)
  {
    long duration, cm;
    digitalWrite(TRIG_SONAR, LOW);
    delayMicroseconds(5);
    digitalWrite(TRIG_SONAR, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_SONAR, LOW);
  
    pinMode(ECHO_SONAR, INPUT);
    duration = pulseIn(ECHO_SONAR, HIGH);

    cm = (duration/2) / 29.1;

    if(cm < 20)
    {
      movement = true;
    }
    else
    {
      movement = false;
    }

    gate_is_open = digitalRead(REED_PIN);

    sonar.setValue(static_cast<int32_t>(cm));
    reed.setState(!gate_is_open);

    sonar_millis = millis();
  }
}

void lock_functions() {

  if(unlock_now)
  {
    myservo.attach(SERVO_PIN, 100, 3500);
    myservo.write(0);
    myDFPlayer.playAdvertisement(3);
    gate_locked = false;
  }
  else if(lock_now)
  {
    myservo.attach(SERVO_PIN, 100, 3500);
    myservo.write(180);
    myDFPlayer.playAdvertisement(3);
    gate_locked = true;
  }

  unlock_now = false;
  lock_now = false;
}

void clearData() {

  while(data_count !=0){
    Data[data_count--] = 0; 
  }
  return;
}

void terminal() {

  if(gate_is_open && !gate_locked && !auto_lock_tmr_started)// if gate closed and gate unlocked
  {
    auto_lock_time = millis();
    auto_lock_tmr_started = true;
    Serial.println("Autolock timer started");
  }

  if(auto_lock_tmr_started && millis() - auto_lock_time > 4000)//automatically locks the gate
  {
    myDFPlayer.playAdvertisement(3);
    gate_locked = true;
    auto_lock_tmr_started = false;
    Serial.println("Auto Locking");
    myservo.attach(SERVO_PIN, 100, 3500);
    myservo.write(180);
    lock_now = true;
  }

  if(digitalRead(REED_PIN) == false && gate_locked && millis() - unlocked_at_time > 5000)
  {
    myDFPlayer.playAdvertisement(3);
    gate_locked = false;
    Serial.println("Auto Unlocking");
    myservo.attach(SERVO_PIN, 100, 3500);
    myservo.write(0);
    unlock_now = true;
  }

  if(terminal_active == true && !movement && !terminal_shutdown_tmr_started)// if no movement
  {
    terminal_shutdown_time = millis();
    terminal_shutdown_tmr_started = true;
    Serial.println("Terminal Shutdown timer started");
  }

  if(terminal_shutdown_tmr_started && millis() - terminal_shutdown_time > 10000)// Automatically turns off terminal
  {
    terminal_active = false;
    terminal_shutdown_tmr_started = false;
    clearData(); 
    Serial.println("Shutting Down Terminal");
  }

  if(terminal_active == true) // if someone activates the terminal
  {
    if(gate_locked == true) // if the door is locked
    {
      Key = Read_Keypad();

      if(Key)
      {
        if(Key != 10 && Key != 11 && Key != 12 && Key != 13 && Key != 14 && Key != 15 && Key != 16)
        {        
          myDFPlayer.playAdvertisement(10);
          Data[data_count] = char(Key+48);
          int x = (data_count + 3) * 11;
          display.setCursor(x,44);
          display.print(Data[data_count]);
          display.display();
          data_count++;
          terminal_shutdown_time = millis();
          delay(300);
        }
      }
  
      if(data_count == Password_Length-1)
      {
        delay(500);

        if(!strcmp(Data, Master)) // If password is correct
        {
          myservo.attach(SERVO_PIN, 100, 3500);
          myservo.write(0);
          gate_locked = false;
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(1);
          display.setFont(&FreeMono9pt7b);
          display.drawRoundRect(0, 0, 128, 64, 3, 1);
          display.drawRoundRect(2, 2, 124, 60, 2, 1);
          display.setCursor(32, 26);
          display.print("ACCESS");
          display.setCursor(25, 45);
          display.print("GRANTED");
          display.display();
          myDFPlayer.playAdvertisement(3);
          delay(1000);
          myDFPlayer.playAdvertisement(11);
          delay(1000);
          display.clearDisplay();
          display.drawBitmap(0, 0, unlocked, SCREEN_WIDTH, SCREEN_HEIGHT, 1);
          display.display();
          unlocked_at_time = millis();
        }
        else // if password is not correct
        {
          display.clearDisplay();
          display.setTextColor(WHITE);
          display.setTextSize(1);
          display.setFont(&FreeMono9pt7b);
          display.drawRoundRect(0, 0, 128, 64, 3, 1);
          display.drawRoundRect(2, 2, 124, 60, 2, 1);
          display.setCursor(32, 26);
          display.print("ACCESS");
          display.setCursor(32, 45);
          display.print("DENIED");
          display.display();
          myDFPlayer.playAdvertisement(2);
          delay(1000);
          myDFPlayer.playAdvertisement(12);
          delay(3000);
          terminal_shutdown_time = millis();
          display.clearDisplay();
          display.drawRoundRect(0, 0, 128, 64, 3, 1);
          display.drawRoundRect(2, 2, 124, 60, 2, 1);
          display.drawRoundRect(4, 28, 120, 23, 20, 1);
          display.setCursor(11, 20);
          display.print("Enter Pin:");
          display.display();
        } 
        clearData();  
      }
    }
    else if(gate_locked == false) // if door is unlocked and someone activates the terminal
    {
      display.clearDisplay();
      display.drawBitmap(0, 0, unlocked, SCREEN_WIDTH, SCREEN_HEIGHT, 1);
      display.display();
    }
  }
  else // if terminal is off
  {
    if (!movement) 
    {
      if(terminal_active == false)
      {
        if(millis() - last_frame_time > frame_delay)
        {
          display.clearDisplay();
          display.drawBitmap(0, 0, codeNumbers[frame_counter], SCREEN_WIDTH, SCREEN_HEIGHT, 1);
          display.display();
          if(frame_counter != frames)
          {
            frame_counter++;
          }
          else
          {
            frame_counter = 0;
          }
          last_frame_time = millis();
        }
      }
    }
    else // if terminal activated
    {
      if(gate_locked)
      {
        delay(300);
        terminal_active = true;
        display.clearDisplay();
        myDFPlayer.playAdvertisement(5);
        display.drawRoundRect(0, 0, 128, 64, 3, 1);
        display.drawRoundRect(2, 2, 124, 60, 2, 1);
        display.drawRoundRect(4, 28, 120, 23, 20, 1);
        display.setCursor(11, 20);
        display.print("Enter Pin:");
        display.display();
      }
      else
      {
        delay(300);
        terminal_active = true;
        display.clearDisplay();
        display.drawBitmap(0, 0, unlocked, SCREEN_WIDTH, SCREEN_HEIGHT, 1);
        display.display();
        myDFPlayer.playAdvertisement(5);
      }
    }
  }
}

void Task1code( void * pvParameters ){
  for(;;) {
    mqtt.loop();
    sensor_functions();
    lock_functions();
    terminal();
    delay(1);
  }
}

void loop() 
{
  readNFC();
}