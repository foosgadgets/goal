 //------------------------------------------------------------------------------------------------------------ 
 //Beta FEB 17 - updated EEPROM.commit(); after writing ----  updated LCD hd44780 --- alignment() --- NEED TO FIX BLESEND() TO SEND THE PROPER MESSAGE, NEED TO SET SENDDATA TO TRUE EACH TIME IT SHOULD SEND... IS THIS NEEDED?  ONLY FOR LASER ALIGNMENT
 //UPDATE APRIL 22, 2021 change "mode" to "type"
#include <Arduino.h>
#include <Wire.h> 
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <EEPROM.h>
hd44780_I2Cexp lcd(0x26); // THIS IS THE 2nd DISPLAY
hd44780_I2Cexp lcd2(0x27); // THIS IS THE MAIN DISPLAY
// BLE STUFF
BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
String NewDeviceName = "FG Test Unit";   //******************************************THIS IS WHERE THE DEVICE NAME GETS STORED*************************************************************************************************************
const int LED = 26; // Should be correct IO26
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool senddata = true; // sets the state whether it should send data or not, if first time connected in loop send otherwise don't
uint8_t txValue = 0;
int buttons;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks { //--------------------------THESE ARE ALL THE RECEIVED COMMANDS FROM THE APP------------------------------------
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);
        Serial.println();
        Serial.println("*********");
        if (rxValue.find("LU") != -1) {
          delay(100);
          buttons = 600;
        }
        if (rxValue.find("LD") != -1) {
          delay(100);
          buttons = 150;
        }
      }
    }
};
// END BLE STUFF

//CONSTANTS
byte light_sense_1 = 1; //Main Outside
byte light_sense_2 = 1; //Main Inside
byte light_sense_3 = 1; //2nd Outside
byte light_sense_4 = 1; //2nd Inside
byte goal; //THIS IS FOR WHICH GOAL HAD A SCORE
byte timeouttaken=0;

int buttons2;
unsigned long start_time = 0;
int miss_time = 0;
int miss_start = 0;
int miss_stop = 0;
float last_speed=0;
float lastL=0;
float lastR=0;
float fast1=0;
float fast2=0;
float fast3=0;
float fast4=0;
byte timeoutL = 2;
byte timeoutR = 2;
byte matchL = 0;
byte matchR = 0;
unsigned long stop_time = 0;
unsigned long total_time = 0;
float mph;
float mull  =  0;
int scoreL = 0;
int scoreR = 0;
int shots = 0;
int shot=1;
int buttontimer=0;
byte quality = 0;
//byte mode = 0;
byte units = 0;
byte player=1;
byte units_name; 
bool align=true;
byte side;
// MENUS
String Item1 = "Single Game *";
String Item2 = "Match Play";
String Item3 = "Practice";
String Item4 = "SpeedBall";
String Item5 = "Units MPH";
String menuItems[] = {Item1, Item2, Item3, Item4, Item5};
// Navigation button variables
int readKey;
// Menu control variables
int menuPage = 0;
int maxMenuPages = round(((sizeof(menuItems) / sizeof(String)) / 2) + .5);
int cursorPosition = 0;
//MINE
//int menubutton = 0;
//  bluetooth stuff
byte type;
byte state;
char mphnd[10];
char mphL[10];
char mphR[10];
char BTlast_speed[20];
char BTfast1[20];
char BTfast2[20];
char BTfast3[20];
char BTfast4[20];
//  bluetooth stuff
char foosball1[20];
char foosball2[20];
String DeviceName;

// Define the bit patterns for each custom chars. These are 5 bits wide and 8 dots deep. LIMIT 8
uint8_t custChar[8][8] = {
  {31, 31, 31, 0, 0, 0, 0, 0},      // Small top line - 0
  {0, 0, 0, 0, 0, 31, 31, 31},      // Small bottom line - 1
  {31, 31, 31, 0, 0, 0, 31, 31},     // lines top and bottom -2
  {0, 0, 0, 0, 0, 15,  15, 15},       // Left lower short - 3
  {7, 15, 15, 15, 15, 15, 15, 7},  // Left side - 4
  {28, 30, 30, 30, 30, 30, 30, 28}, // Right side -5
  {30, 30, 28, 0, 0, 0, 16, 24}, // Right rounds -6
  {15, 15, 7, 0, 0, 0, 1, 3},  // Left round -7
};
// Define numbers 0 thru 9
// 254 is blank and 255 is the "Full Block"
uint8_t bigNums[10][6] = {
  {4, 0, 5, 4, 1, 5},             //0
  {254, 5, 254, 254, 5, 254},     //1
  {7, 2, 5, 4, 1, 1},             //2
  {7, 2, 5, 3, 1, 5},             //3
  {4, 1, 5, 254, 254, 5},         //4
  {4, 2, 6, 3, 1, 5},             //5
  {4, 2, 6, 4, 1, 5},             //6
  {0, 0, 5, 254, 254, 5},         //7
  {4, 2, 5, 4, 1, 5},             //8
  {4, 2, 5, 3, 1, 5},             //9
};
// PRINTS STARTUP MESSAGES CREATE CHAR----------------------------------------------------------------------------------------------------------------- 
void setup()
{
 dtostrf(0, 5, 2, mphL);//THIS CONVERTS MPH TO STRING CHAR ARRAY MPHL SETS THIS TO ZERO
 dtostrf(0, 5, 2, mphR);//THIS CONVERTS MPH TO STRING CHAR ARRAY MPHR SETS THIS TO ZERO
 dtostrf(0, 5, 2, mphnd);//THIS CONVERTS MPH TO STRING CHAR ARRAY MPHR SETS THIS TO ZERO
 pinMode(16, INPUT); //Laser R2
 pinMode(17, INPUT); //Laser R3
 pinMode(18, INPUT); //Laser R3
 pinMode(19, INPUT); //Laser R3
 Wire.setClock(50000); // this works, maybe try even lower for more stability
 EEPROM.begin(512); 
 lcd.begin(16, 2);              // start the library
 lcd2.begin(16, 2);              // start the library
// Create custom character map (8 characters only!)
  for (int cnt = 0; cnt < sizeof(custChar) / 8; cnt++) {
    lcd.createChar(cnt, custChar[cnt]);
    lcd2.createChar(cnt, custChar[cnt]);
  } 
 lcd.setCursor(2,0);
 lcd.print("FOOSGADGETS");
 lcd.setCursor(0,1);
 lcd.print("  Game System");
 lcd2.setCursor(2,0);
 lcd2.print("FOOSGADGETS");
 lcd2.setCursor(0,1);
 lcd2.print("  Game System");
 delay(1000);

 //READ THE MEMORY TYPE 3 SINGLE GAME TYPE 4 MATCH TYPE 5 PRACTICE TYPE 6 SPEEDBALL
  type = EEPROM.read(0);
 if (type>6||type<1) {//Sets default mode 1 if nothing has ever been stored
  type = 3;
 }
 units = EEPROM.read(1);
 if (units==0){//Sets default units to MPH if nothing has ever been stored
  units_name=0;
  mull=.0000176;
 }
 if (units==1){
  units_name = 1;
  mull = .000010936;
 }
 else {
  units_name=0;
  units=0;
  mull=.0000176;
 }
 //START READ MEMORY FOR DEVICE NAME AND CONVERT DEVICE NAME
 for (int i = 98; i < 118; ++i)
  {
    DeviceName += char(EEPROM.read(i));
    }
  if (DeviceName != NewDeviceName) {
    for (int i = 0; i < 20; ++i) { // clear EEPROM values 98-108
          EEPROM.write(98+i, 0);
        }
    for (int i = 0; i < NewDeviceName.length(); ++i)
        {
          EEPROM.write(98 + i, NewDeviceName[i]);
        }
        EEPROM.commit();
        DeviceName = NewDeviceName;
  }

  //Converting device name to character array
  int name_len = DeviceName.length() + 1;
    char BLEName[name_len];
    DeviceName.toCharArray(BLEName, name_len);
  //END READ AND CONVERT DEVICE NAME  
 delay(1000);
// READ THE DONGLES TO SEE IF THEY ARE PLUGGED IN TO DETERMINE IF IT IS A NO DISPLAY SINGLE OR DUAL GOAL SYSTEM
  buttons = analogRead (34);
  buttons2 = analogRead (35);
  if (buttons<200){
    type=2;
  }
  if (buttons2<200){
    type=1;
  }
  

  buttons = analogRead (34);//CHECK SWITCH IS ON
 if (buttons>2000&&buttons<2400){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Use Main Display");
    lcd.setCursor(0,1);
    lcd.print("for Game Setup");
 }
  while (buttons>2000&&buttons<2400){
    menu();
      }
  lcd.clear();
  lcd.setCursor(6,1);
  lcd.print("MODE");
  lcd2.clear();
  lcd2.setCursor(6,1);
  lcd2.print("MODE");
if (type==1||type==2){
  lcd.setCursor(3,0);
  lcd.print("No Display");
  lcd2.setCursor(3,0);
  lcd2.print("No Display");
}
if (type==3){
  lcd.setCursor(3,0);
  lcd.print("Single Game");
  lcd2.setCursor(3,0);
  lcd2.print("Single Game");  
 }
 if (type==4){
  lcd.setCursor(3,0);
  lcd.print("Match Game");
  lcd2.setCursor(3,0);
  lcd2.print("Match Game");  
 }
 if (type==5){
  lcd.setCursor(4,0);
  lcd.print("Practice");
  lcd2.setCursor(4,0);
  lcd2.print("Practice");  
 }
 if (type==6){
  lcd.setCursor(3,0);
  lcd.print("Speedball");
  lcd2.setCursor(3,0);
  lcd2.print("Speedball");  
 }
//BLE STARTUP
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  // Create the BLE Device 
  BLEDevice::init(BLEName);  // --GAMMA--------------------------------------------------ENTER DEVICE NAME HERE------------------------------------------------

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                      BLECharacteristic::PROPERTY_WRITE
                    );
                    
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  
  pServer->getAdvertising()->start();  
  //END BLE STARTUP
  delay(3000);
 
}


