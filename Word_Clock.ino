/* 
 * Javelin Word Clock
 * 
 * This clock is built to display spelled out words depending on the time of day.
 * The code depends on both an RTC and Addressable RGB LEDs
 * 
 * RTC Chip used: DS1307 and connect via I2C interface (pins SCL & SDA)
 * RGB LEDs used: WS2812B LEDs on a 5v strip connected to pin 6
 *
 * To set the RTC for the first time you have to send a string consisting of
 * the letter T followed by ten digit time (as seconds since Jan 1 1970) Also known as EPIC time.
 *
 * You can send the text "T1357041600" on the next line using Serial Monitor to set the clock to noon Jan 1 2013  
 * Or you can use the following command via linux terminal to set the clock to the current time (UTC time zone)
 * date +T%s > /dev/ttyACM0
 * Inside the processSyncMessage() function I'm offsetting the UTC time with Central time.
 * If you want the clock to be accurate for your time zone, you may need to update the value.
 */
#include <Adafruit_NeoPixel.h>
#include <Time.h>
#include <Wire.h>  
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t
 
#define RGBLEDPIN    6
#define FWDButtonPIN 8
#define REVButtonPIN 9
#define LED13PIN     13

#define N_LEDS 169 // 13 x 13 grid
#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define BRIGHTNESSDAY 200 // full on
#define BRIGHTNESSNIGHT 55 // half on

Adafruit_NeoPixel grid = Adafruit_NeoPixel(N_LEDS, RGBLEDPIN, NEO_GRB + NEO_KHZ800);

// a few vars to keep the peace;
int intBrightness = BRIGHTNESSDAY; // the brightness of the clock (0 = off and 255 = 100%)
int intTestMode; // set when both buttons are held down
String strTime = ""; // used to detect if word time has changed
int intTimeUpdated = 0; // used to tell if we need to turn on the brightness again when words change

// a few colors
uint32_t colorWhite = grid.Color(255, 255, 255);
uint32_t colorBlack = grid.Color(0, 0, 0);
uint32_t colorRed = grid.Color(255, 0, 0);
uint32_t colorGreen = grid.Color(0, 255, 0);
uint32_t colorBlue = grid.Color(0, 0, 255);
uint32_t colorJGreen = grid.Color(50, 179, 30);

// the words
int arrJAVELIN[] = {164,147,163,162,148,149,161,150,151,152,158,153,157,156,154,155,-1};
int arrTHE1[] = {65,66,67,-1};
int arrAGE[] = {69,70,71,-1};
int arrNYC[] = {72,73,74,-1};
int arrAGENCY[] = {69,70,71,72,73,74,-1};
int arrOF[] = {76,77,-1};
int arrTHE2[] = {40,41,42,-1};
int arrTIME[] = {168,167,166,165,-1};
int arrWE[] = {143,144,-1};
int arrA[] = {177,-1};
int arrAT[] = {145,146,-1};
int arrCUSTOMER[] = {44,45,46,47,48,49,50,51,-1};
int arrHELLO[] = {56,55,54,53,52,-1};
int arrIT[] = {142,141,-1};
int arrIS[] = {139,138,-1};
int arrQUARTER[] = {119,120,121,122,123,124,125,-1};
int arrHALF[] = {133,132,131,130,-1};
int arrPAST[] = {91,92,93,94,-1};
int arrOCLOCK[] = {5,4,3,2,1,0,-1};
int arrTO[] = {95,96,-1};
int arrONE[] = {98,99,100,-1};
int arrTWO[] = {101,102,103,-1};
int arrTHREE[] = {90,89,88,87,86,-1};
int arrFOUR[] = {85,84,83,82,-1};
int arrMFIVE[] = {109,108,107,106,-1};
int arrFIVE[] = {81,80,79,78,-1};
int arrSIX[] = {38,37,36,-1};
int arrSEVEN[] = {35,34,33,32,31,-1};
int arrEIGHT[] = {30,29,28,27,26,-1};
int arrNINE[] = {13,14,15,16,-1};
int arrTEN[] = {17,18,19,-1};
int arrMTEN[] = {136,135,134,-1};
int arrELEVEN[] = {20,21,22,23,24,25,-1};
int arrTWELVE[] = {12,11,10,9,8,7,-1};
int arrTWENTY[] = {116,115,114,113,112,111,-1};

