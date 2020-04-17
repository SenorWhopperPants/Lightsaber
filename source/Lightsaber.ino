/*
 * This file runs the code for a custom lightsaber
 *
 * @author: Josh Bussis
 * @date:   8/11/2019   
 */
#include <Wire.h>
#include <pcmConfig.h>
#include <pcmRF.h>
#include <TMRpcm.h>
#include <SD.h>
#include <SPI.h>
TMRpcm tmrpcm;

// CONSTANTS
#define BLADE           6
#define BUTTON          2
#define MOTION          7
#define SD_CHIP_SELECT  4
const int MPU_ADDR =    0x68;

// needed shared variables
unsigned int counter, buttonClicks;
unsigned char bladeOn;
unsigned char lock;

// accel and gyro values
int16_t acX, acY, acZ;
double accel_x, accel_y, accel_z;
double accel_mag;

// sound files
const char* power_on =  "power_on.wav";
const char* power_off = "power_off.wav";
const char* swing_1 =   "clash_1.wav";
const char* swing_2_1 = "clash_2_1.wav";
const char* swing_2_2 = "clash_2_2.wav";
const char* swing_2_3 = "clash_2_3.wav";
const char* swing_3 =   "clash_3.wav";
const char* hum =       "hum.wav";
const char* clash_1 =   "clash_1";
const char* clash_2 =   "clash_2";
const char* clash_3 =   "clash_3";

/*
 * setup() is the POR of the project
 * @brief:  Sets up variables, registers, and the interrupt vector
 * 
 * @input:  void
 * @return: void
 */
void setup() {
  // setup for gyroscope
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  // registers
  pinMode(BLADE, OUTPUT);
  pinMode(MOTION, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  pinMode(SD_CHIP_SELECT, OUTPUT);
  digitalWrite(SD_CHIP_SELECT, HIGH);
  // initialize interrupts (pins 2 and 3 can be used for interrupts on the nano)
  
  attachInterrupt(digitalPinToInterrupt(BUTTON), buttonISR, FALLING);
  // should add another isr method to handle gyroscope (for impact detection)
  // will probably use polling for gyroscope on second thought

  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0); // wake up the module
  Wire.endTransmission(true);

  // initilize the microSD
  tmrpcm.speakerPin = 9;
  if (!SD.begin(SD_CHIP_SELECT)) {
    return;
  }
  tmrpcm.setVolume(7);
//  tmrpcm.play("clash_1.wav");

  // initialize needed varibales with values
  counter = 0;
  buttonClicks = 0;
  bladeOn = 0;
  lock = 0;
}

/*
 * loop() is the main loop for the arduino to run
 * @brief:  main code that loops while the arduino is running
 * 
 * @input:  void
 * @return: void
 */
void loop() {
  if (bladeOn) {
    // check if audio is playing, if it isn't play the hum noise again
    if (!tmrpcm.isPlaying()) {
      tmrpcm.play(hum); 
      tmrpcm.loop(1);
    }
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 3*2, true); // request from 6 regesters
    // read the values
    acX = Wire.read()<<8 | Wire.read();
    acY = Wire.read()<<8 | Wire.read();
    acZ = Wire.read()<<8 | Wire.read();
    
    accel_x = acX / 10;
    accel_y = acY / 10;
    accel_z = acZ / 10;   // may want to divide by 100 incase some of these squares have overflow problems
    accel_mag = sqrt((accel_x * accel_x) + (accel_y * accel_y) + (accel_z * accel_z));
    // try to figure out a way to get the velocity from the acceleration to make this more accurate
    if (accel_mag > 2600) {
      // play clash sound (randomly choose one)
      tmrpcm.stopPlayback();
      tmrpcm.play(clash_1); 
      tmrpcm.loop(0);
      
    }
    else if (accel_mag > 2500) {
      // play swing sound (randomly choose one)
      tmrpcm.stopPlayback();
      tmrpcm.play(swing_1);
      tmrpcm.loop(0);
    }
  }
}

/*
 * buttonISR() is the Interrupt Service Routine for the button being clicked
 * @brief:  This method handles whether the lightsaber should be turned on or 
 *          off based on the current state of the blade and the number of 
 *          times the button has been clicked
 *          
 * @input:  void
 * @output: void
 */
void buttonISR() {
  noInterrupts();                             // turn off interrupts so this isn't interrupted
  // debounce the button click
  int origState = digitalRead(BUTTON);        // read the state of the button
  int state;
  delayMicroseconds(50000);                   // 50 ms delay for debounce
  state = digitalRead(BUTTON);                // read the state of the button again after a delay
  if (state == 0 && origState == 0) {         // check if the original and new states are the same
    // buttonClicks++;                        // check what action the lightsaber should do
    if (!bladeOn) {                           // the blade is off and has been clicked once  (&& buttonClicks == 1)
      tmrpcm.play(power_on);                  // play power_on sound
      for (int i = 0; i < 255; ++i) {         // *** turn the blade on and play the ignition sound here ***
        analogWrite(BLADE, i);
        delay(10);
      }
      bladeOn = 1;                            // store that the blade has been turned on
      // counter = 0;                         // reset the counter 
    }
    else if (bladeOn) {                       // the blade is on and has been clicked three or more times (&& buttonClicks >= 3)
      tmrpcm.stopPlayback();
      tmrpcm.play(power_off);                 // play power off sound
      tmrpcm.loop(0);
      for (int i = 254; i >= 0; --i) {        // *** turn off the blade and play the shut down sound here ***
        analogWrite(BLADE, i);
        delay(5);
      }
      bladeOn = 0;                            // store that the blade has been turned off
      // counter = 0;                         // reset the counter
    }
  }
  interrupts();                               // allow interrupts again
}


/*
 * NOTES:
 *  -> PWM on an arduino has a value ranging from 0-255; this value has to be stored as a byte
 *  -> To set a pin to send out PWM signals, must use pinMode(pin, mode) to set the pin to OUTPUT
 *  -> To send the PWM signal, use analogWrite(pin, value) where value is a byte
 */



// THIS IS PROTOTYPE FOR MULTIPLE CLICKS (IN LOOP)
/* if (counter >= 1023) {
    // clear the counter and number of button clicks
    counter = 0; 
    buttonClicks = 0;
  }

  // if the button has been clicked, start counting
  if (buttonClicks > 0) {
    counter++;
  } */