void loop() // MAIN PROGRAM LOOP=====================================================================================================================
{
  while (type==1){
// THIS COVERS NON-DISPLAY SINGLE GOAL SYSTEMS
Alignment1();
lcd2.clear();               //clear LCD
 lcd2.setCursor(0,0);            // move cursor to top line
 lcd2.print("No Display Mode");      //
 light_sense_1=0;
 light_sense_2=0; 
 if (deviceConnected){
   senddata=false;
 }
 else {
  senddata=true;
 }
 // READING FOR SHOT
 while (light_sense_1==0) {
    light_sense_1 = digitalRead(16);
    blesendloop();
    quality=0;
 }
 
 start_time=micros();
 miss_start=millis();
 while (light_sense_2==0&&miss_time<500) {
    light_sense_2 = digitalRead(17); 
    miss_stop=millis();
    miss_time=miss_stop-miss_start;
}
 
 
 stop_time=micros();
 miss_time=0;
 total_time=stop_time-start_time;
 mph=(1/(total_time*mull));
if (mph<.13)  {
  quality=2;
}
if (quality==2){
 lcd2.clear();
 lcd2.setCursor(2,0);
 lcd2.print("Not Recorded");
 lcd2.setCursor(1,1);
 lcd2.print("Speed too Slow");
 state=8;
}
if (quality==0){
 lcd2.clear();               //clear LCD
 lcd2.setCursor(0,0);            // move cursor to top line
 lcd2.print("  Shot Speed:");      //
 lcd2.setCursor(4,1);            // move cursor to bottom line
 lcd2.print(mph);      // 
 lcd2.setCursor(9,1);
 lcd2.print("MPH");
 state=6;
}
dtostrf(mph, 5, 2, mphnd);//THIS CONVERTS MPH TO STRING CHAR ARRAY MPHND
//senddata=true;
blesend();

quality = 0;
light_sense_1 = 1;
light_sense_2 = 1;

 delay(2000); 
 }

//---------------------------------------------------------------------------------------------------------------------
  
  while (type==2){
// THIS COVERS NON-DISPLAY DUAL GOAL SYSTEMS
Alignment();
lcd2.clear();               //clear LCD
 lcd2.setCursor(0,0);            // move cursor to top line
 lcd2.print("No Display Mode");      //
 lcd2.setCursor(0,1);
 lcd2.print("Check Goal Cable");
 light_sense_1=0;
 light_sense_2=0;
 light_sense_3=0;
 light_sense_4=0;
 //state=6; 
 senddata=false;
 // READING FOR SHOT
 while (light_sense_1==0&&light_sense_3==0) {
    light_sense_1 = digitalRead(16);
    light_sense_3 = digitalRead(18);
    blesendloop();
    if (light_sense_1==1){ //score on main goal
      goal=1;
    }
    if (light_sense_3==1){ //score on 2nd goal
      goal=2;
    }
    quality=0;
 }
 
 start_time=micros();
 miss_start=millis();
 while (light_sense_2==0&&light_sense_4==0&&miss_time<500) {
    light_sense_2 = digitalRead(17);
    light_sense_4 = digitalRead(19); 
    miss_stop=millis();
    miss_time=miss_stop-miss_start;
}
 
 
 stop_time=micros();
 miss_time=0;
 total_time=stop_time-start_time;
 mph=(1/(total_time*mull));
if (mph<.13)  {
  quality=2;
}
if (quality==2){
 //delay(400); // DELAY TO HELP POTENTIAL CORRUPT DISPLAY
 lcd.clear();
 lcd.setCursor(2,0);
 lcd.print("Not Recorded");
 lcd.setCursor(1,1);
 lcd.print("Speed too Slow");
 lcd2.clear();
 lcd2.setCursor(2,0);
 lcd2.print("Not Recorded");
 lcd2.setCursor(1,1);
 lcd2.print("Speed too Slow");
 state=8;
}
if (quality==0){
 //delay(400); // DELAY TO HELP POTENTIAL CORRUPT DISPLAY
 lcd.clear();               //clear LCD
 lcd.setCursor(0,0);            // move cursor to top line
 lcd.print("  Shot Speed:");      //
 lcd.setCursor(4,1);            // move cursor to bottom line
 lcd.print(mph);      // 
 lcd.setCursor(9,1);
 lcd.print("MPH");
 lcd2.clear();               //clear LCD
 lcd2.setCursor(0,0);            // move cursor to top line
 lcd2.print("  Shot Speed:");      //
 lcd2.setCursor(4,1);            // move cursor to bottom line
 lcd2.print(mph);      // 
 lcd2.setCursor(9,1);
 lcd2.print("MPH");
 state=6;
}
dtostrf(mph, 5, 2, mphnd);//THIS CONVERTS MPH TO STRING CHAR ARRAY MPHND
//senddata=true;
blesend();

quality = 0;
light_sense_1 = 1;
light_sense_2 = 1;
light_sense_3 = 1;
light_sense_4 = 1;

 delay(2000); 
 }
  


  
  while (type==3||type==4){
//GAME PLAY - This covers both single and match game types--------------------------------------------------------------------------------------------
// CHECK LASER ALIGNMENT--------------------------------------------------------------
 Alignment();  
 // END CHECK LASER ALIGMENT-----------------------------------------------------------
 
quality = 0;
light_sense_1 = 0;
light_sense_2 = 0;
light_sense_3 = 0;
light_sense_4 = 0;
if(units==0){
  state=6;
}
if(units==1){
  state=7;
}

buttons=4095;
buttons2=4095;
if (scoreL==10||scoreR==10){// RESET SCORE AFTER 9 SHOTS
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("Max Score");
  lcd.setCursor(2,1);
  lcd.print("Game Reset");
  lcd2.clear();
  lcd2.setCursor(3,0);
  lcd2.print("Max Score");
  lcd2.setCursor(2,1);
  lcd2.print("Game Reset");
  if (type==4){
        if (scoreL>scoreR){
          matchL ++;
        }
        if (scoreR>scoreL){
          matchR ++;
        }
  }
  scoreL = 0;
  scoreR = 0;
  timeoutL = 2;
  timeoutR = 2;  
  dtostrf(0, 5, 2, mphL);
  dtostrf(0, 5, 2, mphR);
  lastL = 0;
  lastR = 0;
  blesend();
  delay(2000);
}
if (matchL==10||matchR==10){
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print("Max Matches");
  lcd.setCursor(2,1);
  lcd.print("Game Reset");
  lcd2.clear();
  lcd2.setCursor(2,0);
  lcd2.print("Max Matches");
  lcd2.setCursor(2,1);
  lcd2.print("Game Reset");
  matchL = 0;
  matchR = 0;
  scoreL = 0;
  scoreR = 0;
  timeoutL = 2;
  timeoutR = 2;
  dtostrf(0, 5, 2, mphL);
  dtostrf(0, 5, 2, mphR);
  lastL = 0;
  lastR = 0;
  blesend();
  delay(2000);
}
 //convert scoreL 
lcd.clear();               //clear LCD
 printBigNum(scoreL,0,0);
 printBigNum(scoreR,4,0);  
 lcd.setCursor(3,0);
 lcd.print("_");
 lcd.setCursor(8,1);
 lcd.print(lastL);
 lcd.setCursor(13,1); 
 if (units==1){
  lcd.print("KPH"); 
 }
 else {
  lcd.print("MPH");
 }
 //convert scoreR 
lcd2.clear();               //clear LCD
 printBigNum2(scoreR,0,0);
 printBigNum2(scoreL,4,0);  
 lcd2.setCursor(3,0);
 lcd2.print("_");  
 lcd2.setCursor(8,1);
 lcd2.print(lastR);
 lcd2.setCursor(13,1);
 if (units==1){
  lcd2.print("KPH"); 
 }
 else {
  lcd2.print("MPH");
 } 
if (type==3){
 lcd2.setCursor(8,0);            
 lcd2.print("TIMEOUT");      
 lcd2.setCursor(15,0);
 lcd2.print(timeoutL);
 lcd.setCursor(8,0);            
 lcd.print("TIMEOUT");      
 lcd.setCursor(15,0);
 lcd.print(timeoutR);
 }
if (type==4){
 lcd.setCursor(8,0);            
 lcd.print("W");      
 lcd.setCursor(9,0);
 lcd.print(matchL);
 lcd.setCursor(11,0);
 lcd.print("L");
 lcd.setCursor(12,0);
 lcd.print(matchR);
 lcd.setCursor(14,0);
 lcd.print("T");
 lcd.setCursor(15,0);
 lcd.print(timeoutR);
 lcd2.setCursor(8,0);            
 lcd2.print("W");      
 lcd2.setCursor(9,0);
 lcd2.print(matchR);
 lcd2.setCursor(11,0);
 lcd2.print("L");
 lcd2.setCursor(12,0);
 lcd2.print(matchL);
 lcd2.setCursor(14,0);
 lcd2.print("T");
 lcd2.setCursor(15,0);
 lcd2.print(timeoutL);
 }
 
 //delay(200);
 //BEGIN SENSING-----------------------------------------------------------------------------------------------------------------------------------------------------
  if (deviceConnected){
   senddata=false;
 }
 else {
  senddata=true;
 }
  while (light_sense_1==0&&light_sense_3==0&&buttons>2100&&buttons2>2100) {
    light_sense_1 = digitalRead(16);
    light_sense_3 = digitalRead(18);
    buttons = analogRead (34);
    buttons2 = analogRead (35);
    blesendloop();
    if ((buttons>1600&&buttons<2100)||(buttons2>1600&&buttons2<2100))  {// NEW GAME button
      newgame();
      //asm volatile ("  jmp 0");  //soft reboot
    }
    if (buttons>1000&&buttons<1600) {//TIMEOUT
      if (timeoutL==0){
        lcd2.clear();
        lcd2.setCursor(3,0);
        lcd2.print("No Timeouts");
        lcd2.setCursor(4,1);
        lcd2.print("Remaining");
        delay(2000);
        quality=3;
        loop();
      }
      timeoutL --;
      timeouttaken=0;
      blesend();
      Timeout();
      quality=3;
      }
      if (buttons2>1000&&buttons2<1600) {//TIMEOUT
      if (timeoutR==0){
        lcd.clear();
        lcd.setCursor(3,0);
        lcd.print("No Timeouts");
        lcd.setCursor(4,1);
        lcd.print("Remaining");
        delay(2000);
        quality=3;
        loop();
      }
      timeoutR --;
      timeouttaken=1;
      blesend();
      Timeout();
      quality=3;
      }    
    if (buttons>300&&buttons<1000)  {// Right Score Up
      scoreR ++;
      blesend();
      quality=3;
      }
    if (buttons>=0&&buttons<300)  {// Right Score Down
      if (scoreR>0){
        scoreR --;
      blesend();
      }
      quality=3;
    }
    if (buttons2>300&&buttons2<1000)  {// Left Score Up
      scoreL ++;
      blesend();
      quality=3;
    }
    if (buttons2>=0&&buttons2<300)  {// Left Score Down
      if (scoreL>0){
        scoreL --;
      blesend();
        }
        quality=3;
    }
    if (light_sense_1==1) {             // Keeps score
      //scoreL ++;
      side=0;
    }
    if (light_sense_3==1) {             // Keeps score
      //scoreR ++;
      side=1;
    }
 }
 start_time=micros();
 miss_start=millis();
 while (light_sense_2==0&&light_sense_4==0&&miss_time<500) {
    light_sense_2 = digitalRead(17);     
    light_sense_4 = digitalRead(19);
    miss_stop=millis();
    miss_time=miss_stop-miss_start;    
 }
 stop_time=micros();
 miss_time=0;
 total_time=stop_time-start_time;
 mph=(1/(total_time*mull));
 if (total_time>55000&&quality==0)  { //THIS IS EQUAL TO 1.03MPH
  quality=2;
}
 if (total_time<1400&&quality==0){    //THIS IS EQUAL TO 40MPH
  quality=2;
 }
if (quality==1){
 lcd.clear();
 lcd.setCursor(4,0);
 lcd.print("New Game");
 lcd.setCursor(2,1);
 lcd.print("Score Reset");
 lcd2.clear();
 lcd2.setCursor(4,0);
 lcd2.print("New Game");
 lcd2.setCursor(2,1);
 lcd2.print("Score Reset");
 blesend();
 delay(3000);
}
if (quality==2){
 lcd.clear();
 lcd.setCursor(2,0);
 lcd.print(" Shot Speed");
 lcd.setCursor(1,1);
 lcd.print(" Not Recorded");
 lcd2.clear();
 lcd2.setCursor(2,0);
 lcd2.print(" Shot Speed");
 lcd2.setCursor(1,1);
 lcd2.print(" Not Recorded");
 if (side==0){
    scoreL ++;
  }
  if (side==1){
    scoreR ++;
  }
  state=8;
 blesend();
 delay(3000);
}
if (quality==0){
  // Convert mph float to single digit integers
  if (side==0){
    lastL=mph;
    scoreL ++;
  }
  if (side==1){
    lastR=mph;
    scoreR ++;
  }
  dtostrf(lastL, 5, 2, mphL);//THIS CONVERTS MPH TO STRING CHAR ARRAY MPHL
  dtostrf(lastR, 5, 2, mphR);//THIS CONVERTS MPH TO STRING CHAR ARRAY MPHR
  int mphconvert = (mph+.005)*100;
  char array[4];
sprintf(array, "%i", mphconvert);
int mph1 = array[0] - '0'; 
int mph2 = array[1] - '0'; 
int mph3 = array[2] - '0'; 
int mph4 = array[3] - '0';
lcd.clear();
lcd2.clear();
// Format is: number2print, startCol, startRow
  if (mph >= 10){
    printBigNum(mph1, 0, 0);
    printBigNum(mph2, 3, 0);
    printBigNum(mph3, 7, 0);
    printBigNum(mph4, 10, 0);
    printBigNum2(mph1, 0, 0);
    printBigNum2(mph2, 3, 0);
    printBigNum2(mph3, 7, 0);
    printBigNum2(mph4, 10, 0);
  }
  else {
    printBigNum(mph1, 3, 0);
    printBigNum(mph2, 7, 0);  
    printBigNum(mph3, 10, 0);
    printBigNum2(mph1, 3, 0);
    printBigNum2(mph2, 7, 0);
    printBigNum2(mph3, 10, 0);
  }
  lcd.setCursor(6,1);
  lcd.print((char)161);  
  lcd2.setCursor(6,1);
  lcd2.print((char)161);
  lcd.setCursor(13,1);
  if (units_name==1){
    lcd.print("KPH");
  }
  else {
    lcd.print("MPH");
  }
  lcd2.setCursor(13,1);
  if (units_name==1){
    lcd2.print("KPH");
  }
  else {
    lcd2.print("MPH");
  }
 blesend(); 
delay(3000);
}
 }
  while (type==5) {// PRACTICE MODE========================================================================================================================
    // CHECK LASER ALIGNMENT--------------------------------------------------------------
 Alignment();   
 // END CHECK LASER ALIGMENT-----------------------------------------------------------
quality = 0;
if(units==0){
  state=6;
}
if(units==1){
  state=7;
}

quality = 0;
light_sense_1 = 0;
light_sense_2 = 0;
light_sense_3 = 0;
light_sense_4 = 0;
buttons=4095;
buttons2=4095;
if (shots==100){// RESET SCORE AFTER 99 SHOTS
  lcd.clear();
  lcd.setCursor(3,0);
  lcd.print("Max Shots");
  lcd.setCursor(2,1);
  lcd.print("Shots Reset");
  lcd2.clear();
  lcd2.setCursor(3,0);
  lcd2.print("Max Score");
  lcd2.setCursor(2,1);
  lcd2.print("Game Reset");
  shots=0;
  blesend();
// foosball=(String)type+","+state+","+last_speed+","+fast1+","+shots;
//  FooSerial.println(foosball);
    } 
 //convert scoreL
   char arrayshot[2];
sprintf(arrayshot, "%i", shots);
int shots1 = arrayshot[0] - '0'; 
int shots2 = arrayshot[1] - '0'; 
lcd.clear();               //clear LCD
lcd2.clear();
if (shots >= 10){
    printBigNum(shots1, 0, 0);
    printBigNum(shots2, 3, 0);
    printBigNum2(shots1, 0, 0);
    printBigNum2(shots2, 3, 0);
  }
  else {
    printBigNum(shots1, 3, 0);
    printBigNum2(shots1, 3, 0);
  }  
 lcd.setCursor(7,0);
 lcd.print("Lst");
 lcd.setCursor(11,0);            
 lcd.print(last_speed);      
 lcd.setCursor(7,1);
 lcd.print("Max");
 lcd.setCursor(11,1);            
 lcd.print(fast1);
 lcd2.setCursor(7,0);
 lcd2.print("Lst");
 lcd2.setCursor(11,0);            
 lcd2.print(last_speed);      
 lcd2.setCursor(7,1);
 lcd2.print("Max");
 lcd2.setCursor(11,1);            
 lcd2.print(fast1);
 if(units==1){
  state=7;
}
if(units==0){
  state=6;
} 
 if(quality==2){
  state=8;
 }

  
  //BEGIN SENSING 
  if (deviceConnected){
   senddata=false;
 }
 else {
  senddata=true;
 }
 while (light_sense_1==0&&light_sense_3==0&&buttons>3200&&buttons2>2100) {
    light_sense_1 = digitalRead(16);
    light_sense_3 = digitalRead(18);
    buttons = analogRead (34);
    buttons2 = analogRead (35);
    blesendloop();
    if ((buttons<3200)||(buttons2<2100))  {// NEW GAME button
      newgame();
      }    
    if (light_sense_1==1||light_sense_3==1) {      
      quality=0;
    }
 }
 start_time=micros();
 miss_start=millis();
 while (light_sense_2==0&&light_sense_4==0&&miss_time<500) {
    light_sense_2 = digitalRead(17);     
    light_sense_4 = digitalRead(19);
    miss_stop=millis();
    miss_time=miss_stop-miss_start;    
 } 
 stop_time=micros();
 miss_time=0;
 total_time=stop_time-start_time;
 mph=(1/(total_time*mull));
 if (total_time>55000&&quality==0)  { //THIS IS EQUAL TO 1.03MPH
  quality=2;
}
 if (total_time<1400&&quality==0){    //THIS IS EQUAL TO 40MPH
  quality=2;
 }
if (quality==1){
 lcd.clear();
 lcd.setCursor(5,0);
 lcd.print("Practice");
 lcd.setCursor(5,1);
 lcd.print(" Reset");
 lcd2.clear();
 lcd2.setCursor(5,0);
 lcd2.print("Practice");
 lcd2.setCursor(5,1);
 lcd2.print(" Reset");
 delay(3000);
}
if (quality==2){
 lcd.clear();
 lcd.setCursor(2,0);
 lcd.print(" Shot Speed");
 lcd.setCursor(1,1);
 lcd.print(" Not Recorded");
 lcd2.clear();
 lcd2.setCursor(2,0);
 lcd2.print(" Shot Speed");
 lcd2.setCursor(1,1);
 lcd2.print(" Not Recorded");
 shots ++;
 state=8;
 blesend();
 delay(3000);
}
if (quality==0){
  shots ++;
  // Convert mph float to single digit integers
  if (fast1<mph){
      fast1=mph;
    }
    last_speed=mph;
          
  dtostrf(fast1, 5, 2, BTfast1);//THIS CONVERTS TO STRING CHAR ARRAY
  dtostrf(last_speed, 5, 2, BTlast_speed);//THIS CONVERTS TO STRING CHAR ARRAY
  int mphconvert = (mph+.005)*100;
  char arrayS[4];
sprintf(arrayS, "%i", mphconvert);
int mph1 = arrayS[0] - '0'; 
int mph2 = arrayS[1] - '0'; 
int mph3 = arrayS[2] - '0'; 
int mph4 = arrayS[3] - '0';
blesend();
// foosball=(String)type+","+state+","+last_speed+","+fast1+","+shots;
//  FooSerial.println(foosball);
  lcd.clear();
lcd2.clear();
// Format is: number2print, startCol, startRow
  if (mph >= 10){
    printBigNum(mph1, 0, 0);
    printBigNum(mph2, 3, 0);
    printBigNum(mph3, 7, 0);
    printBigNum(mph4, 10, 0);
    printBigNum2(mph1, 0, 0);
    printBigNum2(mph2, 3, 0);
    printBigNum2(mph3, 7, 0);
    printBigNum2(mph4, 10, 0);
  }
  else {
    printBigNum(mph1, 3, 0);
    printBigNum(mph2, 7, 0);  
    printBigNum(mph3, 10, 0);
    printBigNum2(mph1, 3, 0);
    printBigNum2(mph2, 7, 0);
    printBigNum2(mph3, 10, 0);
  }
  lcd.setCursor(6,1);
  lcd.print((char)161);  
  lcd2.setCursor(6,1);
  lcd2.print((char)161);
  lcd.setCursor(13,1);
  lcd2.setCursor(13,1);
  if (units_name==1){
    lcd.print("KPH");
    lcd2.print("KPH");
  }
  else {
    lcd.print("MPH");
    lcd2.print("MPH");
  }  
  
delay(3000);
  }

 }
  while (type==6){ // SPEEDBALL MODE===========================================================================================================================
  // CHECK LASER ALIGNMENT--------------------------------------------------------------
 Alignment();   
 // END CHECK LASER ALIGMENT-----------------------------------------------------------
quality = 0;
if(units==1){
  state=7;
}
if(units==0){
  state=6;
}
quality = 0;
light_sense_1 = 0;
light_sense_2 = 0;
light_sense_3 = 0;
light_sense_4 = 0;
buttons=4095;
buttons2=4095;
if(units==0){
  state=6;
}
if(units==1){
  state=7;
}
if(quality==2){
  state=8;
 }
  
if (shot==4) {
  lcd.clear();  
  lcd.setCursor(0,0);
  lcd.print("1-      3-");
  lcd.setCursor(2,0);
  lcd.print(fast1);
  lcd.setCursor(10,0);
  lcd.print(fast3);
  lcd.setCursor(0,1);
  lcd.print("2-      4-");
  lcd.setCursor(2,1);
  lcd.print(fast2);
  lcd.setCursor(10,1);
  lcd.print(fast4);
  lcd2.clear();  
  lcd2.setCursor(0,0);
  lcd2.print("1-      3-");
  lcd2.setCursor(2,0);
  lcd2.print(fast1);
  lcd2.setCursor(10,0);
  lcd2.print(fast3);
  lcd2.setCursor(0,1);
  lcd2.print("2-      4-");
  lcd2.setCursor(2,1);
  lcd2.print(fast2);
  lcd2.setCursor(10,1);
  lcd2.print(fast4);
  shot=1;
  player ++;
while (buttons>3200&&buttons2>2100){// Waiting for button to move to next player
  buttons = analogRead (34);
  buttons2 = analogRead (35);
  if (buttons>2400&&buttons<3200||buttons2>1600&&buttons2<2100)  {// NEW GAME button
      newgame();      
    }
}
}
if (player==5){
    fast1=0;
    fast2=0;
    fast3=0;
    fast4=0;
    dtostrf(0, 5, 2, BTfast1);
      dtostrf(0, 5, 2, BTfast2);
      dtostrf(0, 5, 2, BTfast3);
      dtostrf(0, 5, 2, BTfast4);
      dtostrf(0, 5, 2, mphL);
      dtostrf(0, 5, 2, mphR);
      dtostrf(0, 5, 2, BTlast_speed);
      shot=1;
      player=1;
      quality=1;
      blesend();    
 lcd.clear();
 lcd.setCursor(4,0);
 lcd.print("Speedball");
 lcd.setCursor(5,1);
 lcd.print(" Reset");
 lcd2.clear();
 lcd2.setCursor(4,0);
 lcd2.print("Speedball");
 lcd2.setCursor(5,1);
 lcd2.print(" Reset");
 quality=0;
 delay(3000);
  }
buttons=4095;
buttons2=4095;
delay(500);
if (shot<4) {
  lcd.clear();
  lcd.setCursor(4,0);
  lcd.print("Player");
  lcd.setCursor(11,0);
  lcd.print(player);
  lcd.setCursor(0,1);
  lcd.print("Ready for Shot");
  lcd.setCursor(15,1);
  lcd.print(shot);
  lcd2.clear();
  lcd2.setCursor(4,0);
  lcd2.print("Player");
  lcd2.setCursor(11,0);
  lcd2.print(player);
  lcd2.setCursor(0,1);
  lcd2.print("Ready for Shot");
  lcd2.setCursor(15,1);
  lcd2.print(shot);
}

  //BEGIN SENSING 
  if (deviceConnected){
   senddata=false;
 }
 else {
  senddata=true;
 }
 while (light_sense_1==0&&light_sense_3==0&&buttons>3200&&buttons2>2100) {
    light_sense_1 = digitalRead(16);
    light_sense_3 = digitalRead(18);
    buttons = analogRead (34);
    buttons2 = analogRead (35);
    blesendloop();
    if (buttons>2400&&buttons<3200||buttons2>1600&&buttons2<2100)  {// NEW GAME button
      newgame();      
    }
    if (buttons<2400||buttons2<1600){
      loop();
    }
    if (light_sense_1==1||light_sense_3==1) {
      shot ++;
      //quality=0;
      }
 }
 start_time=micros();
 miss_start=millis();
 while (light_sense_2==0&&light_sense_4==0&&miss_time<500) {
    light_sense_2 = digitalRead(17);     
    light_sense_4 = digitalRead(19);
    miss_stop=millis();
    miss_time=miss_stop-miss_start;    
 } 
 stop_time=micros();
 miss_time=0;
 total_time=stop_time-start_time;
 mph=(1/(total_time*mull));
 if (total_time>55000&&quality==0)  { //THIS IS EQUAL TO 1.03MPH
  quality=2;
}
 if (total_time<1400&&quality==0){    //THIS IS EQUAL TO 40MPH
  quality=2;
 }
if (quality==1){
 lcd.clear();
 lcd.setCursor(4,0);
 lcd.print("Speedball");
 lcd.setCursor(5,1);
 lcd.print(" Reset");
 lcd2.clear();
 lcd2.setCursor(4,0);
 lcd2.print("Speedball");
 lcd2.setCursor(5,1);
 lcd2.print(" Reset");
 delay(3000);
}
if (quality==2){
 lcd.clear();
 lcd.setCursor(2,0);
 lcd.print(" Shot Speed");
 lcd.setCursor(1,1);
 lcd.print(" Not Recorded");
 lcd2.clear();
 lcd2.setCursor(2,0);
 lcd2.print(" Shot Speed");
 lcd2.setCursor(1,1);
 lcd2.print(" Not Recorded");
 shot --;
 delay(3000);
}
if (quality==0){
  // Convert mph float to single digit integers
  if (player==1){
    if(fast1<mph){
      fast1=mph;
    }
  }
  if (player==2){
    if(fast2<mph){
      fast2=mph;
    }
  }
  if (player==3){
    if(fast3<mph){
      fast3=mph;
    }
  }
  if (player==4){
    if(fast4<mph){
      fast4=mph;
    }
  }
  dtostrf(mph, 5, 2, BTlast_speed);//THIS CONVERTS TO STRING CHAR ARRAY
  dtostrf(fast1, 5, 2, BTfast1);//THIS CONVERTS TO STRING CHAR ARRAY
  dtostrf(fast2, 5, 2, BTfast2);//THIS CONVERTS TO STRING CHAR ARRAY
  dtostrf(fast3, 5, 2, BTfast3);//THIS CONVERTS TO STRING CHAR ARRAY
  dtostrf(fast4, 5, 2, BTfast4);//THIS CONVERTS TO STRING CHAR ARRAY
  int mphconvert = (mph+.005)*100;
  char arrayS[4];
sprintf(arrayS, "%i", mphconvert);
int mph1 = arrayS[0] - '0'; 
int mph2 = arrayS[1] - '0'; 
int mph3 = arrayS[2] - '0'; 
int mph4 = arrayS[3] - '0';
blesend();
lcd.clear();
lcd2.clear();
// Format is: number2print, startCol, startRow
  if (mph >= 10){
    printBigNum(mph1, 0, 0);
    printBigNum(mph2, 3, 0);
    printBigNum(mph3, 7, 0);
    printBigNum(mph4, 10, 0);
    printBigNum2(mph1, 0, 0);
    printBigNum2(mph2, 3, 0);
    printBigNum2(mph3, 7, 0);
    printBigNum2(mph4, 10, 0);
  }
  else {
    printBigNum(mph1, 3, 0);
    printBigNum(mph2, 7, 0);  
    printBigNum(mph3, 10, 0);
    printBigNum2(mph1, 3, 0);
    printBigNum2(mph2, 7, 0);
    printBigNum2(mph3, 10, 0);
  }
  lcd.setCursor(6,1);
  lcd.print((char)161);  
  lcd2.setCursor(6,1);
  lcd2.print((char)161);
  lcd.setCursor(13,1);
  lcd2.setCursor(13,1);
  if (units_name==0){
    lcd.print("MPH");
    lcd2.print("MPH");
  }
  else {
    lcd.print("KPH");
    lcd2.print("KPH");
  }
delay(3000);
}
}
  
}