void setup() {
  // set up the debuging serial output
  Serial.begin(9600);
//  while(!Serial); // Needed for Leonardo only
  delay(200);
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  setSyncInterval(60); // sync the time every 60 seconds (1 minutes)
  if(timeStatus() != timeSet){
     Serial.println("Unable to sync with the RTC");
     RTC.set(1406278800);   // set the RTC to Jul 25 2014 9:00 am
     setTime(1406278800);
  }else{
     Serial.println("RTC has set the system time");
  }
  // print the version of code to the console
  printVersion();
  
  // setup the LED strip  
  grid.begin();
  grid.show();

  // set the bright ness of the strip
  grid.setBrightness(intBrightness);

  colorWipe(colorBlack, 0);
  spellWord(arrHELLO, colorJGreen);
  delay(1000);
  spellWord(arrHELLO, colorWhite);
  delay(1000);
  fadeOut(10);
  colorWipe(colorBlack, 0);

//  paintWord(arrJAVELIN, colorJGreen);
//  fadeIn(50);
  // set the bright ness of the strip
  grid.setBrightness(intBrightness);

  // flash our cool tag line
  THEAGE();
  THECUSTOMER();
  fadeOut(5);
  delay(50);
  colorWipe(colorBlack, 0);
  grid.setBrightness(intBrightness);
  
  // initialize the onboard led on pin 13
  pinMode(LED13PIN, OUTPUT);

  // initialize the buttons
  pinMode(FWDButtonPIN, INPUT);
  pinMode(REVButtonPIN, INPUT);
  
  // lets kick off the clock
  digitalClockDisplay();
}

void loop(){
  // if there is a serial connection lets see if we need to set the time
  if (Serial.available()) {
    time_t t = processSyncMessage();
    if (t != 0) {
      Serial.print("Time set via connection to: ");
      Serial.print(t);
      Serial.println();
      RTC.set(t);   // set the RTC and the system time to the received value
      setTime(t);          
    }
  }
  // check to see if the time has been set
  if (timeStatus() == timeSet){
    // time is set lets show the time
    if((hour() < 7) | (hour() >= 19)){
      intBrightness =  BRIGHTNESSNIGHT;
    }else{
      intBrightness =  BRIGHTNESSDAY;
    }
    grid.setBrightness(intBrightness);
    
    // test to see if both buttons are being held down
    // if so  - start a self test till both buttons are held down again.
    if((digitalRead(FWDButtonPIN) == LOW) && (digitalRead(REVButtonPIN) == LOW)){
      intTestMode = !intTestMode;
      if(intTestMode){
        Serial.println("Selftest Mode TRUE");
        // run through a quick test
        test_grid();
      }else{
        Serial.println("Selftest mode FALSE");
      }
    }
    // test to see if a forward button is being held down for time setting
    if(digitalRead(FWDButtonPIN) == LOW){
      digitalWrite(LED13PIN, HIGH);
      Serial.println("Forward Button Down");
      incrementTime(300);
      delay(100);
      digitalWrite(LED13PIN, LOW);
    }
  
    // test to see if the back button is being held down for time setting
    if(digitalRead(REVButtonPIN) == LOW){
      digitalWrite(LED13PIN, HIGH);
      Serial.println("Backwards Button Down");
      incrementTime(-300);
      delay(100);
      digitalWrite(LED13PIN, LOW);
    }

    // and finaly we display the time (provided we are not in self tes mode
    if(!intTestMode){
      displayTime();
    }
  }else{
    colorWipe(colorBlack, 0);
    paintWord(arrONE, colorRed);
    Serial.println("The time has not been set.  Please run the Time");
    Serial.println("TimeRTCSet example, or DS1307RTC SetTime example.");
    Serial.println();
    delay(4000);
  }
  delay(1000);
}

void incrementTime(int intSeconds){
  // increment the time counters keeping care to rollover as required
  if(timeStatus() == timeSet){
    Serial.print("adding ");
    Serial.print(intSeconds);
    Serial.println(" seconds to RTC");
//    colorWipe(colorBlack, 0);
    adjustTime(intSeconds);
    RTC.set(now() + intSeconds);
    digitalClockDisplay();
    displayTime();
  }
}  

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.print("-");
  Serial.print(month());
  Serial.print("-");
  Serial.print(day());
  Serial.println();
}

