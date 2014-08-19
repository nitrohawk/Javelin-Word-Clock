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
#define BRIGHTNESSDAY 155 // full on
#define BRIGHTNESSNIGHT 55 // half on

Adafruit_NeoPixel grid = Adafruit_NeoPixel(N_LEDS, RGBLEDPIN, NEO_GRB + NEO_KHZ800);

int intBrightness = 155; // the brightness of the clock (0 = off and 255 = 100%)
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

void setup() {
  // set up the debuging serial output
  Serial.begin(9600);
//  while(!Serial); // Needed for Leonardo only
  delay(200);
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
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
  intBrightness = BRIGHTNESSDAY;
  grid.setBrightness(intBrightness);

  chase(colorBlack, 0);
  HELLO_SPELL(colorJGreen);
  HELLO_SPELL(colorWhite);
  delay(1000);
  fadeOut(10);
  chase(colorBlack, 0);
//  colorWipe(colorBlack, 0);
  JAVELIN(colorJGreen);
  fadeIn(10);

  // flash our cool tag line
  THEAGE();
  THECUSTOMER();
  fadeOut(10);
  delay(100);
  chase(colorBlack, 0);
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
      digitalClockDisplay();
    }
  }else{
    colorWipe(colorBlack, 0);
    ONE(colorRed);
    Serial.println("The time has not been set.  Please run the Time");
    Serial.println("TimeRTCSet example, or DS1307RTC SetTime example.");
    Serial.println();
    delay(4000);
  }
  delay(100);
}

void incrementTime(int intSeconds){
  // increment the time counters keeping care to rollover as required
  if(timeStatus() == timeSet){
    Serial.print("adding ");
    Serial.print(intSeconds);
    Serial.println(" seconds to RTC");
    colorWipe(colorBlack, 0);
    adjustTime(intSeconds);
  }
}  

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.println();
  displayTime();
}


