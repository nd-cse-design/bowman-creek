// Working on reading from 7 moisture sensors and setting up sleep mode
#include <avr/sleep.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

//The data log file
#define FILE_NAME "DataLog.txt"
 
//Data header  (these lines get written to the beginning"
#define LOGGERNAME "Presentation Data"
#define DATA_HEADER "SampleTime, PercentMoisture"

// wakePin is 10

long UnixTime;
int intTime;
int lastData = 1;
int mostRecentTime;
int sleepStatus = 0; //variable to store request for sleep
int count = 0; // this is for the timer count in seconds
int analogNum = 0;
float sensorVal;
float voltage;

void wakeUpNow()  //interrupt is handled here after waking up
{
  //execute code here before returning to loop function
  //timers and codes using timer don't work here
  //basically just used to wake up microcontroller
}

void setupLogFile()
{
  //Initialise the SD card
  if (!SD.begin(12))
  {
    Serial1.println("Error: SD card failed to initialise or is missing."); 
  }
  
  //Check if the file already exists
  bool oldFile = SD.exists(FILE_NAME);  
  
  //Open the file in write mode
  File logFile = SD.open(FILE_NAME, FILE_WRITE);
  
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
 
void setup() {
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(2), wakeUpNow, CHANGE); // interrupt 0 is attached to digital pin 2
  setupLogFile();
  sleepNow();
  Wire.begin();
}

void sleepNow() //put microcontroller to sleep here
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); 
  sleep_enable();

  attachInterrupt(digitalPinToInterrupt(2), wakeUpNow, CHANGE); //reattach interrupt here
  sleep_mode(); //where device is put to sleep

  Serial.println("Sensors are back on");
  delay(100);
  sleep_disable(); //first thing done after waking up
  detachInterrupt(digitalPinToInterrupt(2)); //disable interrupt so that wakeUpNow
                      //will not be executed during normal running
}

String createDataRecord()
{
  //Create a String type data record in csv format
  //SampleTime, Moisture
  String data = "Time: ";
  data += millis();
  data += ", Moistures: ";
  
  for (analogNum = 0; analogNum <= 7; analogNum++){
    if (analogNum == 6){
      analogNum = 7;
    }
    sensorVal = analogRead(analogNum); //read soil moisture sensor
    voltage =(sensorVal/1023)*3; //conversion to voltage from analog
  
    data += voltage;     // raingarden team wants raw voltage
  }
  
  return data;
}

void loop() {
  intTime = millis();
  if (intTime%60 == 0 && (mostRecentTime != intTime))  //do this every 60 seconds for 30 minutes
  {
     String dataRec = createDataRecord();  //collect data into a string
  
    //Save the data record to the log file
    logData(dataRec); 
    
    mostRecentTime = intTime;  //change Unix time
    
    count++; //count up to 2 minutes before going to sleep
  }

  if (count >= 120) { // sleep after 2 minutes
      Serial.println("Timer: Entering Sleep mode"); //goes to sleep after 30 minutes
      delay(100);     // this delay is needed, the sleep
                      //function will provoke a Serial error otherwise!!
      count = 0;
      sleepNow();     // sleep function called here
  }
 }