// DEFINE FUNCTION CALL Print big number over 2 lines, 3 colums per half digit LCD1-----------------------------------------------------------------
void printBigNum(int number, int startCol, int startRow) {
int cnt=0;
  // Position cursor to requested position (each char takes 3 cols plus a space col)
  lcd.setCursor(startCol, startRow);
  // Each number split over two lines, 3 chars per line. Retrieve character
  // from the main array to make working with it here a bit easier.
  uint8_t thisNumber[6];
  for (int cnt = 0; cnt < 6; cnt++) {
    thisNumber[cnt] = bigNums[number][cnt];
  }
  // First line (top half) of digit
  for (int cnt = 0; cnt < 3; cnt++) {
    lcd.print((char)thisNumber[cnt]);
  }
  // Now position cursor to next line at same start column for digit
  lcd.setCursor(startCol, startRow + 1);;
  // 2nd line (bottom half)
  for (int cnt = 3; cnt < 6; cnt++) {
    lcd.print((char)thisNumber[cnt]);    
  }
}
// DEFINE FUNCTION CALL Print big number over 2 lines, 3 colums per half digit LCD2-----------------------------------------------------------------
void printBigNum2(int number, int startCol, int startRow) {
int cnt2=0;
  // Position cursor to requested position (each char takes 3 cols plus a space col)
  lcd2.setCursor(startCol, startRow);
  // Each number split over two lines, 3 chars per line. Retrieve character
  // from the main array to make working with it here a bit easier.
  uint8_t thisNumber[6];
  for (int cnt = 0; cnt < 6; cnt++) {
    thisNumber[cnt] = bigNums[number][cnt];
  }
  // First line (top half) of digit
  for (int cnt = 0; cnt < 3; cnt++) {
    lcd2.print((char)thisNumber[cnt]);
  }
  // Now position cursor to next line at same start column for digit
  lcd2.setCursor(startCol, startRow + 1);
  // 2nd line (bottom half)
  for (int cnt = 3; cnt < 6; cnt++) {
    lcd2.print((char)thisNumber[cnt]);    
  }
}
//FUNCTION CALL FOR LASER ALIGNMENT CHECK------------------------------------------------------------------------------------------------------------------------
void Alignment(){
 //senddata=true;
 //state=6;
    light_sense_1 = digitalRead(16);
    light_sense_2 = digitalRead(17);
    light_sense_3 = digitalRead(18);
    light_sense_4 = digitalRead(19);
    if (light_sense_1==1||light_sense_2==1||light_sense_3==1||light_sense_4==1){
    senddata=true;
    lcd.clear();
    lcd.setCursor(0,0);            // move cursor to top line
    lcd.print(" Align Lasers:");      //
    lcd2.clear();
    lcd2.setCursor(0,0);            // move cursor to top line
    lcd2.print(" Align Lasers:");      //
    } 
 while (light_sense_1==1||light_sense_2==1||light_sense_3==1||light_sense_4==1) {
    light_sense_1 = digitalRead(16);
    light_sense_2 = digitalRead(17);
    light_sense_3 = digitalRead(18);
    light_sense_4 = digitalRead(19);
    //if (light_sense_1==1||light_sense_2==1||light_sense_3==1||light_sense_4==1){
    //senddata=true;
    //lcd.clear();
    //lcd.setCursor(0,0);            // move cursor to top line
    //lcd.print(" Align Lasers:");      //
    //lcd2.clear();
    //lcd2.setCursor(0,0);            // move cursor to top line
    //lcd2.print(" Align Lasers:");      //
    //}
    if (light_sense_1==1){//Main out
      lcd.setCursor(0,1);
      lcd.print("MAIN  O");
      lcd2.setCursor(0,1);
      lcd2.print("MAIN  O");
      state=1;
    }
    if (light_sense_2==1){//Main in
      lcd.setCursor(0,1);
      lcd.print("MAIN I");
      lcd2.setCursor(0,1);
      lcd2.print("MAIN I");
      state=2;
    }
    if (light_sense_3==1){//2nd out
      lcd.setCursor(8,1);
      lcd.print("2ND  O");
      lcd2.setCursor(8,1);
      lcd2.print("2ND  O");
      state=3;
    }
    if (light_sense_4==1){//2nd in
      lcd.setCursor(8,1);
      lcd.print("2ND I");
      lcd2.setCursor(8,1);
      lcd2.print("2ND I");
      state=4;
    }
    blesendloop();
    senddata=false;
    delay(500);
 }
 if (state>=1&&state<=4){
  state=5;
  blesend();
 }
 }

