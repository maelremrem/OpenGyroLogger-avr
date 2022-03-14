#include <SPI.h>
#include <SD.h>
#include "I2Cdev.h"
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
bool blinkState = false;
char txtname[] = "gf.txt";
int _time = 0;
//gfloggerinfo
#include <avr/pgmspace.h>
PROGMEM const char logger_header[] = "GYROFLOW IMU LOG";
PROGMEM const char logger_header_version[] = "version,1.1";
PROGMEM const char logger_name[] = "id,custom_logger_name";
PROGMEM const char logger_orientation[] = "orientation,YxZ";
PROGMEM const char logger_note[] = "note,development_test";
PROGMEM const char logger_fwversion[] = "fwversion,FIRMWARE_0.1.0";
PROGMEM const char logger_timestamp[] = "timestamp,1644159993";
PROGMEM const char logger_vendor[] = "vendor,potatocam";
PROGMEM const char logger_filename[] = "videofilename,videofilename.mp4";
PROGMEM const char logger_lensprofile[] = "lensprofile,potatocam_mark1_prime_7_5mm_4k";
PROGMEM const char logger_timescale[] = "tscale,0.001";
PROGMEM const char logger_gscale[] = "gscale,0.00122173047";
PROGMEM const char logger_ascale[] = "ascale,0.00048828125";
PROGMEM const char logger_headdata[] = "t,gx,gy,gz,ax,ay,az";


void setup() {
    // join I2C bus (I2Cdev library doesn't do this automatically)
    #if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
        Wire.begin();
    #elif I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE
        Fastwire::setup(400, true);
    #endif   // initialize serial communication
    // serial value can be up to you depending on project
    Serial.begin(9600);
    Serial.println("Initializing GyroFlow Logger...");
    // initialize SD card
    if (SD.begin())
    {
        Serial.println("SD card is ready to use.");
    } else
    {
        Serial.println("SD card initialization failed");
        return;
    }
    // initialize accel/gyro
    Serial.println("Initializing accelerometer...");
    accelgyro.initialize();
    accelgyro.setSleepEnabled(false);
    // verify connection
    Serial.println("Testing accelerometer connection...");
    Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");
    // configure Arduino LED for
    pinMode(LED_PIN, OUTPUT);
    if(SD.exists(txtname)){
      Serial.println("File Allready present... Removing...");
      SD.remove(txtname);
      Serial.println("File removed !");
    }else{
      Serial.println("New Created !");
    }
    myFile = SD.open(txtname, FILE_WRITE);
    if (myFile) {
      myFile.println(logger_header);
      myFile.println(logger_header_version);
      myFile.println(logger_name);
      myFile.println(logger_orientation);
      myFile.println(logger_note);
      myFile.println(logger_fwversion);
      myFile.println(logger_timestamp);
      myFile.println(logger_vendor);
      myFile.println(logger_filename);
      myFile.println(logger_lensprofile);
      myFile.println(logger_timescale);
      myFile.println(logger_gscale);
      myFile.println(logger_ascale);
      myFile.println(logger_headdata); 
      myFile.close();
      Serial.println("HeaderInfoWrited...");
      Serial.println("Ready to write MPU data !");
      Serial.println("t,gx,gy,gz,ax,ay,az");
    }else{
      Serial.println("SDError...");
    }
    
}
void loop() {
    // read raw accelerometer measurements from device
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    
    // write the accelerometer values to SD card
    myFile = SD.open(txtname, FILE_WRITE);
    if (myFile) {
      //t,gx,gy,gz,ax,ay,az
      //0,39,86,183,-1137,-15689,-2986
      String templine = String(_time);
      templine.concat(",");
      templine.concat(String(gx));
      templine.concat(",");
      templine.concat(String(gy));
      templine.concat(",");
      templine.concat(String(gz));
      templine.concat(",");
      templine.concat(String(ax));
      templine.concat(",");
      templine.concat(String(ay));
      templine.concat(",");
      templine.concat(String(az));
      Serial.println(templine);
      myFile.println(templine);
      /*
      myFile.print(_time);
      myFile.print(gx); myFile.print(",");
      myFile.print(gy); myFile.print(",");
      myFile.print(gz); myFile.print(",");
      myFile.print(ax); myFile.print(",");
      myFile.print(ay); myFile.print(",");
      myFile.println(az); 
      */
      myFile.close();
    }
    
    // blink LED to indicate activity
    blinkState = !blinkState;
    digitalWrite(LED_PIN, blinkState);
    _time++;
}
