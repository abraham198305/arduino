/* Copyright (C) 2012 Kristian Lauszus, TKJ Electronics. All rights reserved.
 
 This software may be distributed and modified under the terms of the GNU
 General Public License version 2 (GPL2) as published by the Free Software
 Foundation and appearing in the file GPL2.TXT included in the packaging of
 this file. Please note that GPL2 Section 2[b] requires that all works based
 on this software must also be made publicly available under the terms of
 the GPL2 ("Copyleft").
 
 Contact information
 -------------------
 
 Kristian Lauszus, TKJ Electronics
 Web      :  http://www.tkjelectronics.com
 e-mail   :  kristianl@tkjelectronics.com
 */

#include <Wire.h>
#include "Kalman.h" // Source: https://github.com/TKJElectronics/KalmanFilter

Kalman kalmanX; // Create the Kalman instances
Kalman kalmanY;
const int numReadings = 10;

int readings[numReadings]; // the readings from the analog input
int readings_ca[numReadings];
int index = 0;                  // the index of the current reading
int total = 0;                  // the running total
int average = 0; // the average
int total1 = 0;
int average1 = 0; // the average
/* IMU Data */
int16_t accX, accY, accZ;
int16_t tempRaw;
int16_t gyroX, gyroY, gyroZ;

double accXangle, accYangle; // Angle calculate using the accelerometer
double temp; // Temperature
double gyroXangle, gyroYangle; // Angle calculate using the gyro
double compAngleX, compAngleY; // Calculate the angle using a complementary filter
double kalAngleX, kalAngleY; // Calculate the angle using a Kalman filter

uint32_t timer;
uint8_t i2cData[14]; // Buffer for I2C data
double CurrentAngle;
double Ref_Angle=269;

int angleThreshold = 5;
int diffAngle;
int kp = 408/angleThreshold;
int kd = 0, ki = 0;
int dvt_diffAngle, int_diffAngle ;
int diffAngle_1;



 

 
#define runEvery(t) for (static typeof(t) _lasttime;(typeof(t))((typeof(t))millis() - _lasttime) > (t);_lasttime += (t))

void setup() 
{  
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(7, OUTPUT);
  for (int thisReading = 0; thisReading < numReadings; thisReading++)
    readings[thisReading] = 0;  
  
  //Serial.begin(9600);
  Wire.begin();
  TWBR=((F_CPU/400000L)-16)/2;
  i2cData[0] = 7; // Set the sample rate to 1000Hz - 8kHz/(7+1) = 1000Hz
  i2cData[1] = 0x00; // Disable FSYNC and set 260 Hz Acc filtering, 256 Hz Gyro filtering, 8 KHz sampling
  i2cData[2] = 0x00; // Set Gyro Full Scale Range to ±250deg/s
  i2cData[3] = 0x00; // Set Accelerometer Full Scale Range to ±2g
  while(i2cWrite(0x19,i2cData,4,false)); // Write to all four registers at once
  while(i2cWrite(0x6B,0x01,true)); // PLL with X axis gyroscope reference and disable sleep mode 
  
  while(i2cRead(0x75,i2cData,1));
  if(i2cData[0] != 0x68) 
  { // Read "WHO_AM_I" register
    Serial.print(F("Error reading sensor"));
    while(1);
  }
  delay(100); // Wait for sensor to stabilize
  
  /* Set kalman and gyro starting angle */
  while(i2cRead(0x3B,i2cData,6));
  accX = ((i2cData[0] << 8) | i2cData[1]);
  accY = ((i2cData[2] << 8) | i2cData[3]);
  accZ = ((i2cData[4] << 8) | i2cData[5]);
  // atan2 outputs the value of -p to p (radians) - see http://en.wikipedia.org/wiki/Atan2
  // We then convert it to 0 to 2p and then from radians to degrees
  accYangle = (atan2(accX,accZ)+PI)*RAD_TO_DEG;
  accXangle = (atan2(accY,accZ)+PI)*RAD_TO_DEG;
  
  kalmanX.setAngle(accXangle); // Set starting angle
  kalmanY.setAngle(accYangle);
  gyroXangle = accXangle;
  gyroYangle = accYangle;
  compAngleX = accXangle;
  compAngleY = accYangle;
  timer = micros();
  //int kp_pot = analogRead(A0)/4;
  //kp = kp_pot/angleThreshold;

  
  
}