//END ALIGNMENT CHECK--------------------------------------------------------------------------------------------------------------------------------------------

//FUNCTION CALL FOR LASER ALIGNMENT CHECK SINGLE GOAL------------------------------------------------------------------------------------------------------------------------
void Alignment1(){
    light_sense_1 = digitalRead(16);
    light_sense_2 = digitalRead(17);
    if (light_sense_1==1||light_sense_2==1){
    senddata=true;
    lcd.clear();
    lcd.setCursor(0,0);            // move cursor to top line
    lcd.print(" Align Lasers:");      //
    lcd2.clear();
    lcd2.setCursor(0,0);            // move cursor to top line
    lcd2.print(" Align Lasers:");      //
    } 
 while (light_sense_1==1||light_sense_2==1) {
    light_sense_1 = digitalRead(16);
    light_sense_2 = digitalRead(17);
    if (light_sense_1==1){//Main out
      lcd.setCursor(0,1);
      lcd.print("MAIN  O");
      lcd2.setCursor(0,1);
      lcd2.print("MAIN  O");
      state=1;
    }
    if (light_sense_2==1){//Main in
      lcd.setCursor(0,1);
      lcd.print("MAIN I");
      lcd2.setCursor(0,1);
      lcd2.print("MAIN I");
      state=2;
    }
    blesendloop();
    senddata=false;
    delay(500);
 }
 if (state>=1&&state<=4){
  state=5;
  blesend();
 }
 }

