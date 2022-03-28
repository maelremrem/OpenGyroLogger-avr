#include <SPI.h>
#include <SD.h>
#include "I2Cdev.h"
#include <avr/pgmspace.h>
#include "MPU6050.h"
#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
    #include "Wire.h"
#endif>
File myFile;// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 accelgyro;
//MPU6050 accelgyro(0x69); // <-- use for AD0 high
int pinCS = 10; //CS pin for SD card 
int scale = 16384; //divide values by MPU6050 sensitivity scale to get readings in g's (m/s^2 / 9.8)
                   //use scale = 16384 for the default I2Cdevlib sensitivity setting of +/-2
int16_t ax, ay, az;
int16_t gx, gy, gz;
#define LED_PIN 13
#define BTN_PIN 3

bool isRecording = false;
bool isRecordingShown = false;
bool btnReleased = false;
static bool blinkState = false;
char configFileName[] = "gf.conf";
#define TIMESTAMP 160000
#define TIMESCALE 0.001
#define GSCALE 0.00012
#define ASCALE 0.0004
char txtname[] = "gf";
static int nbLogs = 0;
static int16_t _time = 0;
#define USE_TIMER_1     true
#include "TimerInterrupt.h"
#define TIMER_INTERVAL_MS    100
void setup() {
  //init config
  File configFile;
    int timestamp = 0 ;
    float timeScale = 0;
    float gScale = 0;
    float aScale = 0;
    int fileNumber = 0;
   // join I2C bus (I2Cdev library doesn't do this automatically)
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif   // initialize serial communication
   Serial.begin(9600);
   Serial.println(F("Initializing GyroFlow Logger..."));
   // initialize SD card
   if (SD.begin())
   {
       Serial.println(F("SD card is ready to use."));
   } else {
       Serial.println(F("SD card initialization failed"));
       while(true){
          blinkState = !blinkState;
          digitalWrite(LED_PIN, blinkState);
          delay(100);
       }
   }
//load config file to custom header and logging speed
//timestamp;timescale;gscale;ascale
   if(SD.exists(configFileName)){
      Serial.println(F("Config File Found !"));
      configFile = SD.open(configFileName, FILE_READ);
      //read config file
      //char tempLine[30] = configFile.read();
      char * pch;
      //pch = strtok (tempLine,";");
      //split loop
   }else{
     Serial.println(F("Config File NOT Found !"));
     configFile = SD.open(configFileName, FILE_WRITE);
     //write default config file from define
     timestamp = TIMESTAMP;
     timeScale = TIMESCALE;
     gScale = GSCALE;
     aScale = ASCALE;
     //configFile.println(F("GYROFLOW IMU LOG\nversion,1.1\nid,custom_logger_name\norientation,YxZ\nnote,development_test\nfwversion,FIRMWARE_0.1.0\ntimestamp,1644159993\nvendor,potatocam\nvideofilename,videofilename.mp4\nlensprofile,potatocam_mark1_prime_7_5mm_4k\ntscale,0.001\ngscale,0.00122173047\nascale,0.00048828125\nt,gx,gy,gz,ax,ay,az"));
     //configFile.println(F(timestamp.toSring()+";"+timeScale+";"+gScale+";"+aScale));
     configFile.flush();
   }
   
   if(SD.exists(txtname)){
     Serial.println(F("File Allready present... Removing..."));
     SD.remove(txtname);
     Serial.println(F("File removed !"));
   }else{
     Serial.println(F("New Created !"));
   }
   myFile = SD.open(txtname, FILE_WRITE);
   if (myFile) {

      myFile.println(F("GYROFLOW IMU LOG\nversion,1.1\nid,custom_logger_name\norientation,YxZ\nnote,development_test\nfwversion,FIRMWARE_0.1.0\ntimestamp,1644159993\nvendor,potatocam\nvideofilename,videofilename.mp4\nlensprofile,potatocam_mark1_prime_7_5mm_4k\ntscale,0.001\ngscale,0.00122173047\nascale,0.00048828125\nt,gx,gy,gz,ax,ay,az"));
      myFile.flush();
      Serial.println(F("HeaderInfoWrited..."));
      Serial.println(F("Ready to write MPU data !"));
      Serial.println(F("t,gx,gy,gz,ax,ay,az"));
    }else{
      Serial.println(F("SDError..."));
    }
    // initialize accel/gyro
    Serial.println(F("Initializing accelerometer..."));
    accelgyro.initialize();
    accelgyro.setSleepEnabled(false);
    // verify connection
    Serial.println(F("Testing accelerometer connection..."));
    Serial.println(accelgyro.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));
    // configure Arduino LED for
    pinMode(LED_PIN, OUTPUT);
    pinMode(BTN_PIN, INPUT_PULLUP);
    
    ITimer1.init();
    ITimer1.attachInterruptInterval(TIMER_INTERVAL_MS, WriteNewLine);
    
}
void loop() {
  accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  while(!isRecording){
    Serial.println("Waiting to start...");
    blinkState = !blinkState;
    digitalWrite(LED_PIN, blinkState);
    delay(1000);
    if(!digitalRead(BTN_PIN)){
      Serial.println("Starting record...");
      isRecording=true;
      btnReleased = false;
    }
  }
  if(digitalRead(BTN_PIN)&&!btnReleased){
    btnReleased = true;
  }
  if(isRecording && !isRecordingShown){
    isRecordingShown = true;
    Serial.println(F("Recording..."));
  }
  if(isRecording && isRecordingShown){
    Serial.print(F("t/a/g:\t"));
    Serial.print(_time); Serial.print("\t");
    Serial.print(ax); Serial.print("\t");
    Serial.print(ay); Serial.print("\t");
    Serial.print(az); Serial.print("\t");
    Serial.print(gx); Serial.print("\t");
    Serial.print(gy); Serial.print("\t");
    Serial.println(gz);
  }
  if(!digitalRead(BTN_PIN)&&isRecording&&btnReleased){
    Serial.println(F("Stopping record and closing file..."));
    isRecording=false;
    myFile.close();
    for(int i=0;i<5;i++){
      blinkState = !blinkState;
      digitalWrite(LED_PIN, blinkState);
      delay(100);
    }
    while(true){//halting
      
    }
  }
}


void WriteNewLine(){
   if (myFile && isRecording) {
    myFile.print(_time); myFile.print(",");
    myFile.print(gx);myFile.print(",");
    myFile.print(gy);myFile.print(",");
    myFile.print(gz);myFile.print(",");
    myFile.print(ax);myFile.print(",");
    myFile.print(ay);myFile.print(",");
    myFile.println(az);
    myFile.flush();
    blinkState = !blinkState;
    digitalWrite(LED_PIN, blinkState);
    _time++;
  }
}
