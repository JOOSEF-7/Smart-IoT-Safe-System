#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <Keypad.h>
#include <EEPROM.h>


#define IR_PIN 2         
#define SERVO_PIN 3      
#define GREEN_LED 11     
#define RED_LED 12       
#define BUZZER_PIN A1    


const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {10, 9, 8, 7}; 
byte colPins[COLS] = {6, 5, 4, A0}; 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

LiquidCrystal_I2C lcd(0x27, 16, 2); 
Servo myServo;


String currentInput = "";
String savedPassword = "";
String tempPassword = "";
String otpCode = ""; 
unsigned long lockoutStartTime = 0;
int wrongAttempts = 0;
bool isLockedOut = false;


const int PASS_FLAG_ADDR = 0; 
const int PASS_LEN_ADDR = 1;  
const int PASS_START_ADDR = 2; 
const int LOCK_STATE_ADDR = 100; 


enum SystemState { SAFE_IDLE, READY_TO_LOCK, SET_PASS_1, SET_PASS_2, LOCKED, RESET_ASK, OTP_WAIT, OTP_INPUT, OPENED, DURESS_CONFIRM };
SystemState currentState = SAFE_IDLE;



void playTone(int freq, int duration) {
  tone(BUZZER_PIN, freq); delay(duration); noTone(BUZZER_PIN);
}

void beepKey() { playTone(2500, 100); }

void beepError() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(RED_LED, HIGH); playTone(500, 200); digitalWrite(RED_LED, LOW); delay(100);
  }
}

void beepAlarm() {
  for (int i = 0; i < 5; i++) {
    tone(BUZZER_PIN, 3000); delay(200); tone(BUZZER_PIN, 2000); delay(200);
  }
  noTone(BUZZER_PIN);
}

void savePasswordToEEPROM(String pwd) {
  EEPROM.write(PASS_FLAG_ADDR, 1); 
  EEPROM.write(PASS_LEN_ADDR, pwd.length());
  for (int i = 0; i < pwd.length(); i++) EEPROM.write(i + PASS_START_ADDR, pwd[i]);
  savedPassword = pwd;
}

void loadPasswordFromEEPROM() {
  if (EEPROM.read(PASS_FLAG_ADDR) == 1) {
    int len = EEPROM.read(PASS_LEN_ADDR);
    savedPassword = "";
    for (int i = 0; i < len; i++) savedPassword += (char)EEPROM.read(i + PASS_START_ADDR);
  } else savedPassword = "";
}


void saveLockState(bool locked) {
  EEPROM.write(LOCK_STATE_ADDR, locked ? 1 : 0);
}

void clearInput() { currentInput = ""; }

void updateLCD(String l1, String l2) {
  lcd.clear(); lcd.setCursor(0, 0); lcd.print(l1); lcd.setCursor(0, 1); lcd.print(l2);
}