//END ALIGNMENT CHECK--------------------------------------------------------------------------------------------------------------------------------------------


//FUNCTION CALL FOR TIMEOUT--------------------------------------------------------------------------------------------------------------------------------------
void Timeout(){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Timeout:");
      lcd2.clear();
      lcd2.setCursor(0,0);
      lcd2.print("Timeout:");
      int timer = 30;
      while (timer >= 0){
        int time1 = (timer/10);
        int time2 = (timer-(time1*10));
        //char array[2];
        //sprintf(array, "%i", timer);
        //int time1 = array[0] - '0';
        //int time2 = array[1] - '0';
      if (timer >= 10) {
        printBigNum(time1,9,0);
        printBigNum(time2,12,0);
        printBigNum2(time1,9,0);
        printBigNum2(time2,12,0);
      }
      else {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Timeout:");
        lcd2.clear();
        lcd2.setCursor(0,0);
        lcd2.print("Timeout:");
        printBigNum(time2,12,0);
        printBigNum2(time2,12,0);
      }
       delay (980);// this accounts for the delay to draw to the LCD
       timer --;
       if (timeouttaken==0) {
       buttons = analogRead(34);//button is only read every second, HOLD TO RESET
       if (buttons>1400&&buttons<2400) {
        timer = 0;
       }
       }
       if (timeouttaken==1) {
       buttons2 = analogRead(35);//button is only read every second, HOLD TO RESET
       if (buttons2>1000&&buttons2<1600) {
        timer = 0;
       }
       }
      }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  Timeout Over");
  lcd.setCursor(0,1);
  lcd.print("    GAME ON!");
  lcd2.clear();
  lcd2.setCursor(0,0);
  lcd2.print("  Timeout Over");
  lcd2.setCursor(0,1);
  lcd2.print("    GAME ON!");
  delay(2000);
  loop();
}

