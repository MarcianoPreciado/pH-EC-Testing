/* EC Calibration
    author: Marciano C. Preciado
    date: 03-19-2018

    This sketch makes use of the Atlas Scientific EC probe and EZO EC Circuit.
    The communication method with EZO is I2C. The EZO is calibrated using the
    3-point method. This sketch assumes the EZO circuit is already set to I2C mode.

    Instructions (courtesy of Atlas Scientific)
    Dry Calibration
      1. Issuing the "Cal,dry" command fine tunes the internal electrical properties of the device.
        This calibration only needs to be done once. Even though you may see reading of 0.00
        before issuing the "Cal,dry" command, it is still a necessary component of calibration
    Lowpoint Calibration
      1. Pour a small amount of the calibration solution into a cup. Shake the probe to make sure
        you do not have trapped air bubbles in the sensing area. You should see readings that
        are off by 1 – 40% from the stated value of the calibration solution. Wait for readings to
        stabilize (small movement from one reading to the next is normal).
      2. Once the readings stabilize, issue the low point or single point calibration command.
        Low point calibration: "Cal,low,12880" (Readings will NOT change)
    Highpoint Calibration
      1. Shake the probe to remove trapped air and adjust the temperature as done in the previous
        step. Once the readings have stabilized issue the high point calibration command.
        High point calibration: "Cal,high,80000" (Readings will change, calibration complete)
*/
#include <Wire.h>

// Default EC EZO address
const int addr = 0x64;
const int led_pin = 13;

// Function Prototypes
void dry_cal();
void lowpoint_cal();
void highpoint_cal();
float read_ec();
float read_until_settled();
void sleep_mode();
float finite_diff(float fx3, float fx2, float fx1, float h);

void setup() {
  Wire.begin();
  Serial.begin(9600);
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, HIGH);
}

void loop() {
  while (analogRead(A7) > 100) { // Wait for button press
    Serial.println(read_ec(),5);
  }
  digitalWrite(led_pin, LOW);
  dry_cal();
  sleep_mode();
  digitalWrite(led_pin, HIGH);

  while (analogRead(A7) > 100) { // Wait for button press for next calibration
    Serial.println(read_ec(),5);
  }
  digitalWrite(led_pin, LOW);
  lowpoint_cal();
  sleep_mode();
  digitalWrite(led_pin, HIGH);
  
  while (analogRead(A7) > 100) { // Wait for button press for next calibration
    Serial.println(read_ec(),5);
  }
  digitalWrite(led_pin, LOW);
  highpoint_cal();
  sleep_mode();
  digitalWrite(led_pin, HIGH);

  // Blink on and off to signify completed calibration
  while(true){
    digitalWrite(led_pin, HIGH);
    delay(250);
    digitalWrite(led_pin, LOW);
    delay(250);
  }
}

// Function Definitions
void dry_cal() {
  read_until_settled();
  Wire.beginTransmission(addr);
  Wire.write("Cal,dry");
  Wire.endTransmission();
  delay(305);
  Wire.requestFrom(addr, 2);
  byte response = Wire.read();
  byte terminator = Wire.read();
  if (response = 1)
    return;
  else {
    while (response != 1) {
      delay(50);
      Wire.requestFrom(addr, 2);
      response = Wire.read();
      terminator = Wire.read();
    }
  }
}


void lowpoint_cal() {
  read_until_settled();
  Wire.beginTransmission(addr);
  Wire.write("Cal,low,12880");
  Wire.endTransmission();
  delay(305);
  Wire.requestFrom(addr, 2);
  byte response = Wire.read();
  byte terminator = Wire.read();
  if (response = 1)
    return;
  else {
    while (response != 1) {
      delay(50);
      Wire.requestFrom(addr, 2);
      response = Wire.read();
      terminator = Wire.read();
    }
  }  
}


void highpoint_cal() {
  read_until_settled();
  Wire.beginTransmission(addr);
  Wire.write("Cal,high,80000");
  Wire.endTransmission();
  delay(305);
  Wire.requestFrom(addr, 2);
  byte response = Wire.read();
  byte terminator = Wire.read();
  if (response = 1)
    return;
  else {
    while (response != 1) {
      delay(50);
      Wire.requestFrom(addr, 2);
      response = Wire.read();
      terminator = Wire.read();
    }
  }
}

/* Requests and returns a single EC reading from the EZO circuit
*/
float read_ec() {
  char ec_data[20];
  char next;
  float ec = 0.0;

  // Request reading
  Wire.beginTransmission(addr);
  Wire.write('R');
  Wire.endTransmission();
  delay(600);
  // Get Reading
  Wire.requestFrom(addr, 20, true);
  byte response = Wire.read();
  int i = 0;
  switch (response) {
    case 1: // Data ready
      while (Wire.available()) {
        next = Wire.read();
        if (next == 0) {
          Wire.endTransmission();
          break;
        }
        ec_data[i++] = next;
      }
      ec = atof(ec_data);
      break;
    case 2: // Failed reading/syntax error
      ec = -1;
      break;
    case 254: // Data pending
      delay(100);
      Wire.requestFrom(addr, 20, true);
      response = Wire.read();
      while (Wire.available()) {
        next = Wire.read();
        if (next == 0) {
          Wire.endTransmission();
          break;
        }
        ec_data[i++] = next;
      }
      ec = atof(ec_data);
      break;
    case 255: // No data
      ec = -1;
      break;
  };
  return ec;
}

/* Reads from ec probe until the readings reach equilibrium.
    Returns the last reading before settling is reached.
*/
float read_until_settled() {
  // Request Reading
  float ec0 = read_ec();
  float ec1 = read_ec();
  float ec2 = read_ec();
  float dec = finite_diff(ec2, ec1, ec0, 0.6);
  float tol;
  if (ec2 < 100)
    tol = 0.01/0.6;
  else if (ec2 < 1000)
    tol = 0.1/0.6;
  else if (ec2 < 10000)
    tol = 1.0/0.6;
  else if (ec2 <= 99990)
    tol = 10.0/0.6;
  else if (ec2 <= 999900)
    tol = 100.0/0.6;
    
  while (fabs(dec) > tol*1.01) {
    ec0 = ec1;
    ec1 = ec2;
    ec2 = read_ec();
    dec = finite_diff(ec2, ec1, ec0, 0.95);
  }
  return ec2;
}

/* Puts EZO circuit into sleep mode until another command is recieved.
*/
void sleep_mode() {
  Wire.beginTransmission(addr);
  Wire.write("Sleep");
  Wire.endTransmission();
}

/* Returns the numerical derivative of the 3 data points using backward
    finite difference method O(h^2).
    fx3 = f(x i)
    fx2 = f(x i-1)
    fx1 = f(x i-2)
*/
float finite_diff(float fx3, float fx2, float fx1, float h) {
  return (3 * fx3 - 4 * fx2 + fx1) / h;
}