void setup() {
  Serial.begin(9600);
  
  pinMode(IR_PIN, INPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  lcd.init();
  lcd.backlight();
  updateLCD("System Loading...", "Check State...");
  delay(1000); 
  
  myServo.attach(SERVO_PIN);
  loadPasswordFromEEPROM();

  
  int lastState = EEPROM.read(LOCK_STATE_ADDR);
  
  if (lastState == 1 && savedPassword.length() > 0) {

    myServo.write(0); 
    currentState = LOCKED;
    updateLCD("Power Restored", "System LOCKED");
    delay(2000);
    updateLCD("Enter Password", "");
  } else {
 
    myServo.write(180); 
    currentState = SAFE_IDLE;
    saveLockState(false); 
    updateLCD("System Ready", "Waiting Object..");
    playTone(1000, 300);
  }
}

void loop() {

  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();


    if (command.indexOf("OTP:") >= 0) {
      otpCode = command.substring(4);
      beepKey(); 
      updateLCD("Enter OTP Code:", "");
      currentState = OTP_INPUT; 
      wrongAttempts = 0;
      clearInput();
    } 
    
    else if (command.startsWith("CMD_OPEN:")) {
      String webPass = command.substring(9); 

      if (isLockedOut) {
         Serial.println("ERR_LOCKED_OUT"); 
         updateLCD("System Locked!", "Wait Timer...");
         beepError();
         delay(2000);
       
         if(currentState == LOCKED) updateLCD("Enter Password", "");
      }
      else if (currentState == LOCKED) {
         if (webPass == savedPassword) {
            myServo.write(180); 
            currentState = SAFE_IDLE; 
            saveLockState(false); 
            updateLCD("Opened via Web", "System Ready"); 
            playTone(2000, 300);
            delay(2000); 
            updateLCD("System Ready", "Waiting Object..");
         } else {
            Serial.println("ERR_WRONG_PASS");
            updateLCD("Web: Wrong Pass", "");
            beepError();
            delay(2000);
            updateLCD("Enter Password", "");
         }
      }
    }

    else if (command.indexOf("CMD_LOCK") >= 0) {
       
       if (digitalRead(IR_PIN) == HIGH) { 
          Serial.println("ERR_EMPTY");
          updateLCD("Error: Empty!", "Put Object First");
          beepError();
          delay(2000);
          updateLCD("System Ready", "Waiting Object..");
       }
       else if (savedPassword.length() > 0) {
          myServo.write(0); 
          currentState = LOCKED;
          saveLockState(true);
          updateLCD("Locked via Web", ""); delay(1000); updateLCD("Enter Password", "");
          Serial.println("LOCK_SUCCESS"); 
       } else {
          Serial.println("ERR_NO_PASS"); 
          updateLCD("Error: No Pass", "Set Pass First");
          delay(2000);
          updateLCD("System Ready", "Waiting Object..");
       }
    }
  }


  if (isLockedOut) {
    unsigned long remaining = 120000 - (millis() - lockoutStartTime);
    if (remaining > 0) {
       if (millis() % 1000 < 50) { 
          lcd.setCursor(0, 0); lcd.print("SYSTEM LOCKED!  ");
          lcd.setCursor(0, 1); lcd.print("Wait: "); lcd.print(remaining / 1000); lcd.print(" sec   ");
       }
       return; 
    } else {
       isLockedOut = false; 
       wrongAttempts = 0; 
       currentState = LOCKED;
       updateLCD("Enter Password", "");
    }
  }

  char key = keypad.getKey();
  if (key) beepKey();


  if (key == 'D') {
      currentState = DURESS_CONFIRM;
      updateLCD("SOS Mode?", "*:Confirm #:Cnl");
      return;
  }


  switch (currentState) {
    
    case DURESS_CONFIRM:
      if (key == '*') {
         updateLCD("SENDING SOS...", "");
         Serial.println("ALERT_DURESS"); 
         playTone(3000, 500);
         delay(1000);
     
         currentState = LOCKED; 
         updateLCD("Enter Password", "");
      } else if (key == '#') {
         currentState = LOCKED;
         updateLCD("Enter Password", "");
      }
      break;

    case SAFE_IDLE: 
      if (digitalRead(IR_PIN) == LOW) { 
        delay(100); 
        if (digitalRead(IR_PIN) == LOW) {
           if (savedPassword.length() > 0) {
              currentState = READY_TO_LOCK; 
              updateLCD("Object Detected", "Press A to Lock");
              digitalWrite(GREEN_LED, HIGH);
           } else {
              currentState = SET_PASS_1; 
              updateLCD("Set Password:", ""); clearInput();
           }
        }
      }
      break;

    case READY_TO_LOCK:
       if (digitalRead(IR_PIN) == HIGH) {
          digitalWrite(GREEN_LED, LOW);
          currentState = SAFE_IDLE;
          updateLCD("System Ready", "Waiting Object..");
          return;
       }
      if (key == 'A') {
        digitalWrite(GREEN_LED, LOW);
        myServo.write(0); 
        currentState = LOCKED; 
        saveLockState(true); 
        updateLCD("Door Locked", ""); delay(1000); updateLCD("Enter Password", ""); clearInput();
      }
      else if (key == 'C') { 
         digitalWrite(GREEN_LED, LOW);
         currentState = SET_PASS_1; savedPassword = ""; 
         EEPROM.write(PASS_FLAG_ADDR, 0);
         updateLCD("Pass Cleared", "Set New One"); delay(1500); updateLCD("Set Password", ""); clearInput();
      }
      break;

    case SET_PASS_1: 
      if (key == '#') { clearInput(); updateLCD("Set Password:", ""); }
      else if (key == '*') {
        if (currentInput.length() < 4) {
          updateLCD("Min 4 Digits!", ""); delay(2000); updateLCD("Set Password:", ""); clearInput();
        } else {
          tempPassword = currentInput; currentState = SET_PASS_2; updateLCD("Confirm Pass:", ""); clearInput();
        }
      } 
      else if (key && key != 'A' && key != 'B' && key != 'C') { 
        currentInput += key; lcd.setCursor(0, 1); lcd.print(currentInput); 
      }
      break;

    case SET_PASS_2: 
      if (key == '#') { currentState = SET_PASS_1; updateLCD("Set Password:", ""); clearInput(); }
      else if (key == '*') {
        if (currentInput == tempPassword) {
          savePasswordToEEPROM(currentInput); currentState = READY_TO_LOCK; updateLCD("Password Set!", "Press A to Lock");
        } else {
          updateLCD("Mismatch!", ""); beepError(); delay(1500); currentState = SET_PASS_1; updateLCD("Set Password:", ""); clearInput();
        }
      } 
      else if (key && key != 'A' && key != 'B' && key != 'C') { 
        currentInput += key; lcd.setCursor(0, 1); lcd.print(currentInput); 
      }
      break;

    case LOCKED:
      if (key == 'C') { currentState = RESET_ASK; updateLCD("Forgot Pass?", "*:Yes #:No"); }
      else if (key == '#') { clearInput(); updateLCD("Enter Password", ""); }
      else if (key == '*') {
        if (currentInput == savedPassword) {
           updateLCD("Correct!", "Press B to Open");
           wrongAttempts = 0;
           currentState = OPENED; 
        } else {
           updateLCD("Wrong Password!", ""); beepError(); wrongAttempts++;
           if (wrongAttempts >= 3) { 
             isLockedOut = true; 
             lockoutStartTime = millis(); 
             Serial.println("ALERT_INTRUDER"); 
             beepAlarm(); 
             updateLCD("System Locked!", "Wait 2 Mins"); 
           }
           else { delay(1000); updateLCD("Enter Password", ""); clearInput(); }
        }
      }
      else if (key && key != 'A' && key != 'B' && key != 'C') { 
        currentInput += key; lcd.setCursor(0, 1); String m=""; for(int i=0;i<currentInput.length();i++) m+="*"; lcd.print(m); 
      }
      break;
      
    case OPENED:
       if (key == 'B') {
         myServo.write(180); 
         currentState = SAFE_IDLE; 
         saveLockState(false);
         updateLCD("Door Opened", "System Ready");
         playTone(2000, 300);
         delay(2000);
         updateLCD("System Ready", "Waiting Object..");
       }
       break;

    case RESET_ASK:
      if (key == '#') { currentState = LOCKED; updateLCD("Enter Password", ""); }
      else if (key == '*') { 
        updateLCD("Sending OTP...", "To Telegram"); 
        Serial.println("REQ_OTP"); 
        currentState = OTP_WAIT; 
      }
      break;

    case OTP_WAIT: break; 

    case OTP_INPUT:
       if (key == '#') { currentState = LOCKED; updateLCD("Enter Password", ""); clearInput(); }
       else if (key == '*') {
         if (currentInput == otpCode) {
           updateLCD("OTP Correct!", ""); playTone(1000, 200); delay(1000);
           wrongAttempts = 0; 
           currentState = SET_PASS_1; updateLCD("Set New Pass:", ""); clearInput();
         } else {
           updateLCD("Wrong OTP!", ""); beepError(); 
           wrongAttempts++;
           if (wrongAttempts >= 3) {
             isLockedOut = true; lockoutStartTime = millis(); 
             Serial.println("ALERT_INTRUDER");
             beepAlarm(); updateLCD("Too Many Tries!", "Wait 2 Mins");
             currentState = LOCKED; 
           } else {
             delay(1000); updateLCD("Enter OTP Code:", "Tries Left: " + String(3 - wrongAttempts)); clearInput();
           }
         }
       } else if (key && isDigit(key)) { currentInput += key; lcd.setCursor(0, 1); lcd.print(currentInput); }
       break;
  }
}