//FUNCTION CALLS FOR MENUS------------------------------------------------------------------------------------------------------------------------------------------
// This function will generate the 2 menu items that can fit on the screen. They will change as you scroll through your menu. 
// Up and down arrows will indicate your current menu position.
void mainMenuDraw() {
  if (type == 3){ // when loading a stored mode, this is required to print the initial menu correctly
  menuItem1();  
 }
 if (type == 4){
  menuItem2();  
 }
 if (type == 5){
  menuItem3();  
 }
 if (type == 6){
  menuItem4();  
 }
 if (units==1){
  Item5 = "Units KPH";
 }
 else{
  Item5 = "Units MPH";
 }
  String menuItems[] = {Item1, Item2, Item3, Item4, Item5};
  lcd2.clear();
  lcd2.setCursor(1, 0);
  lcd2.print(menuItems[menuPage]);
  lcd2.setCursor(1, 1);
  lcd2.print(menuItems[menuPage + 1]);
  if (menuPage == 0) {
    lcd2.setCursor(15, 1);
    lcd2.print("+");
  } else if (menuPage > 0 and menuPage < maxMenuPages) {
    lcd2.setCursor(15, 1);
    lcd2.print("+");
    lcd2.setCursor(15, 0);
    lcd2.write("-");
  } else if (menuPage == maxMenuPages) {
    lcd2.setCursor(15, 0);
    lcd2.print("-");
  }
}
//When called, this function will erase the current cursor and redraw it based on the cursorPosition and menuPage variables.-------------------------------------------
void drawCursor() {
  for (int x = 0; x < 2; x++) {     // Erases current cursor
    lcd2.setCursor(0, x);
    lcd2.print(" ");
  }

  // The menu is set up to be progressive (menuPage 0 = Item 1 & Item 2, menuPage 1 = Item 2 & Item 3, menuPage 2 = Item 3 & Item 4), so
  // in order to determine where the cursor should be you need to see if you are at an odd or even menu page and an odd or even cursor position.
  if (menuPage % 2 == 0) {
    if (cursorPosition % 2 == 0) {  // If the menu page is even and the cursor position is even that means the cursor should be on line 1
      lcd2.setCursor(0, 0);
      lcd2.print((char)B01111110);
    }
    if (cursorPosition % 2 != 0) {  // If the menu page is even and the cursor position is odd that means the cursor should be on line 2
      lcd2.setCursor(0, 1);
      lcd2.print((char)B01111110);
    }
  }
  if (menuPage % 2 != 0) {
    if (cursorPosition % 2 == 0) {  // If the menu page is odd and the cursor position is even that means the cursor should be on line 2
      lcd2.setCursor(0, 1);
      lcd2.print((char)B01111110);
    }
    if (cursorPosition % 2 != 0) {  // If the menu page is odd and the cursor position is odd that means the cursor should be on line 1
      lcd2.setCursor(0, 0);
      lcd2.print((char)B01111110);
    }
  }
}


