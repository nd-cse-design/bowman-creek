//This example sketch puts the Mayfly board into sleep mode.  It wakes up at specific times, records the temperature
//and battery voltage onto the microSD card, prints the data string to the serial port, and goes back to sleep.
//Trying to add printing of the battery voltage. 
 
#include <Wire.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <SPI.h>
#include <SD.h>
 
#include <RTCTimer.h>
#include <Sodaq_DS3231.h>
#include <Sodaq_PcInt.h>
 
RTCTimer timer;
 
String dataRec = "";
int currentminute;
long currentepochtime = 0;
float boardtemp;
 
int batteryPin = A6;    // select the input pin for the potentiometer
int batterysenseValue = 0;  // variable to store the value coming from the sensor
float batteryvoltage;
 
//RTC Interrupt pin
#define RTC_PIN 10
#define RTC_INT_PERIOD EveryMinute
 
//#define SD_SS_PIN 12
 
//The data log file
//#define FILE_NAME "datafile.txt"
 
//Data header
//#define LOGGERNAME "SampleLogger"
//#define DATA_HEADER "DateTime_EST,Loggertime,BoardTemp_C,Battery_V"

 //MOISTURE SENSOR CODE_____________________________________________________
 
 //Digital pin 12 is the MicroSD slave select pin on the Mayfly
#define SD_SS_PIN 12

//The data log file
#define FILE_NAME "DataLog.txt"

//Data header  (these lines get written to the beginning of a file when it's created)
#define LOGGERNAME "Mayfly microSD Card Tester"
#define DATA_HEADER "SampleNumber, Battery_volts"

int sampleinterval = 1;    //time between samples, in seconds

int samplenum = 1;      // declare the variable "samplenum" and start with 1
int analogNum = 0;
   // on the Mayfly board, pin A6 is connected to a resistor divider on the battery input

int moistureValue = 0;  // variable to store the value coming from the analogRead function
//float moisture;       // the battery voltage as calculated by the formula below

void setup() 
{
  //Initialise the serial connection
  Serial.begin(57600);
  Serial1.begin(9600); //Xbee stuff
  
  rtc.begin();
  delay(100);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
 
  greenred4flash();    //blink the LEDs to show the board is on
 
  setupLogFile();
 
  setupTimer();        //Setup timer events
  
  setupSleep();        //Setup sleep mode
 
  Serial.println("DATA_HEADER");
  showTime(getNow());
}
 
void loop() 
{
  //Update the timer 
  timer.update();
  if(currentminute % 1 == 0)   // change "2" to "5" to wake up logger every 5 minutes instead
     {   Serial.println("Multiple of 2!   Initiating sensor reading and logging data to SDcard....");
          
          dataRec = createDataRecord();
 
          //Save the data record to the log file
          logData(dataRec);
    
          //Echo the data to the serial connection
          //Serial.println();
          Serial.print("Data Record: ");
          Serial.println(dataRec);
          Serial1.print("Data Record: "); //Xbee stuff
          Serial1.println(dataRec); //Xbee stuff  
          String dataRec = "";
          printVolts();
     }
  
  delay(1000);
  //Sleep
  systemSleep();
}

void printVolts(){
  int sensorValue = analogRead(batteryPin);
  float voltage = (3.3/1023) * 1.47 * sensorValue;
  Serial.print("Battery Voltage: ");
  Serial.println(voltage);
  Serial1.print("Battery Voltage: ");
  Serial1.println(voltage);
}
 
void showTime(uint32_t ts)
{
  //Retrieve and display the current date/time
  String dateTime = getDateTime();
  Serial.println(dateTime);
  Serial1.println(dateTime)
}
 
void setupTimer()
{
  
    //Schedule the wakeup every minute
  timer.every(1, showTime);
  
  //Instruct the RTCTimer how to get the current time reading
  timer.setNowCallback(getNow);
 
}
 
