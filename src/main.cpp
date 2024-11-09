#include <Arduino.h>
#include <FastLED.h>
#include <DFRobotDFPlayerMini.h>
#include <Servo.h>

// state
#define INITIALIZING 0
#define STANDBY 1 // waiting for people to come
#define WORKING 2 // pushing out a candy
#define WAITING 3 // waiting for people to go

int state; // the dispenser state
int count = 0; // to detect for people to go and come

// pin setting
const int PIN_US_TRG = 7; // Ultrasonic sensor trig
const int PIN_US_ECH = 9; // Ultrasonic sensor echo
const int PIN_LED_RGB = 13; // RGB LEDs
const int PIN_LED_RED1 = 1; // Red LED
const int PIN_LED_RED2 = 15; // Red LED
const int PIN_SM = 5; // Servo motor

// setting for the ultrasonic sensor
float temperature = 23; // temperature
float speed_of_sound = 331.5 + 0.6 * temperature;  // sound speed [m/sec]
float us_timeout = 100 * 10000 / speed_of_sound * 2; // timeout usec @ 100cm
float duration = 0;
float distance = 0;
float approaching_distance = 30; // threshold distance for approaching [cm]
float going_away_distance = 50; // // threshold distance for going away[cm]

// setting for the RGB LEDs
const int NUM_LEDS = 10;            // for M5StampS3 defalut LED WS2812
CRGB leds[NUM_LEDS];
const int brightness_max = 10;
int brightness_indicator = brightness_max; // from 0 to brightness_max
bool brightness_up = false;

// setting for the servo motor
const int CH_SM = 1;
const int WR_MINUS = 500, WR_MAXUS = 2500; // [us], Pulse width range of DS-007M and MG90S
const int SM_RANGE[] = {0, 65}; //0-65 deg 
Servo servo = Servo();

// setting for the speaker
DFRobotDFPlayerMini myDFPlayer;
HardwareSerial mySerial(0);

// function prototype
boolean update_sensor(float);

void setup() {
  state = INITIALIZING;
  //USBSerial.begin(115200);
  mySerial.begin(9600);
  delay(2000);

  servo.attach(PIN_SM, CH_SM, WR_MINUS, WR_MAXUS);
  servo.write(PIN_SM, SM_RANGE[0]);

  pinMode(PIN_US_TRG, OUTPUT);
  pinMode(PIN_US_ECH, INPUT);

  pinMode(PIN_LED_RED1, OUTPUT);
  pinMode(PIN_LED_RED2, OUTPUT);
  digitalWrite(PIN_LED_RED1, HIGH);
  digitalWrite(PIN_LED_RED2, HIGH);

  FastLED.setBrightness(255); // 0-255
  FastLED.addLeds<WS2812, PIN_LED_RGB, GRB>(leds, NUM_LEDS);
  for (int i = 0; i < NUM_LEDS; i++) {
    //leds[i] = CRGB::Blue;
    leds[i] = CHSV(120+i*5, 255, 255);
  }
  //speaker setting
  if (!myDFPlayer.begin(mySerial)) {
    //USBSerial.print("Failed to initialize DFPlayer");
    leds[0] = CRGB::Red;
    FastLED.show();
    while(1){
      delay(2000);
    }
  }else{
    myDFPlayer.volume(30); //Set volume value. From 0 to 30
    //leds[0] = CRGB::Blue;
    //FastLED.show();
  }
  
  FastLED.show();

  state = STANDBY;
  myDFPlayer.play(1);
  delay(200);

}

void loop() {
  
  switch(state){
    case STANDBY:
      if(update_sensor(approaching_distance)){
        state = WORKING;
        leds[9] = CHSV(240, 255, 255);
        brightness_indicator = brightness_max;
        brightness_up = false;
      }else{ 
      }
      break;
    case WORKING:
      myDFPlayer.play(1);
      delay(200);
      leds[0] = CHSV(200, 255, 255);
      leds[1] = CHSV(210, 255, 255);
      leds[2] = CHSV(220, 255, 255);
      FastLED.show();
      servo.write(PIN_SM, SM_RANGE[1]);
      delay(1000);
      servo.write(PIN_SM, SM_RANGE[0]);
      state = WAITING;
      break;
    case WAITING:
      if(update_sensor(going_away_distance)){
        count = 0;
      }else{
        if(count > 5){
          for (int i = 0; i < NUM_LEDS; i++) {
            //leds[i] = CRGB::Blue;
            leds[i] = CHSV(120+i*5, 255, 255);
          }        
          state = STANDBY;
          count = 0;
        }else{
          count++;
          delay(200);
        }
      }
      break;
    default:
      break;
  }

  FastLED.show();
  delay(200);
}

boolean update_sensor(float detect_distance){
  digitalWrite(PIN_US_TRG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_US_TRG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_US_TRG, LOW);
  duration = pulseIn(PIN_US_ECH, HIGH, us_timeout);  // [usec]
  if(duration>0){
    
    //USBSerial.print("Duration:");
    //USBSerial.print(duration);
    //USBSerial.print(", ");
    duration = duration / 2;                               // one way time [usec]
    distance = duration * speed_of_sound * 100 / 1000000;  // [cm]
    //USBSerial.print("Distance:");
    //USBSerial.print(distance);
    //USBSerial.println(" cm");
    
    if(distance<detect_distance){
      return true;
    }
  }
  return false;
}