void operateMainMenu() {
  int activeButton = 0;
  while (activeButton == 0) {
    int button;
    readKey = analogRead(34);
    if (readKey < 2100) {
      delay(100);
      readKey = analogRead(34);
    }
    button = evaluateButton(readKey);
    switch (button) {
      case 0: // When button returns as 0 there is no action taken
        break;
      case 3:  // This case will execute if the "forward" button is pressed
        button = 0;
        switch (cursorPosition) { // The case that is selected here is dependent on which menu page you are on and where the cursor is.
          case 0:
            menuItem1();
            break;
          case 1:
            menuItem2();
            break;
          case 2:
            menuItem3();
            break;
          case 3:
            menuItem4();
            break;
          case 4:
            menuItem5();
            break;
        }
        activeButton = 1;
        mainMenuDraw();
        drawCursor();
        break;
      case 1:
        button = 0;
        if (menuPage == 0) {
          cursorPosition = cursorPosition - 1;
          cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));
        }
        if (menuPage % 2 == 0 and cursorPosition % 2 == 0) {
          menuPage = menuPage - 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        if (menuPage % 2 != 0 and cursorPosition % 2 != 0) {
          menuPage = menuPage - 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }
        cursorPosition = cursorPosition - 1;
        cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));

        mainMenuDraw();
        drawCursor();
        activeButton = 1;
        break;
      case 2:
        button = 0;
        if (menuPage % 2 == 0 and cursorPosition % 2 != 0) {
          menuPage = menuPage + 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }

        if (menuPage % 2 != 0 and cursorPosition % 2 == 0) {
          menuPage = menuPage + 1;
          menuPage = constrain(menuPage, 0, maxMenuPages);
        }
        
        cursorPosition = cursorPosition + 1;
        cursorPosition = constrain(cursorPosition, 0, ((sizeof(menuItems) / sizeof(String)) - 1));
        mainMenuDraw();
        drawCursor();
        activeButton = 1;
        break;
     case 4:
        button = 0;
        break;
     case 5:
        button = 0;
        lcd2.clear();
        lcd2.setCursor(0,0);
        lcd2.print(" Options Saved");
        EEPROM.put(0,type);
        EEPROM.commit();
        if (Item5=="Units MPH"){
          int units=0;
        }
        else{
          int units=1;
        }
        EEPROM.put(1,units);
        EEPROM.commit();
        lcd2.clear();
        lcd2.setCursor(0,0);
        lcd2.print(" Options Saved");
        delay(2000);
        menu();
        break;      
    }
  }
}
//This function is called whenever a button press is evaluated. The LCD shield works by observing a voltage drop across the buttons all hooked up to 34.----------
int evaluateButton(int x) {
  int result = 0;
  if (x < 300) {
    result = 1; // up
  } else if (x < 1000) {
    result = 2; // down
  } else if (x < 1600) {
    result = 3; // accept
  } else if (x < 1800) {// THIS BUTTON CHOICE SHOULDN'T EVEN EXIST??????????????????????????????????
    result = 4; // NOTHING
  } else if (x < 2100) {
    result = 5; // save
  }
  return result;
}

void menuItem1() { // Function executes when you select the 1st item from main menu
  int activeButton = 0;
  type = 3;
  Item1 = "Single Game *";
  Item2 = "Match Play";
  Item3 = "Practice";
  Item4 = "SpeedBall";
}

void menuItem2() { // Function executes when you select the 2nd item from main menu
  int activeButton = 0;
  type = 4;
  Item1 = "Single Game";
  Item2 = "Match Play *";
  Item3 = "Practice";
  Item4 = "SpeedBall";
}