void displayTime(){
  String strCurrentTime; // build the current time
  if(intTimeUpdated == 1){
    intTimeUpdated = 0; // reset the value
//    grid.setBrightness(intBrightness);
    fadeIn(10);
//    grid.show();
  }else{
    grid.show();
  }
  //colorWipe(colorBlack, 0);
  JAVELIN(colorJGreen);
  // Now, turn on the "It is" leds
  IT(colorWhite);
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
    IS(colorJGreen);
  }else{
    IS(colorWhite);    
  }
  // now we display the appropriate minute counter
  if((minute()>4) && (minute()<10)){
    // FIVE MINUTES
    strCurrentTime = "five ";
    MFIVE(colorWhite);  
  } 
  if((minute()>9) && (minute()<15)) { 
    //TEN MINUTES;
    strCurrentTime = "ten ";
    MTEN(colorWhite); 
  }
  if((minute()>14) && (minute()<20)) {
    // QUARTER
    strCurrentTime = "a quarter ";
    A(colorWhite);
    QUARTER(colorWhite); 
  }
  if((minute()>19) && (minute()<25)) { 
    //TWENTY MINUTES
    strCurrentTime = "twenty ";
    TWENTY(colorWhite); 
  }
  if((minute()>24) && (minute()<30)) { 
    //TWENTY FIVE
    strCurrentTime = "twenty five ";
    TWENTY(colorWhite);
    MFIVE(colorWhite);
  }  
  if((minute()>29) && (minute()<35)) {
    strCurrentTime = "half ";
    HALF(colorWhite);
  }
  if((minute()>34) && (minute()<40)) { 
    //TWENTY FIVE
    strCurrentTime = "twenty five ";
    TWENTY(colorWhite);
    MFIVE(colorWhite);
  }  
  if((minute()>39) && (minute()<45)) {
    strCurrentTime = "twenty ";
    TWENTY(colorWhite);
  }
  if((minute()>44) && (minute()<50)) {
    strCurrentTime = "a quarter ";
    A(colorWhite);
    QUARTER(colorWhite); 
  }
  if((minute()>49) && (minute()<55)){
    strCurrentTime = "ten ";
    MTEN(colorWhite); 
  } 
  if(minute()>54){
    strCurrentTime = "five ";
    MFIVE(colorWhite);
  }
  if((minute() <5)){
    switch(hour()){
      case 1:
      case 13:
      strCurrentTime = strCurrentTime + "one ";
      ONE(colorWhite);
      break;
    case 2:
    case 14:
      strCurrentTime = strCurrentTime + "two ";
      TWO(colorWhite); 
      break;
    case 3: 
    case 15:
      strCurrentTime = strCurrentTime + "three ";
      THREE(colorWhite);
      break;
    case 4: 
    case 16:
      strCurrentTime = strCurrentTime + "four ";
      FOUR(colorWhite);
      break;
    case 5: 
    case 17:
      strCurrentTime = strCurrentTime + "five ";
      FIVE(colorWhite);
      break;
    case 6: 
    case 18:
      strCurrentTime = strCurrentTime + "six ";
      SIX(colorWhite);
      break;
    case 7: 
    case 19:
      strCurrentTime = strCurrentTime + "seven ";
      SEVEN(colorWhite);
      break;
    case 8: 
    case 20:
      strCurrentTime = strCurrentTime + "eight ";
      EIGHT(colorWhite);
      break;
    case 9: 
    case 21:
      strCurrentTime = strCurrentTime + "nine ";
      NINE(colorWhite);
      break;
    case 10:
    case 22:
      strCurrentTime = strCurrentTime + "ten ";
      TEN(colorWhite);
      break;
    case 11:
    case 23:
      strCurrentTime = strCurrentTime + "eleven ";
      ELEVEN(colorWhite);
      break;
    case 0:
    case 12: 
    case 24:
      strCurrentTime = strCurrentTime + "twelve ";
      TWELVE(colorWhite);
      break;
    }
    strCurrentTime = strCurrentTime + "oclock ";
    OCLOCK(colorWhite);
  }else if((minute() < 35) && (minute() >4)){
    strCurrentTime = strCurrentTime + "past ";
    PAST(colorWhite);
    switch (hour()) {
      case 1:
      case 13:
        strCurrentTime = strCurrentTime + "one ";
        ONE(colorWhite);
        break;
      case 2: 
      case 14:
        strCurrentTime = strCurrentTime + "two ";
        TWO(colorWhite); 
        break;
      case 3: 
      case 15:
        strCurrentTime = strCurrentTime + "three ";
        THREE(colorWhite); 
        break;
      case 4: 
      case 16:
        strCurrentTime = strCurrentTime + "four ";
        FOUR(colorWhite); 
        break;
      case 5: 
      case 17:
        strCurrentTime = strCurrentTime + "five ";
        FIVE(colorWhite); 
        break;
      case 6: 
      case 18:
        strCurrentTime = strCurrentTime + "six ";
        SIX(colorWhite); 
        break;
      case 7: 
      case 19:
        strCurrentTime = strCurrentTime + "seven ";
        SEVEN(colorWhite); 
        break;
      case 8: 
      case 20:
        strCurrentTime = strCurrentTime + "eight ";
        EIGHT(colorWhite); 
        break;
      case 9: 
      case 21:
        strCurrentTime = strCurrentTime + "nine ";
        NINE(colorWhite); 
        break;
      case 10:
      case 22:
        strCurrentTime = strCurrentTime + "ten ";
        TEN(colorWhite); 
        break;
      case 11:
      case 23:
        strCurrentTime = strCurrentTime + "eleven ";
        ELEVEN(colorWhite); 
        break;
      case 0:
      case 12:
      case 24:
        strCurrentTime = strCurrentTime + "twelve ";
        TWELVE(colorWhite); 
        break;
      }
    }else{
      // if we are greater than 34 minutes past the hour then display
      // the next hour, as we will be displaying a 'to' sign
      strCurrentTime = strCurrentTime + "to ";
      TO(colorWhite);
      switch (hour()) {
        case 1: 
        case 13:
        strCurrentTime = strCurrentTime + "two ";
        TWO(colorWhite); 
        break;
      case 14:
      case 2:
        strCurrentTime = strCurrentTime + "three ";
        THREE(colorWhite); 
        break;
      case 15:
      case 3:
        strCurrentTime = strCurrentTime + "four ";
        FOUR(colorWhite); 
        break;
      case 4: 
      case 16:
        strCurrentTime = strCurrentTime + "five ";
        FIVE(colorWhite); 
        break;
      case 5: 
      case 17:
        strCurrentTime = strCurrentTime + "six ";
        SIX(colorWhite); 
        break;
      case 6: 
      case 18:
        strCurrentTime = strCurrentTime + "seven ";
        SEVEN(colorWhite); 
        break;
      case 7: 
      case 19:
        strCurrentTime = strCurrentTime + "eight ";
        EIGHT(colorWhite); 
        break;
      case 8: 
      case 20:
        strCurrentTime = strCurrentTime + "nine ";
        NINE(colorWhite); 
        break;
      case 9: 
      case 21:
        strCurrentTime = strCurrentTime + "ten ";
        TEN(colorWhite); 
        break;
      case 10: 
      case 22:
        strCurrentTime = strCurrentTime + "eleven ";
        ELEVEN(colorWhite); 
        break;
      case 11: 
      case 23:
        strCurrentTime = strCurrentTime + "twelve ";
        TWELVE(colorWhite); 
        break;
      case 0:
      case 12: 
      case 24:
        strCurrentTime = strCurrentTime + "one ";
        ONE(colorWhite); 
        break;
    }
  }
  if(strCurrentTime != strTime){
    // display our tagline
    if((minute()==0) | (minute()==30)){
//      fadeOut(10);
      colorWipe(colorBlack, 10);
      grid.setBrightness(intBrightness);
      THEAGE();
      THECUSTOMER();
      fadeOut(10);
      colorWipe(colorBlack, 10);
    //grid.setBrightness(intBrightness);
    }
    strTime = strCurrentTime;
//    fadeOut(100); // wont work because it calls show() which will show both current and new time at the same... time ha!
    colorWipe(colorBlack, 10);
  }else{
//    grid.show();
  }
}