void displayTime(){
  String strCurrentTime; // build the current time
  //colorWipe(colorBlack, 0);
  paintWord(arrJAVELIN, colorJGreen);
  // Now, turn on the "It is" leds
  paintWord(arrIT, colorWhite);
  // if the time IS one of the following IS is green
  if((minute()==5)
    |(minute()==10)
    |(minute()==15)
    |(minute()==20)
    |(minute()==25)
    |(minute()==30)
    |(minute()==35)
    |(minute()==40)
    |(minute()==45)
    |(minute()==50)
    |(minute()==55)){
    paintWord(arrIS, colorJGreen);
  }else{
    paintWord(arrIS, colorWhite);
  }
  // now we display the appropriate minute counter
  if((minute()>4) && (minute()<10)){
    // FIVE MINUTES
    strCurrentTime = "five ";
    paintWord(arrMFIVE, colorWhite);
    paintWord(arrMTEN, colorBlack);
    paintWord(arrA, colorBlack);
    paintWord(arrQUARTER, colorBlack);
    paintWord(arrHALF, colorBlack);
    paintWord(arrTWENTY, colorBlack);
  } 
  if((minute()>9) && (minute()<15)) { 
    //TEN MINUTES;
    strCurrentTime = "ten ";
    paintWord(arrMFIVE, colorBlack);
    paintWord(arrMTEN, colorWhite);
    paintWord(arrA, colorBlack);
    paintWord(arrQUARTER, colorBlack);
    paintWord(arrHALF, colorBlack);
    paintWord(arrTWENTY, colorBlack);
  }
  if((minute()>14) && (minute()<20)) {
    // QUARTER
    strCurrentTime = "a quarter ";
    paintWord(arrMFIVE, colorBlack);
    paintWord(arrMTEN, colorBlack);
    paintWord(arrA, colorWhite);
    paintWord(arrQUARTER, colorWhite);
    paintWord(arrHALF, colorBlack);
    paintWord(arrTWENTY, colorBlack);
  }
  if((minute()>19) && (minute()<25)) { 
    //TWENTY MINUTES
    strCurrentTime = "twenty ";
    paintWord(arrMFIVE, colorBlack);
    paintWord(arrMTEN, colorBlack);
    paintWord(arrA, colorBlack);
    paintWord(arrQUARTER, colorBlack);
    paintWord(arrHALF, colorBlack);
    paintWord(arrTWENTY, colorWhite);
  }
  if((minute()>24) && (minute()<30)) { 
    //TWENTY FIVE
    strCurrentTime = "twenty five ";
    paintWord(arrMFIVE, colorWhite);
    paintWord(arrMTEN, colorBlack);
    paintWord(arrA, colorBlack);
    paintWord(arrQUARTER, colorBlack);
    paintWord(arrHALF, colorBlack);
    paintWord(arrTWENTY, colorWhite);
  }  
  if((minute()>29) && (minute()<35)) {
    strCurrentTime = "half ";
    paintWord(arrMFIVE, colorBlack);
    paintWord(arrMTEN, colorBlack);
    paintWord(arrA, colorBlack);
    paintWord(arrQUARTER, colorBlack);
    paintWord(arrHALF, colorWhite);
    paintWord(arrTWENTY, colorBlack);
  }
  if((minute()>34) && (minute()<40)) { 
    //TWENTY FIVE
    strCurrentTime = "twenty five ";
    paintWord(arrMFIVE, colorWhite);
    paintWord(arrMTEN, colorBlack);
    paintWord(arrA, colorBlack);
    paintWord(arrQUARTER, colorBlack);
    paintWord(arrHALF, colorBlack);
    paintWord(arrTWENTY, colorWhite);

  }  
  if((minute()>39) && (minute()<45)) {
    strCurrentTime = "twenty ";
    paintWord(arrMFIVE, colorBlack);
    paintWord(arrMTEN, colorBlack);
    paintWord(arrA, colorBlack);
    paintWord(arrQUARTER, colorBlack);
    paintWord(arrHALF, colorBlack);
    paintWord(arrTWENTY, colorWhite);
  }
  if((minute()>44) && (minute()<50)) {
    strCurrentTime = "a quarter ";
    paintWord(arrMFIVE, colorBlack);
    paintWord(arrMTEN, colorBlack);
    paintWord(arrA, colorWhite);
    paintWord(arrQUARTER, colorWhite);
    paintWord(arrHALF, colorBlack);
    paintWord(arrTWENTY, colorBlack);
  }
  if((minute()>49) && (minute()<55)){
    strCurrentTime = "ten ";
    paintWord(arrMFIVE, colorBlack);
    paintWord(arrMTEN, colorWhite);
    paintWord(arrA, colorBlack);
    paintWord(arrQUARTER, colorBlack);
    paintWord(arrHALF, colorBlack);
    paintWord(arrTWENTY, colorBlack); 
  } 
  if(minute()>54){
    strCurrentTime = "five ";
    paintWord(arrMFIVE, colorWhite);
    paintWord(arrMTEN, colorBlack);
    paintWord(arrA, colorBlack);
    paintWord(arrQUARTER, colorBlack);
    paintWord(arrHALF, colorBlack);
    paintWord(arrTWENTY, colorBlack);
  }
  if((minute() <5)){
    switch(hour()){
      case 1:
      case 13:
      strCurrentTime = strCurrentTime + "one ";
        paintWord(arrONE, colorWhite);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
      break;
    case 2:
    case 14:
      strCurrentTime = strCurrentTime + "two ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorWhite);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
      break;
    case 3: 
    case 15:
      strCurrentTime = strCurrentTime + "three ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorWhite);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
      break;
    case 4: 
    case 16:
      strCurrentTime = strCurrentTime + "four ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorWhite);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
      break;
    case 5: 
    case 17:
      strCurrentTime = strCurrentTime + "five ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorWhite);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
      break;
    case 6: 
    case 18:
      strCurrentTime = strCurrentTime + "six ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorWhite);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
      break;
    case 7: 
    case 19:
      strCurrentTime = strCurrentTime + "seven ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorWhite);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
      break;
    case 8: 
    case 20:
      strCurrentTime = strCurrentTime + "eight ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorWhite);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
      break;
    case 9: 
    case 21:
      strCurrentTime = strCurrentTime + "nine ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorWhite);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
      break;
    case 10:
    case 22:
      strCurrentTime = strCurrentTime + "ten ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorWhite);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
      break;
    case 11:
    case 23:
      strCurrentTime = strCurrentTime + "eleven ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorWhite);
        paintWord(arrTWELVE, colorBlack);
      break;
    case 0:
    case 12: 
    case 24:
      strCurrentTime = strCurrentTime + "twelve ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorWhite);
      break;
    }
    strCurrentTime = strCurrentTime + "oclock ";
    paintWord(arrPAST, colorBlack);
    paintWord(arrOCLOCK, colorWhite);
    paintWord(arrTO, colorBlack);
  }else if((minute() < 35) && (minute() >4)){
    strCurrentTime = strCurrentTime + "past ";
    paintWord(arrPAST, colorWhite);
    paintWord(arrOCLOCK, colorBlack);
    paintWord(arrTO, colorBlack);
    switch (hour()) {
      case 1:
      case 13:
        strCurrentTime = strCurrentTime + "one ";
        paintWord(arrONE, colorWhite);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 2: 
      case 14:
        strCurrentTime = strCurrentTime + "two ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorWhite);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 3: 
      case 15:
        strCurrentTime = strCurrentTime + "three ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorWhite);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 4: 
      case 16:
        strCurrentTime = strCurrentTime + "four ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorWhite);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 5: 
      case 17:
        strCurrentTime = strCurrentTime + "five ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorWhite);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 6: 
      case 18:
        strCurrentTime = strCurrentTime + "six ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorWhite);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 7: 
      case 19:
        strCurrentTime = strCurrentTime + "seven ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorWhite);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 8: 
      case 20:
        strCurrentTime = strCurrentTime + "eight ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorWhite);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 9: 
      case 21:
        strCurrentTime = strCurrentTime + "nine ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorWhite);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 10:
      case 22:
        strCurrentTime = strCurrentTime + "ten ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorWhite);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 11:
      case 23:
        strCurrentTime = strCurrentTime + "eleven ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorWhite);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 0:
      case 12:
      case 24:
        strCurrentTime = strCurrentTime + "twelve ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorWhite);
        break;
      }
    }else{
      // if we are greater than 34 minutes past the hour then display
      // the next hour, as we will be displaying a 'to' sign
      strCurrentTime = strCurrentTime + "to ";
      paintWord(arrPAST, colorBlack);
      paintWord(arrOCLOCK, colorBlack);
      paintWord(arrTO, colorWhite);
      switch (hour()) {
        case 1: 
        case 13:
        strCurrentTime = strCurrentTime + "two ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorWhite);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 14:
      case 2:
        strCurrentTime = strCurrentTime + "three ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorWhite);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 15:
      case 3:
        strCurrentTime = strCurrentTime + "four ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorWhite);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 4: 
      case 16:
        strCurrentTime = strCurrentTime + "five ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorWhite);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 5: 
      case 17:
        strCurrentTime = strCurrentTime + "six ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorWhite);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 6: 
      case 18:
        strCurrentTime = strCurrentTime + "seven ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorWhite);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 7: 
      case 19:
        strCurrentTime = strCurrentTime + "eight ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorWhite);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 8: 
      case 20:
        strCurrentTime = strCurrentTime + "nine ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorWhite);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack); 
        break;
      case 9: 
      case 21:
        strCurrentTime = strCurrentTime + "ten ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorWhite);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack); 
        break;
      case 10: 
      case 22:
        strCurrentTime = strCurrentTime + "eleven ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorWhite);
        paintWord(arrTWELVE, colorBlack);
        break;
      case 11: 
      case 23:
        strCurrentTime = strCurrentTime + "twelve ";
        paintWord(arrONE, colorBlack);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorWhite);
        break;
      case 0:
      case 12: 
      case 24:
        strCurrentTime = strCurrentTime + "one ";
        paintWord(arrONE, colorWhite);
        paintWord(arrTWO, colorBlack);
        paintWord(arrTHREE, colorBlack);
        paintWord(arrFOUR, colorBlack);
        paintWord(arrFIVE, colorBlack);
        paintWord(arrSIX, colorBlack);
        paintWord(arrSEVEN, colorBlack);
        paintWord(arrEIGHT, colorBlack);
        paintWord(arrNINE, colorBlack);
        paintWord(arrTEN, colorBlack);
        paintWord(arrELEVEN, colorBlack);
        paintWord(arrTWELVE, colorBlack);
        break;
    }
  }

  if(strCurrentTime != strTime){
    digitalClockDisplay();
    strTime = strCurrentTime;
    if(strTime == ""){
      fadeIn(20);
    }
    // display our tagline
    if((minute()==0) | (minute()==30)){
      fadeOut(20);
      colorWipe(colorBlack, 0);
      grid.setBrightness(intBrightness);
      THEAGE();
      THECUSTOMER();
      fadeOut(20);
      colorWipe(colorBlack, 10);
      grid.setBrightness(intBrightness);
      // print the version of code to the console
      printVersion();
    }
  }else{
//    grid.show();
  }
}

