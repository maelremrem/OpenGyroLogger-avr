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
#define BTN_PIN 3
bool isRecording = false;
bool btnReleased = false;
bool blinkState = false;
char txtname[] = "gf.txt";
int _time = 0;
//gfloggerinfo
#include <avr/pgmspace.h>
PROGMEM const char logger_header[] = "GYROFLOW IMU LOG\nversion,1.1\nid,custom_logger_name\norientation,YxZ\nnote,development_test\fwversion,FIRMWARE_0.1.0\ntimestamp,1644159993\vendor,potatocam\videofilename,videofilename.mp4\lensprofile,potatocam_mark1_prime_7_5mm_4k\tscale,0.001\gscale,0.00122173047\nascale,0.00048828125\nt,gx,gy,gz,ax,ay,az";
String templine;
bool newLine = true;
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
    pinMode(BTN_PIN, INPUT_PULLUP);
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
      Serial.println("HeaderInfoWrited...");
      Serial.println("Ready to write MPU data !");
      Serial.println("t,gx,gy,gz,ax,ay,az");
    }else{
      Serial.println("SDError...");
    }
    // initialize timer1 
    cli();           // disable all interrupts
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = 0;
    OCR1A = F_CPU / 1000; 
    TCCR1B = (1 << WGM12);   // CTC mode
    TCCR1B = (1 << CS12);    // // Frequency 16Mhz/ 256 = 62500 
    TIMSK1 = (1 << OCIE1A);  // Local interruption OCIE1A
    sei();             // enable all interrupts
    
}
void loop() {
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
  if(!digitalRead(BTN_PIN)&&isRecording&&btnReleased){
    Serial.println("Stopping record and closing file...");
    isRecording=false;
    myFile.close();
    for(int i=0;i<5;i++){
      blinkState = !blinkState;
      digitalWrite(LED_PIN, blinkState);
      delay(100);
    }
  }
}


ISR(TIMER1_COMPA_vect)        // interrupt service routine that wraps a user defined function supplied by attachInterrupt
{
  WriteNewLine();
}

void WriteNewLine(){
   if (myFile && isRecording) {
    Serial.println("Recording...");
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    //t,gx,gy,gz,ax,ay,az
    //0,39,86,183,-1137,-15689,-2986
    templine = String(_time);
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
    blinkState = !blinkState;
    digitalWrite(LED_PIN, blinkState);
    _time++;
  }
}