void JAVELIN(uint32_t color){
  Serial.println("JAVELIN ");
  grid.setPixelColor(164,color); // J (top)
  grid.setPixelColor(147,color); // J (bottom)
  grid.setPixelColor(163,color); // A (top left)
  grid.setPixelColor(162,color); // A (top right
  grid.setPixelColor(148,color); // A (bottom left)
  grid.setPixelColor(149,color); // A (bottom right)
  grid.setPixelColor(161,color); // V (top)
  grid.setPixelColor(150,color); // V (bottom)
  grid.setPixelColor(160,color); // E (top left)
  grid.setPixelColor(159,color); // E (top right)
  grid.setPixelColor(151,color); // E (bottom left)
  grid.setPixelColor(152,color); // E (bottom right)
  grid.setPixelColor(158,color); // L (top)
  grid.setPixelColor(153,color); // L (bottom)
  // the I is part of the N
  grid.setPixelColor(157,color); // N (top left)
  grid.setPixelColor(156,color); // N (top right)
  grid.setPixelColor(154,color); // N (bottom left)
  grid.setPixelColor(155,color); // N (bottom right)
}

void THE1(uint32_t color){
  Serial.println("the ");
  grid.setPixelColor(65,color); // T
  grid.setPixelColor(66,color); // H
  grid.setPixelColor(67,color); // E
}
void AGE(uint32_t color){
  Serial.println("age ");
  grid.setPixelColor(69,color); // A
  grid.setPixelColor(70,color); // G
  grid.setPixelColor(71,color); // E
}
void NCY(uint32_t color){
  Serial.println("ncy ");
  grid.setPixelColor(72,color); // N
  grid.setPixelColor(73,color); // C
  grid.setPixelColor(74,color); // Y
}
void AGENCY(uint32_t color){
  Serial.println("agency ");
  grid.setPixelColor(69,color); // A
  grid.setPixelColor(70,color); // G
  grid.setPixelColor(71,color); // E
  grid.setPixelColor(72,color); // N
  grid.setPixelColor(73,color); // C
  grid.setPixelColor(74,color); // Y
}
void OF(uint32_t color){
  Serial.println("of ");
  grid.setPixelColor(76,color); // O
  grid.setPixelColor(77,color); // F
}

void THE2(uint32_t color){
  Serial.println("the ");
  grid.setPixelColor(40,color); // T
  grid.setPixelColor(41,color); // H
  grid.setPixelColor(42,color); // E
}

void TIME(uint32_t color){
  Serial.println("time ");
  grid.setPixelColor(168,color); // T
  grid.setPixelColor(167,color); // I
  grid.setPixelColor(166,color); // M
  grid.setPixelColor(165,color); // E
}

void WE(uint32_t color){
  Serial.println("we ");
  grid.setPixelColor(143,color); // W
  grid.setPixelColor(144,color); // E
}
void A(uint32_t color){
  Serial.println("a ");
  grid.setPixelColor(117,color); // W
}
void AT(uint32_t color){
  Serial.println("at ");
  grid.setPixelColor(145,color); // A
  grid.setPixelColor(146,color); // T
}

void CUSTOMER(uint32_t color){
 Serial.println("customer ");
  grid.setPixelColor(44,color); // C
  grid.setPixelColor(45,color); // U
  grid.setPixelColor(46,color); // S
  grid.setPixelColor(47,color); // T
  grid.setPixelColor(48,color); // O
  grid.setPixelColor(49,color); // M
  grid.setPixelColor(50,color); // U
  grid.setPixelColor(51,color); // R
}

