#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <PS2Keyboard.h>
#include <SoftwareSerial.h>
#include "Vars.h"

#define I2C_ADDR 0x3F // I2C address for the green 16x2 LCD screen (0x27 is the address of the blue i2c screen)
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

/* LCD screen declarations */
LiquidCrystal_I2C  lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);
unsigned short index = 0;
const unsigned short LCD_LENGTH = 16;
const unsigned short LCD_HEIGHT = 2;

/* keyboard declarations */
const int DataPin = 4;
const int IRQpin =  3;
PS2Keyboard keyboard;

/* email input declarations */
String userEmail = "";
char verificationInput = 'x'; //initialize it with 'x' value, so we know when it is not set yet

State state = EMAIL_INPUT;
const unsigned short ARROW_STEP = 3; //how many characters will the displayed word be shifted upon button press
unsigned short rightIndex = 0;
unsigned short leftIndex = 0;
const unsigned short MAX_EMAIL_LENGTH = 50;

/* ESP2688 wifi module declarations */
#define SSID "Honeypott" //your SSID
#define PASS "krukuschFR0NG69382!?" //your wifi password
#define IP "184.106.153.149" // thingspeak.com IP address
String GET = "GET /update?key=8NQMI3VUN6SZY1FJ&field1="; //get request url
SoftwareSerial ESP8266(10, 11); // RX, TX
const unsigned short DEBUG = ON; //debug information on Serial
boolean bootError = false;
const unsigned short MAX_VALIDATION_EFFORTS = 5; //max ammount of efforts to try and reconnect to the network if you have failed in setup()

void setup() {
  keyboard.begin(DataPin, IRQpin);
  lcd.begin (LCD_LENGTH, LCD_HEIGHT); //16x2
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.home ();
  lcd.print("Locating ESP8266");
  if (DEBUG) Serial.begin(9600);
  ESP8266.begin(9600);
  sendCommand("AT");
  loadAnimation(5000); //show something pretty while we wait for the wifi module to boot
  if (ESP8266.find("OK")) {
    if (DEBUG) Serial.println("RECEIVED: OK");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Configuring WiFi");
    bootError = !connectWiFi();
    if (bootError) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Config failed");
      lcd.setCursor(0, 1);
      lcd.print("Check module");
    } else {
      clearLine(1);
      lcd.print("Success");
    }
  } else {
    bootError = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ESP8266 missing");
    lcd.setCursor(0, 1);
    lcd.print("Check connection");
  }
  delay(1000);
  if (!bootError) { //if there is no error so far, one final check
    unsigned short efforts = 0;
    do {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Validating WiFi");
      bootError = !validIP();
      efforts++;
      if (bootError) {  //if you failed getting an IP, try to connect again before checking again for an IP
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Configuring WiFi");
        connectWiFi();
      }
    } while (bootError && efforts < MAX_VALIDATION_EFFORTS); //keep trying until you have reached the MAX_EFFORTS or there is no bootError
    if (!bootError) { //if there is still no error, you are good to go
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Enter your email");
    } else { //this means we have not got a valid IP address
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Bad connection");
      lcd.setCursor(0, 1);
      lcd.print("Check WiFi");
    }
  }
}