void wakeISR()
{
  //Leave this blank
}
 
void setupSleep()
{
  pinMode(RTC_PIN, INPUT_PULLUP);
  PcInt::attachInterrupt(RTC_PIN, wakeISR);
 
  //Setup the RTC in interrupt mode
  rtc.enableInterrupts(RTC_INT_PERIOD);
  
  //Set the sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
}
 
void systemSleep()
{
  //This method handles any sensor specific sleep setup
  sensorsSleep();
  
  //Wait until the serial ports have finished transmitting
  Serial.flush();
  Serial1.flush();
  
  //The next timed interrupt will not be sent until this is cleared
  rtc.clearINTStatus();
    
  //Disable ADC
  ADCSRA &= ~_BV(ADEN);
  
  //Sleep time
  noInterrupts();
  sleep_enable();
  interrupts();
  sleep_cpu();
  sleep_disable();
 
  //Enbale ADC
  ADCSRA |= _BV(ADEN);
  
  //This method handles any sensor specific wake setup
 // sensorsWake();
}
 
void sensorsSleep()
{
    //Serial.println(moisture);

  //Add any code which your sensors require before sleep
}
 
//void sensorsWake()
//{
//  //Add any code which your sensors require after waking
//}
 
String getDateTime()
{
  String dateTimeStr;
  
  //Create a DateTime object from the current time
  DateTime dt(rtc.makeDateTime(rtc.now().getEpoch()));
 
  currentepochtime = (dt.get());    //Unix time in seconds 
 
  currentminute = (dt.minute());
  //Convert it to a String
  dt.addToString(dateTimeStr); 
  return dateTimeStr;  
}
 
uint32_t getNow()
{
  currentepochtime = rtc.now().getEpoch();
  return currentepochtime;
}
 
void greenred4flash()
{
  for (int i=1; i <= 4; i++){
  digitalWrite(8, HIGH);   
  digitalWrite(9, LOW);
  delay(50);
  digitalWrite(8, LOW);
  digitalWrite(9, HIGH);
  delay(50);
  }
  digitalWrite(9, LOW);
}
 
//MOISTURE SENSOR CODE NEW_______________________________
void setupLogFile()
{
  //Initialise the SD card
  if (!SD.begin(SD_SS_PIN))
  {
    Serial.println("Error: SD card failed to initialise or is missing.");
    //Hang
  //  while (true); 
  }
  
  //Check if the file already exists
  bool oldFile = SD.exists(FILE_NAME);  
  
  //Open the file in write mode
  File logFile = SD.open(FILE_NAME, FILE_WRITE);
  // write to file if old file exists
  //Add header information if the file did not already exist
  if (!oldFile)
  {
    logFile.println(LOGGERNAME);
    logFile.println(DATA_HEADER);
  }
  
  //Close the file to save it
  logFile.close();  
}


void logData(String rec)
{
  //Re-open the file
  File logFile = SD.open(FILE_NAME, FILE_WRITE);
  
  //Write the CSV data
  logFile.println(rec);
  
  //Close the file to save it
  logFile.close();  
}

String createDataRecord()
{
  //Create a String type data record in csv format
  //SampleNumber, Battery
  String data = "";
  data += ", Moistures: ";
  
  for (analogNum = 0; analogNum <= 7; analogNum++){ //The 7 sensors being used (A0-A5 and A7)
   if (analogNum == 6){ //A6 is not used for the sensors
    analogNum = 7;
   }
  moistureValue = analogRead(analogNum); //read soil moisture sensor into int
  //int voltage =(moistureValue/1023)*3; //conversion to voltage from analog
  data += "A";
  data += analogNum;
  data += ": ";
  data += moistureValue;
  data += ", ";
  }
  return data; 
}
 
static void addFloatToString(String & str, float val, char width, unsigned char precision)
{
  char buffer[10];
  dtostrf(val, width, precision, buffer);
  str += buffer;
}