void HELLO_SPELL(uint32_t color){
  Serial.println("HELLO ");
  grid.setPixelColor(56,color); // H
  grid.show();
  delay(500);
  grid.setPixelColor(55,color); // E
  grid.show();
  delay(500);
  grid.setPixelColor(54,color); // L
  grid.show();
  delay(500);
  grid.setPixelColor(53,color); // L
  grid.show();
  delay(500);
  grid.setPixelColor(52,color); // O
  grid.show();
  delay(700);
}
void HELLO(uint32_t color){
  Serial.println("HELLO ");
  grid.setPixelColor(56,color); // H
  grid.setPixelColor(55,color); // E
  grid.setPixelColor(54,color); // L
  grid.setPixelColor(53,color); // L
  grid.setPixelColor(52,color); // O
}
void IT(uint32_t color){
  Serial.println("it");
  grid.setPixelColor(142,color); // I
  grid.setPixelColor(141,color); // T
}
void IS(uint32_t color){
  Serial.println("is ");
  grid.setPixelColor(139,color); // I
  grid.setPixelColor(138,color); // S
}
void QUARTER(uint32_t color){
  Serial.println("quarter ");
  grid.setPixelColor(119,color); // Q
  grid.setPixelColor(120,color); // U
  grid.setPixelColor(121,color); // A
  grid.setPixelColor(122,color); // R
  grid.setPixelColor(123,color); // T
  grid.setPixelColor(124,color); // E
  grid.setPixelColor(125,color); // R
}
void HALF(uint32_t color){
  Serial.println("half ");
  grid.setPixelColor(133,color); // H
  grid.setPixelColor(132,color); // A
  grid.setPixelColor(131,color); // L
  grid.setPixelColor(130,color); // F
}
void PAST(uint32_t color){
  Serial.println("past ");
  grid.setPixelColor(91,color); // P
  grid.setPixelColor(92,color); // A
  grid.setPixelColor(93,color); // S
  grid.setPixelColor(94,color); // T
}
void OCLOCK(uint32_t color){
  Serial.println("oclock ");
  grid.setPixelColor(5,color); // O
  grid.setPixelColor(4,color); // C
  grid.setPixelColor(3,color); // L
  grid.setPixelColor(2,color); // O
  grid.setPixelColor(1,color); // C
  grid.setPixelColor(0,color); // K
}
void TO(uint32_t color){
  Serial.println("to ");
  grid.setPixelColor(95,color); // T
  grid.setPixelColor(96,color); // O
}
void ONE(uint32_t color){
  Serial.println("one ");
  grid.setPixelColor(98,color); // O
  grid.setPixelColor(99,color); // N
  grid.setPixelColor(100,color); // E
}
void TWO(uint32_t color){
  Serial.println("two ");
  grid.setPixelColor(101,color); // T
  grid.setPixelColor(102,color); // W
  grid.setPixelColor(103,color); // O
}
void THREE(uint32_t color){
  Serial.println("three ");
  grid.setPixelColor(90,color); // T
  grid.setPixelColor(89,color); // H
  grid.setPixelColor(88,color); // R
  grid.setPixelColor(87,color); // E
  grid.setPixelColor(86,color); // E
}
void FOUR(uint32_t color){
  Serial.println("four ");
  grid.setPixelColor(85,color); // F
  grid.setPixelColor(84,color); // O
  grid.setPixelColor(83,color); // U
  grid.setPixelColor(82,color); // R
}
void MFIVE(uint32_t color){
  Serial.println("five ");
  grid.setPixelColor(109,color); // F
  grid.setPixelColor(108,color); // I
  grid.setPixelColor(107,color); // V
  grid.setPixelColor(106,color); // E
}
void FIVE(uint32_t color){
  Serial.println("five ");
  grid.setPixelColor(81,color); // F
  grid.setPixelColor(80,color); // I
  grid.setPixelColor(79,color); // V
  grid.setPixelColor(78,color); // E
}
void SIX(uint32_t color){
  Serial.println("six ");
  grid.setPixelColor(38,color); // S
  grid.setPixelColor(37,color); // I
  grid.setPixelColor(36,color); // X
}
void SEVEN(uint32_t color){
  Serial.println("seven ");
  grid.setPixelColor(35,color); // S
  grid.setPixelColor(34,color); // E
  grid.setPixelColor(33,color); // V
  grid.setPixelColor(32,color); // E
  grid.setPixelColor(31,color); // N
}
void EIGHT(uint32_t color){
  Serial.println("eight ");
  grid.setPixelColor(30,color); // E
  grid.setPixelColor(29,color); // I
  grid.setPixelColor(28,color); // G
  grid.setPixelColor(27,color); // H
  grid.setPixelColor(26,color); // T
}
void NINE(uint32_t color){
  Serial.println("nine ");
  grid.setPixelColor(13,color); // N
  grid.setPixelColor(14,color); // I
  grid.setPixelColor(15,color); // N
  grid.setPixelColor(16,color); // E
}
void TEN(uint32_t color){
  Serial.println("ten ");
  grid.setPixelColor(17,color); // T
  grid.setPixelColor(18,color); // E
  grid.setPixelColor(19,color); // N
}
void MTEN(uint32_t color){
  Serial.println("m-ten ");
  grid.setPixelColor(136,color); // T
  grid.setPixelColor(135,color); // E
  grid.setPixelColor(134,color); // N
}
void ELEVEN(uint32_t color){
  Serial.println("eleven ");
  grid.setPixelColor(20,color); // E
  grid.setPixelColor(21,color); // L
  grid.setPixelColor(22,color); // E
  grid.setPixelColor(23,color); // V
  grid.setPixelColor(24,color); // E
  grid.setPixelColor(25,color); // N
}
static void TWELVE(uint32_t color){
  Serial.println("twelve ");
  grid.setPixelColor(12,color); // T
  grid.setPixelColor(11,color); // W
  grid.setPixelColor(10,color); // E
  grid.setPixelColor(9,color); // L
  grid.setPixelColor(8,color); // V
  grid.setPixelColor(7,color); // E
}
static void TWENTY(uint32_t color){
  Serial.println("twenty ");
  grid.setPixelColor(116,color); // T
  grid.setPixelColor(115,color); // W
  grid.setPixelColor(114,color); // E
  grid.setPixelColor(113,color); // N
  grid.setPixelColor(112,color); // T
  grid.setPixelColor(111,color); // Y
}
void THEAGE(){
  THE1(colorWhite);
  grid.show();
  delay(1000);
  AGE(colorWhite);
  grid.show();
  delay(1000);
  OF(colorWhite);
  grid.show();
  delay(1000);
  THE2(colorWhite);
  grid.show();
  delay(1000);
  CUSTOMER(colorWhite);
  grid.show();
  delay(1000);
}
void THECUSTOMER(){
  JAVELIN(colorJGreen);
  grid.show();
  delay(2000);
  IS(colorWhite);
  grid.show();
  delay(1000);
  THE1(colorJGreen);
  grid.show();
  delay(1000);
  AGENCY(colorJGreen);
  grid.show();
  delay(1000);
  OF(colorJGreen);
  grid.show();
  delay(1000);
  THE2(colorJGreen);
  grid.show();
  delay(1000);
  CUSTOMER(colorJGreen);
  grid.show();
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
  HELLO_SPELL(colorJGreen);
  delay(1000);
  fadeOut(50);
  grid.setBrightness(intBrightness);
  HELLO(colorWhite);
  delay(1000);
  fadeOut(50);
  grid.setBrightness(intBrightness);
  colorWipe(colorJGreen, 50);
  chase(colorRed,2); // Red
  chase(colorGreen,2); // Green
  chase(colorBlue,2); // Blue
  chase(colorWhite,2); // White
  colorWipe(colorBlack, 0);
  TIME(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  WE(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  AT(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  IT(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  IS(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  HALF(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  A(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  QUARTER(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  PAST(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  OCLOCK(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  TO(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  ONE(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  TWO(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  THREE(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  FOUR(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  MFIVE(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  FIVE(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  SIX(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  SEVEN(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  EIGHT(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  NINE(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  MTEN(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  TEN(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  ELEVEN(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  TWELVE(colorWhite);
  grid.show();
  delay(1000);
  colorWipe(colorBlack, 0);
  TWENTY(colorWhite);
  grid.show();
  delay(1000);

  colorWipe(colorBlack, 0);
  JAVELIN(colorJGreen);
  grid.show();
  delay(2000);
  IS(colorWhite);
  grid.show();
  delay(1000);
  THE1(colorWhite);
  grid.show();
  delay(1000);
  AGENCY(colorWhite);
  grid.show();
  delay(1000);
  OF(colorWhite);
  grid.show();
  delay(1000);
  THE2(colorWhite);
  grid.show();
  delay(1000);
  CUSTOMER(colorWhite);
  grid.show();
  delay(1000);
  fadeOut(100);
  colorWipe(colorBlack, 0);
  grid.setBrightness(intBrightness);
  intTestMode = !intTestMode;
}
