#include <SPI.h>
#include <RF24.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Declaring pins
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define encoder_pin_1 1
#define encoder_pin_2 3
#define encoder_sw 4
#define throttle A2
#define pitch A1
#define roll A0
#define yaw A3

//Radio transmitter settings
RF24 radio(9, 10);                        //Digital pins 9 and  10 are used
const uint64_t pipe = 0xE8E8F0F0E1LL;     //This is the writing pipe
//LCD settings
LiquidCrystal_I2C lcd(0x38, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // set the LCD address to 0x38

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Declaring global variables
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct dataStruct{
  int actual_throttle;                                        //Throttle variable
  int actual_pitch;                                           //Pitch variable
  int actual_roll;                                            //Roll variable
  int actual_yaw;                                             //Yaw variable
  bool start;                                                 //Motor start variable
  float p;
  float i;
  float d;
} data;                                                       //Everything saved under "data"
long timer;
int pos = 0;          //0 - null | 1 - P | 2 - I | 3 - D   //Editing

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Setup code
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool start_serial = false;                      //Toggle Serial Communcation

void setup() {
  if(start_serial == true){
    Serial.begin(9600);                         //Start serial communication
    Serial.println("===================================================");
    Serial.println("[INFO]Serial started");
    Serial.println("==================================================="); 
  }

  //FOR CALIBRATING
  data.p = 2.0;
  data.i = 0.0;
  data.d = 15.0;

  
  pinMode(throttle, INPUT);                     //Throttle channel  - Analog Input
  pinMode(pitch,    INPUT);                     //Pitch channel     - Analog Input
  pinMode(roll,     INPUT);                     //Roll channel      - Analog Input
  pinMode(yaw,      INPUT);                     //Yaw channel       - Analog Input
  pinMode(encoder_pin_1, INPUT);                //Encorer pin 1     - Digital Input
  pinMode(encoder_pin_2, INPUT);                //Encoder pin 2     - Digital Input
  pinMode(encoder_sw,    INPUT);                //Encoder switch    - Digital Input
  attachInterrupt(0, updateEncoder, FALLING);   //Attach an interupt on pin 2
  //attachInterrupt(1, updateEncoder, FALLING);   //Attach an interupt on pin 3
  radio.begin();                                //Start the radio
  radio.openWritingPipe(pipe);                  //Open the Writing pipe
  radio.setPALevel(RF24_PA_MAX);                //Set the Power Amplifier level
  radio.setDataRate(RF24_250KBPS);              //Set the Data rate
  lcd.begin (16,4);                             //Start LCD
  data.start = false;
  lcd.write("Motors: OFF");
  lcd.setCursor(0,1);
  lcd.write("P-Value: ");
  lcd.setCursor(0,2);
  lcd.write("I-Value: ");
  lcd.setCursor(0,3);
  lcd.write("D-Value: ");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Main loop
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  //Read the values and map them to usable data (1000 - 2000)
  data.actual_throttle = map(analogRead(throttle), 495, 0,  1000, 2000);                  //Map the measured data from 1000 to 2000 for throttle
  data.actual_pitch    = map(analogRead(pitch),    0, 1023, 2000, 1000);                  //Map the measured data from 1000 to 2000 for pitch
  data.actual_roll     = map(analogRead(roll),     0, 1023, 2000, 1000);                  //Map the measured data from 1000 to 2000 for roll
  data.actual_yaw      = map(analogRead(yaw),      0, 1023, 2000, 1000);                  //Map the measured data from 1000 to 2000 for yaw
  
  //If throttle is lower than 1000, set it to 1000
  if(data.actual_throttle <= 1000) data.actual_throttle = 1000;                 

  if(data.actual_throttle <= 1050 && data.actual_yaw >= 1900 && data.start == false){     //Start the motors if throttle is down and yaw is left
    lcd.setCursor(0,0);
    if(millis() - timer <= 999)  lcd.write("Motors: OFF ****");                        
    else if(millis() - timer <= 1999)  lcd.write("Motors: OFF  ***");
    else if(millis() - timer <= 2999)  lcd.write("Motors: OFF   **");
    else if(millis() - timer <= 3999)  lcd.write("Motors: OFF    *");
    else if(millis() - timer >= 4000){
      data.start = true;                                                                  //Set the start variable to true
      lcd.write("Motors: ON      ");                                                      //Refresh the screen
    }
  }
  else if(data.actual_throttle <= 1050 && data.actual_yaw <= 1050 && data.start == true){ //Stop the motors if throttle is down and yaw is right
    lcd.setCursor(0,0);
    if(millis() - timer <= 499)  lcd.write("Motors: ON  **** ");
    else if(millis() - timer <= 999)  lcd.write("Motors: ON   ***");
    else if(millis() - timer <= 1499)  lcd.write("Motors: ON    **");
    else if(millis() - timer <= 1999)  lcd.write("Motors: ON     *");
    else if(millis() - timer >= 2000){
      data.start = false;                                                                 //Set the start variable to false
      lcd.setCursor(0,0);
      lcd.write("Motors: OFF     ");                                                      //Refresh the screen
    }
  }
  else{
    lcd.setCursor(0,0);
    if(data.start == false){ 
      lcd.write("Motors: OFF     ");                                                      //Refresh the screen
    }
    else{
      lcd.write("Motors: ON      ");                                                      //Refresh the screen
    }
    timer = millis();                                                                     //Set timer to current millis
  }

  lcd.setCursor(9,1);
  lcd.print(String(data.p, 2));
  lcd.setCursor(9,2);
  lcd.print(String(data.i, 2));
  lcd.setCursor(9,3);
  lcd.print(String(data.d, 1));
  
  //Send out the data
  radio.write(&data, sizeof(data));
  if(digitalRead(encoder_sw) == LOW){
    if(pos == 0){
      pos = 1;
      lcd.setCursor(15,1);
      lcd.write("#");
    }
    else if(pos == 1){
      pos = 2;
      lcd.setCursor(15,1);
      lcd.write(" ");
      lcd.setCursor(15,2);
      lcd.write("#");
    }
    else if(pos == 2){
      pos = 3;
      lcd.setCursor(15,2);
      lcd.write(" ");
      lcd.setCursor(15,3);
      lcd.write("#");
    }
    else if(pos == 3){
      pos = 0;
      lcd.setCursor(15,3);
      lcd.write(" ");
    }
    while(digitalRead(encoder_sw) == LOW);
  }
}

void updateEncoder(){
  if(digitalRead(encoder_pin_1) == LOW && digitalRead(encoder_pin_2) == HIGH){
    if(pos == 1)      data.p += 0.1;
    else if(pos == 2) data.i += 0.01;
    else if(pos == 3) data.d += 0.5;
  }
  else{
    if(pos == 1 && data.p != 0.0)      data.p -= 0.1;
    else if(pos == 2 && data.i != 0.0) data.i -= 0.01;
    else if(pos == 3 && data.d != 0.0) data.d -= 0.5;
  }
}