void loop() {
  if (bootError) return;
  if (keyboard.available()) {
    lcd.setCursor(0, 1);
    char c = keyboard.read();
    if (c == PS2_ENTER) { //when ENTER is pressed
      switch (state) {
        case EMAIL_INPUT:
          {
            unsigned short wordLength = userEmail.length();
            if (wordLength > 0) { //if the user actually provided soemthing
              state = EMAIL_VERIFICATION; //go to the EMAIL_VERIFICATION state
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Submit (y/n):");
              lcd.setCursor(0, 1);
              leftIndex = 0;
              if (wordLength > LCD_LENGTH) { //if the email is longer than the LCD_LENGTH, then set the right index to the LCD_LENGTH and pring the word from the beginning
                rightIndex = LCD_LENGTH;
              } else {
                rightIndex = wordLength;
              }
              lcd.print(userEmail.substring(leftIndex, rightIndex)); //print what is between the indices
            }
          }
          break;
        case EMAIL_VERIFICATION:
          if (verificationInput == 'y') {
            verificationInput = 'x'; //reseting the value
            if (emailValid(userEmail)) { //validate user's email
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Submitting email");
              boolean emailSubmitted = submitEmail(userEmail); //submit email
              if (emailSubmitted) { //if email was submitted successfully, go back to the EMAIL_INPUT state and reinitialize the fields to accept new emails
                state = EMAIL_INPUT;
                rightIndex = 0;
                leftIndex = 0;
                userEmail = "";
                lcd.setCursor(0, 0);
                lcd.print("Enter your email");
                clearLine(1); //clear the email line so a new one can be submitted
              } else { //if something went wrong, remain in the EMAIL_VERIFICATION state
                unsigned short wordLength = userEmail.length();
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Submit (y/n):");
                lcd.setCursor(0, 1);
                leftIndex = 0;
                if (wordLength > LCD_LENGTH) { //if the email is longer than the LCD_LENGTH, then set the right index to the LCD_LENGTH and pring the word from the beginning
                  rightIndex = LCD_LENGTH;
                } else {
                  rightIndex = wordLength; //if it's within the LCD screen's dimensions, keep it at the word's length
                }
                lcd.print(userEmail.substring(leftIndex, rightIndex)); //print what is between the indices
              }
            } else { //if email is not valid
              clearLine(0); //clear what is on the first line
              lcd.print("Invalid email");
              delay(1500);
              clearLine(0);
              lcd.print("Submit (y/n):");
            }
          } else if (verificationInput == 'n') {
            state = EMAIL_INPUT;
            verificationInput = 'x'; //reseting the value
            lcd.setCursor(0, 0);
            lcd.print("Enter your email");
            lcd.setCursor(0, 1);
            unsigned short wordLength = userEmail.length();
            rightIndex = wordLength;
            if (wordLength > LCD_LENGTH) {
              leftIndex = wordLength - LCD_LENGTH; //print the word from the end and as much as the screen's dimensions permit it
            } else { //if the email is too long for the screen, just display it from the beginning
              leftIndex = 0;
            }
            lcd.print(userEmail.substring(leftIndex, rightIndex));
          }
          break;
      }
    } else if (c == PS2_TAB) {} //when TAB is pressed
    else if (c == PS2_ESC) {} //when ESCape is pressed
    else if (c == PS2_PAGEDOWN) {} //when PgDn is pressed
    else if (c == PS2_PAGEUP) {} //when PgUp is pressed
    else if (c == PS2_LEFTARROW) { //when Left Arrow is pressed
      switch (state) {
        case EMAIL_VERIFICATION:
          {
            unsigned short wordLength = userEmail.length();
            if (wordLength > LCD_LENGTH) {
              lcd.setCursor(0, 1); //set the cursor where it should be
              if (leftIndex <= ARROW_STEP) { //you can't go more "left"
                leftIndex = 0;
                rightIndex = LCD_LENGTH;
              } else { //if there are stuff on the left
                leftIndex -= ARROW_STEP;
                rightIndex -= ARROW_STEP;
              }
              lcd.print(userEmail.substring(leftIndex, rightIndex));
            }
          }
          break;
      }
    }
    else if (c == PS2_RIGHTARROW) { //when Right Arrow is pressed
      switch (state) {
        case EMAIL_VERIFICATION:
          {
            unsigned short wordLength = userEmail.length();
            if (wordLength > LCD_LENGTH) {
              lcd.setCursor(0, 1);
              if (rightIndex + ARROW_STEP > wordLength) { //don't "step" out of the string
                rightIndex = wordLength;
                leftIndex = wordLength - LCD_LENGTH;
              } else {
                rightIndex += ARROW_STEP;
                leftIndex += ARROW_STEP;
              }
              lcd.print(userEmail.substring(leftIndex, rightIndex));
            }
          }
          break;
      }
    }
    else if (c == PS2_UPARROW) {} //when Up Arrow is pressed
    else if (c == PS2_DOWNARROW) {} //when Down Arrow is pressed
    else if (c == PS2_DELETE) { //when DELETE or BACKSPACE is pressed
      switch (state) {
        case EMAIL_INPUT:
          {
            unsigned short wordLength = userEmail.length();
            if (wordLength) {
              userEmail.remove(wordLength - 1); //if wordLength is not 0, remove the last character
              wordLength--;
              if (wordLength < LCD_LENGTH) clearLine(1); //we have to get rid of the excessive characters now that we are writing too few
              printEmail(wordLength);
            }
          }
          break;
      }
    } else { //if it's a common character
      switch (state) {
        case EMAIL_INPUT:
          {
            unsigned short wordLength = userEmail.length();
            if (wordLength < MAX_EMAIL_LENGTH) { //don't accept words larger than MAX_EMAIL_LENGTH characters, too big to be email
              userEmail += c; //concat the new character
              wordLength++; //our userEmail just became one digit longer
              printEmail(wordLength);
            }
          }
          break;
        case EMAIL_VERIFICATION:
          if (c == 'y' || c == 'n') { //ignore any other button presses in this state
            verificationInput = c;
            lcd.setCursor(14, 0);
            lcd.print(verificationInput); //print out user's choice on the screen
          }
          break;
      } //end if
    }
  }
}

void printEmail(const unsigned short wordLength) {
  lcd.setCursor(0, 1);
  if (wordLength <= LCD_LENGTH) {//if the word is shorter than the LCD length, just print it out
    lcd.print(userEmail);
  } else { //if the word is longer than LCD_LENGTH, then display what can fit, starting from the end
    lcd.print(userEmail.substring(wordLength - LCD_LENGTH));
  }
}

void clearLine(const unsigned short line) {
  lcd.setCursor(0, line);
  lcd.print("                "); //print a blank line
  lcd.setCursor(0, line);
}

void loadAnimation(const unsigned short millisToWait) {
  clearLine(1);
  unsigned short millisDelay = millisToWait / LCD_LENGTH;
  for (int i = 0; i < LCD_LENGTH; i++) {
    lcd.print(".");
    delay(millisDelay);
  }
  clearLine(1);
}

boolean emailValid(const String userEmail) {
  unsigned short wordLength = userEmail.length();
  if (wordLength < 7) return false; //can't have something smaller than a@bc.de
  short atIndex = userEmail.lastIndexOf('@');
  if (atIndex <= 0) return false; //if we don't have an '@' or it only exists as the first character
  short dotIndex = userEmail.lastIndexOf('.');
  if ((dotIndex <= 3) || (dotIndex > wordLength - 3) ) return false; //if the last occurence of '.' is not too early or does not exist or is in the very end
  if (atIndex > dotIndex) return false; //if the last '@' is after the last '.'
  return true;
}

boolean submitEmail(const String userEmail) {
  if (DEBUG) Serial.println(userEmail);
  return postString(userEmail);
}