void THEAGE(){
//  paintWord(arrJAVELIN, colorBlack);
  paintWord(arrTHE1, colorWhite);
  delay(1000);
  paintWord(arrAGE, colorWhite);
  delay(1000);
  paintWord(arrOF, colorWhite);
  delay(1000);
  paintWord(arrTHE2, colorWhite);
  delay(1000);
  paintWord(arrCUSTOMER, colorWhite);
  delay(1000);
}

void THECUSTOMER(){
  paintWord(arrJAVELIN, colorJGreen);
  delay(1000);
  paintWord(arrIS, colorWhite);
  delay(1000);
  paintWord(arrTHE1, colorJGreen);
  delay(1000);
  paintWord(arrAGENCY, colorJGreen);
  delay(1000);
  paintWord(arrOF, colorJGreen);
  delay(1000);
  paintWord(arrTHE2, colorJGreen);
  delay(1000);
  paintWord(arrCUSTOMER, colorJGreen);
  delay(1000);
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void rainbow(uint8_t wait) {
  //secret rainbow mode
  uint16_t i, j;
 
  for(j=0; j<256; j++) {
    for(i=0; i<grid.numPixels(); i++) {
      grid.setPixelColor(i, Wheel((i+j) & 255));
    }
    grid.show();
    delay(wait);
  }
}

static void chase(uint32_t color, uint8_t wait) {
  for(uint16_t i=0; i<grid.numPixels()+4; i++) {
      grid.setPixelColor(i  , color); // Draw new pixel
      grid.setPixelColor(i-4, 0); // Erase pixel a few steps back
      grid.show();
      delay(wait);
  }
}

void fadeOut(int time){
  for (int i = intBrightness; i > 0; --i){
    grid.setBrightness(i);
    grid.show();
    delay(time);
  }
}

void fadeIn(int time){
  for(int i = 1; i < intBrightness; ++i){
    grid.setBrightness(i);
    grid.show();
    delay(time);  
  }
}

// Fill the dots one after the other with a color
void colorWipe(uint32_t color, uint8_t wait) {
  for(uint16_t i=0; i<grid.numPixels(); i++) {
      grid.setPixelColor(i, color);
  }
  grid.show();
  delay(wait);
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  if(WheelPos < 85) {
   return grid.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if(WheelPos < 170) {
   WheelPos -= 85;
   return grid.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
   WheelPos -= 170;
   return grid.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

void paintWord(int arrWord[], uint32_t intColor){
  for(int i = 0; i < grid.numPixels() + 1; i++){
    if(arrWord[i] == -1){
      grid.show();
      break;
    }else{
      grid.setPixelColor(arrWord[i],intColor);
    }
  }
}

void spellWord(int arrWord[], uint32_t intColor){
  for(int i = 0; i < grid.numPixels() + 1; i++){
    if(arrWord[i] == -1){
      break;
    }else{
      grid.setPixelColor(arrWord[i],intColor);
      grid.show();
      delay(500);
    }
  }
}

// print out the software version number
void printVersion(void) {
  delay(2000);
  Serial.println();
  Serial.println("Javelin WordClock - Arduino v1.0a - reduced brightnes version");
  Serial.println("(c)2014 Richard Silva");
  Serial.println();
}

unsigned long processSyncMessage() {
  unsigned long pctime = 0L;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013 

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     pctime = pctime - 18000;
     return pctime;
     if( pctime < DEFAULT_TIME) { // check the value is a valid time (greater than Jan 1 2013)
       pctime = 0L; // return 0 to indicate that the time is not valid
     }
     Serial.println();
     Serial.println("Time Set via Serial");
     Serial.println();
  }
  return pctime;
}

// runs throught the various displays, testing
void test_grid(){
  printVersion();
  colorWipe(colorBlack, 0);
  spellWord(arrHELLO, colorJGreen);
  delay(1000);
  fadeOut(50);
  grid.setBrightness(intBrightness);
  paintWord(arrHELLO, colorWhite);
  delay(1000);
  fadeOut(50);
  grid.setBrightness(intBrightness);
  colorWipe(colorJGreen, 50);
  chase(colorRed,2); // Red
  chase(colorGreen,2); // Green
  chase(colorBlue,2); // Blue
  chase(colorWhite,2); // White
  colorWipe(colorBlack, 0);
  paintWord(arrTIME, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrWE, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrAT, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrIT, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrIS, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrHALF, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrA, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrQUARTER, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrPAST, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrOCLOCK, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrTO, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrONE, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrTWO, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrTHREE, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrFOUR, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrMFIVE, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrFIVE, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrSIX, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrSEVEN, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrEIGHT, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrNINE, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrMTEN, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrTEN, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrELEVEN, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrTWELVE, colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  paintWord(arrTWENTY, colorWhite);
  grid.show();
  delay(1000);

  colorWipe(colorBlack, 0);
  paintWord(arrJAVELIN, colorJGreen);
  grid.show();
  delay(2000);
  paintWord(arrIS, colorWhite);
  grid.show();
  delay(1000);
  paintWord(arrTHE1, colorWhite);
  grid.show();
  delay(1000);
  paintWord(arrAGENCY, colorWhite);
  grid.show();
  delay(1000);
  paintWord(arrOF, colorWhite);
  grid.show();
  delay(1000);
  paintWord(arrTHE2, colorWhite);
  grid.show();
  delay(1000);
  paintWord(arrCUSTOMER, colorWhite);
  grid.show();
  delay(1000);
  fadeOut(100);
  colorWipe(colorBlack, 0);
  grid.setBrightness(intBrightness);
  intTestMode = !intTestMode;
}