void loop() 
{

  
  runEvery(50)  // run code @ 40 Hz
  {

    

    diffAngle_1 = diffAngle;
    dof();
    diffAngle = gyroYangle - kalAngleY;
        
    if (diffAngle > angleThreshold)
    {
      diffAngle = angleThreshold;
    }
      
    else if (diffAngle < -angleThreshold)
    {
      diffAngle = -angleThreshold;
    }
      
    int pwm_width = kp*diffAngle + kd*dvt_diffAngle + ki*int_diffAngle;
    
    if(diffAngle > 0)
    {
      forward(abs(pwm_width)); 
    }
    else if (diffAngle < 0)
    {
      reverse(abs(pwm_width));
    }
    
    dvt_diffAngle = diffAngle-diffAngle_1;
    int_diffAngle = int_diffAngle+diffAngle;
    


    
    Serial.print(gyroYangle);Serial.print("\t");
    Serial.print(kalAngleY);Serial.print("\t");
    Serial.print(diffAngle);Serial.print("\t");
    Serial.print(pwm_width);Serial.print("\t");
    Serial.print(dvt_diffAngle);Serial.print("\t");
    Serial.println("");

  }

}

void forward(int pwm_width)
{
  digitalWrite(8, HIGH);
  digitalWrite(7, LOW);
  analogWrite(9, pwm_width);
}

void reverse(int pwm_width)
{
  digitalWrite(8, LOW);
  digitalWrite(7, HIGH);
  analogWrite(9, pwm_width);
}


 


void dof() {
  /* Update all the values */  
  while(i2cRead(0x3B,i2cData,14));
  accX = ((i2cData[0] << 8) | i2cData[1]);
  accY = ((i2cData[2] << 8) | i2cData[3]);
  accZ = ((i2cData[4] << 8) | i2cData[5]);
  tempRaw = ((i2cData[6] << 8) | i2cData[7]);  
  gyroX = ((i2cData[8] << 8) | i2cData[9]);
  gyroY = ((i2cData[10] << 8) | i2cData[11]);
  gyroZ = ((i2cData[12] << 8) | i2cData[13]);
  
  // atan2 outputs the value of -p to p (radians) - see http://en.wikipedia.org/wiki/Atan2
  // We then convert it to 0 to 2p and then from radians to degrees
  accXangle = (atan2(accY,accZ)+PI)*RAD_TO_DEG;
  accYangle = (atan2(accX,accZ)+PI)*RAD_TO_DEG;
  
  double gyroXrate = (double)gyroX/131.0;
  double gyroYrate = -((double)gyroY/131.0);
  
//  gyroXangle += gyroXrate*((double)(micros()-timer)/1000000); // Calculate gyro angle without any filter  
//  gyroYangle += gyroYrate*((double)(micros()-timer)/1000000);
//  //gyroXangle += kalmanX.getRate()*((double)(micros()-timer)/1000000); // Calculate gyro angle using the unbiased rate
//  //gyroYangle += kalmanY.getRate()*((double)(micros()-timer)/1000000);
//  
 compAngleX = (0.93*(compAngleX+(gyroXrate*(double)(micros()-timer)/1000000)))+(0.07*accXangle); // Calculate the angle using a Complimentary filter
//  compAngleY = (0.93*(compAngleY+(gyroYrate*(double)(micros()-timer)/1000000)))+(0.07*accYangle);

//  
  kalAngleX = kalmanX.getAngle(accXangle, gyroXrate, (double)(micros()-timer)/1000000); // Calculate the angle using a Kalman filter
  kalAngleY = kalmanY.getAngle(accYangle, gyroYrate, (double)(micros()-timer)/1000000);
  CurrentAngle = kalmanY.getAngle(accYangle, gyroYrate, (double)(micros()-timer)/1000000);
  timer = micros();
  
  temp = ((double)tempRaw + 12412.0) / 340.0;
  
  
  /* Print Data */
  /*
  Serial.print(accX);Serial.print("\t");
  Serial.print(accY);Serial.print("\t");
  Serial.print(accZ);Serial.print("\t");
  
  Serial.print(gyroX);Serial.print("\t");
  Serial.print(gyroY); Serial.print("\t");
  Serial.print(gyroZ);Serial.print("\t");Serial.print("\n");
*/
//  Serial.print(accXangle);Serial.print("\t");
 // Serial.print(gyroXangle);Serial.print("\t");
//  Serial.print(compAngleX);Serial.print("\t");
  //Serial.print(kalAngleX);Serial.print("\t");
//  
//  Serial.print("\t");
//  
//  Serial.print(accYangle);Serial.print("\t");
 // Serial.print(gyroYangle);Serial.print("\t");
//  Serial.print(compAngleY); Serial.print("\t");
  //Serial.print(kalAngleY);Serial.print("\t");
  
 //Serial.print(temp);Serial.print("\t");Serial.print("\n");
  
  
//Serial.print(CurrentAngle); Serial.print("\t");

//Serial.print("\r\n");
// delay(1);
}