void menuItem3() { // Function executes when you select the 3rd item from main menu
  int activeButton = 0;
  type = 5;
  Item1 = "Single Game";
  Item2 = "Match Play";
  Item3 = "Practice *";
  Item4 = "SpeedBall";
}

void menuItem4() { // Function executes when you select the 4th item from main menu
  int activeButton = 0;
  type = 6;
  Item1 = "Single Game";
  Item2 = "Match Play";
  Item3 = "Practice";
  Item4 = "SpeedBall *";
}

void menuItem5() { // Function executes when you select the 5th item from main menu
  int activeButton = 0;
  if (Item5 == "Units MPH"){
    Item5 = "Units KPH";
    units = 1;
  }
  else {
    Item5 = "Units MPH";
    units = 0;
  }
}

//
void menu(){
    mainMenuDraw();
    drawCursor();
    operateMainMenu();
}

 void blesend (){
  //START BLE **************************************************************
    if (deviceConnected) {
          if (type==1||type==2){
          sprintf(foosball1,"%u,%u,%s,%u",type,state,mphnd,goal);
          pTxCharacteristic->setValue(foosball1); // ONLY SENDS 20 CHARACTERS
          pTxCharacteristic->notify();  
          }
          if (type==3||type==4){
          sprintf(foosball1,"%u,%u,%s,%s,",type,state,mphL,mphR); 
          pTxCharacteristic->setValue(foosball1); // ONLY SENDS 20 CHARACTERS
          pTxCharacteristic->notify();
          sprintf(foosball2,"%i,%i,%u,%u,%u,%u",scoreL,scoreR,timeoutL,timeoutR,matchL,matchR); 
          pTxCharacteristic->setValue(foosball2); // ONLY SENDS 20 CHARACTERS
          pTxCharacteristic->notify();
          }
          if (type==5){
            sprintf(foosball1,"%u,%u,%s,%s,%i",type,state,BTlast_speed,BTfast1,shots); 
          pTxCharacteristic->setValue(foosball1); // ONLY SENDS 20 CHARACTERS
          pTxCharacteristic->notify();
          }
          if (type==6){
            sprintf(foosball1,"%u,%u,%s,%s,",type,state,BTlast_speed,BTfast1); 
          pTxCharacteristic->setValue(foosball1); // ONLY SENDS 20 CHARACTERS
          pTxCharacteristic->notify();
          sprintf(foosball2,"%s,%s,%s",BTfast2,BTfast3,BTfast4); 
          pTxCharacteristic->setValue(foosball2); // ONLY SENDS 20 CHARACTERS
          pTxCharacteristic->notify();
          }
          //senddata=false;
          digitalWrite(LED, HIGH);
                    }
       // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        //senddata=true;
        digitalWrite(LED, LOW);
        delay(200); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        //Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
   // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
    //END BLE    **************************************************************************
 }

 void blesendloop (){ // Use this only in loops when you want to send data only once
  if (deviceConnected&&senddata) {
          digitalWrite(LED, HIGH);
          lcd.clear();
          lcd.setCursor(3,0);
          lcd.print("Bluetooth");
          lcd.setCursor(2,1);
          lcd.print("Connecting.");
          lcd2.clear();
          lcd2.setCursor(3,0);
          lcd2.print("Bluetooth");
          lcd2.setCursor(2,1);
          lcd2.print("Connecting.");
          delay(1000);
          lcd.setCursor(2,1);
          lcd.print("Connecting..");
          lcd2.setCursor(2,1);
          lcd2.print("Connecting..");
          delay(1000);
          lcd.setCursor(2,1);
          lcd.print("Connecting...");
          lcd2.setCursor(2,1);
          lcd2.print("Connecting..."); 
          delay(1000);
          if (type==1||type==2){
          sprintf(foosball1,"%u,%u,%s,%u",type,state,mphnd,goal);
          pTxCharacteristic->setValue(foosball1); // ONLY SENDS 20 CHARACTERS
          pTxCharacteristic->notify();  
          }
          if (type==3||type==4){
          sprintf(foosball1,"%u,%u,%s,%s,",type,state,mphL,mphR); 
          pTxCharacteristic->setValue(foosball1); // ONLY SENDS 20 CHARACTERS
          pTxCharacteristic->notify();
          sprintf(foosball2,"%i,%i,%u,%u,%u,%u",scoreL,scoreR,timeoutL,timeoutR,matchL,matchR); 
          pTxCharacteristic->setValue(foosball2); // ONLY SENDS 20 CHARACTERS
          pTxCharacteristic->notify();
          }
          if (type==5){
            sprintf(foosball1,"%u,%u,%s,%s,%i",type,state,BTlast_speed,BTfast1,shots); 
          pTxCharacteristic->setValue(foosball1); // ONLY SENDS 20 CHARACTERS
          pTxCharacteristic->notify();
          }
          if (type==6){
            sprintf(foosball1,"%u,%u,%s,%s,",type,state,BTlast_speed,BTfast1); 
          pTxCharacteristic->setValue(foosball1); // ONLY SENDS 20 CHARACTERS
          pTxCharacteristic->notify();
          sprintf(foosball2,"%s,%s,%s",BTfast2,BTfast3,BTfast4); 
          pTxCharacteristic->setValue(foosball2); // ONLY SENDS 20 CHARACTERS
          pTxCharacteristic->notify();
          }
          senddata=false;
          loop();
                    }
       // disconnecting
    if (!deviceConnected){
      senddata=true;
    }
    if (!deviceConnected && oldDeviceConnected) {
        senddata=true;
        digitalWrite(LED, LOW);
        delay(200); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        //Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
      //senddata=true;
   // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
 }

void newgame (){
  buttontimer=0;
  buttons=4095;
  lcd.clear();               //clear LCD
 lcd.setCursor(0,0);            // move cursor to top line
 lcd.print("  New Game?");      //
 lcd.setCursor(0,1);            // move cursor to bottom line
 lcd.print(" YES NO");
 lcd2.clear();               //clear LCD
 lcd2.setCursor(0,0);            // move cursor to top line
 lcd2.print("  New Game?");      //
 lcd2.setCursor(0,1);
 lcd2.print(" YES NO");
 //buttontimer=millis();
 while(buttons>1400&&buttons2>800&&buttontimer<50){
  buttons = analogRead(34);
  buttons2 = analogRead(35);
  buttontimer ++;
 if (buttons<500||buttons2<250){
 if (type==4){
        if (scoreL>scoreR){
          matchL ++;
        }
        if (scoreR>scoreL){
          matchR ++;
        }
        if (type==4&&scoreL==0&&scoreR==0){
          matchL = 0;
          matchR = 0;
        }
      }
      dtostrf(0, 5, 2, BTfast1);
      dtostrf(0, 5, 2, BTfast2);
      dtostrf(0, 5, 2, BTfast3);
      dtostrf(0, 5, 2, BTfast4);
      dtostrf(0, 5, 2, mphL);
      dtostrf(0, 5, 2, mphR);
      dtostrf(0, 5, 2, BTlast_speed);
      fast1=0;
      fast2=0;
      fast3=0;
      fast4=0;
      shots=0;
      last_speed=0;
      shot=1;
      player=1;
      scoreL = 0;
      scoreR = 0;
      timeoutL = 2;
      timeoutR = 2;
      lastL = 0;
      lastR = 0;
      blesend();      
      quality = 1; // 0-Good shot 1-Reset 2-Ball too slow 3-Score adjusted (won't do anything)
      return;
      
 }
 if (buttons>500&&buttons<1400||buttons2>250&&buttons2<800){
  delay(500);
  loop();
 }
 delay(100);
 }
 buttons=0;
 quality=3;
 return;
}

 
 
