// This example sketch puts the Mayfly board into sleep mode.  It wakes up at specific times, records the temperature
// and battery voltage onto the microSD card, prints the data string to the serial port, and goes back to sleep.  This version has code to push data to the XCTU app using a connected XBEE.
// Before running this code run adjust with the specific time and date to be adjusted based upon what the current date is

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
int awake;

int batteryPin = A6;    // select the input pin for the potentiometer
int batterysenseValue = 0;  // variable to store the value coming from the sensor
float batteryvoltage;

//RTC Interrupt pin
#define RTC_PIN 10
#define RTC_INT_PERIOD EveryMinute

//MOISTURE SENSOR CODE_____________________________________________________

//Digital pin 12 is the MicroSD slave select pin on the Mayfly
#define SD_SS_PIN 12

//The data log file, CHANGE THIS BASED UPON WHAT YOU ARE READING
#define FILE_NAME "Test2.txt"

//Data header  (these lines get written to the beginning of a file when it's created)
#define LOGGERNAME "Mayfly microSD Card Tester"
#define DATA_HEADER "Date and Time, Moisture Values"

int sampleinterval = 1;    //time between samples, in seconds

int samplenum = 1;      // declare the variable "samplenum" and start with 1
int analogNum = 0;

// on the Mayfly board, pin A6 is connected to a resistor divider on the battery input=======
   // on the Mayfly board, pin A6 is connected to a resistor divider on the battery input

int moistureValue = 0;  // variable to store the value coming from the analogRead function
float moisture;       // the battery voltage as calculated by the formula below

String global_date = "";

void setup()
{
  //Initialise the serial connection
  Serial1.begin(9600); //Xbee stuff
  rtc.begin();
  delay(100);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  
  setupLogFile();

  setupTimer();        //Setup timer events

  setupSleep();        //Setup sleep mode
}

void loop()
{
  //Update the timer
  timer.update();

  // change "2" to "5" to wake up logger every 5 minutes instead
  if (currentminute % 1 == 0)   {
    dataRec = global_date;
    dataRec += createDataRecord();
    
    //Save the data record to the log file
    logData(dataRec);

    //Echo the data to the serial connection
    Serial1.print("Time: ");  // print to xbee
    Serial1.println(global_date); // will be able to take this out once data is logged to SD
    // printing logfile to Xbee
    dumpToXB();
    
    String dataRec = "";
    /* prints out the voltage */
    printVolts();
  }
  
  delay(1000);
  
  //Sleep
  systemSleep();
}

float getVolts(){
  int sensorValue = analogRead(batteryPin);
  float voltage = (3.3/1023) * 1.47 * sensorValue;
  return voltage;
}

void printVolts(){
  int sensorValue = analogRead(batteryPin);
  float voltage = (3.3/1023) * 1.47 * sensorValue;
  Serial1.print("Battery Voltage: ");
  Serial1.println(voltage);
}
 
void showTime(uint32_t ts)
{
  //Retrieve and display the current date/time
  String dateTime = getDateTime();
  global_date = getDateTime();
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
  attachInterrupt(0, wakeISR, LOW);

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

/* gets current time
 *  
 */
uint32_t getNow()
{
  currentepochtime = rtc.now().getEpoch();
  return currentepochtime;
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

/*
 * logs the data to the SDCard
 */
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
  String data = "";
  
  data += ", Moistures: ";
  
  for (analogNum = 0; analogNum <= 7; analogNum++){ //The 7 sensors being used (A0-A5 and A7)
   if (analogNum == 6){ //A6 is not used for the sensors, it is the battery pin
    analogNum = 7;
   }
  moistureValue = analogRead(analogNum); //read soil moisture sensor
  int voltage =(moistureValue/1023)*3; //conversion to voltage from analog
  data += "A";
  data += analogNum;
  data += ": ";
  data += moistureValue;
  data += ", ";
  }
  return data; 
}

/*
 * changes a string to a float value
 */
static void addFloatToString(String & str, float val, char width, unsigned char precision)
{
  char buffer[10];
  dtostrf(val, width, precision, buffer);
  str += buffer;
}

/*
 * Useless function for now, attachinterrupt disables reading from serial
 */
int checkXB(){
  char incomingByte;
  if(Serial1.available() > 0){
    return 1;
    incomingByte = Serial1.read();
    if(incomingByte == 'r' || incomingByte == 'R'){
      Serial1.println("Hello");
      return 1;
    }
    else{
      return 0;
    }
  }
  else{
    return 0;
  }
}

/* this dumps the data from the xbee to the xctu monitor
 *  after it dumps to the monitor save the log and run decodeXCTU.py to get readable output
 *  
 */
void dumpToXB(){
  File logFile = SD.open(FILE_NAME);
  if(logFile){
    Serial1.println(FILE_NAME);

//    read from the file until there is nothing else in it
    while(logFile.available()){
       Serial1.write(logFile.read());
    }
    // close the file
    logFile.close();
  }
  else{
    // if the file didn't open output an error
    Serial1.println("Error opening file");
  }
}

