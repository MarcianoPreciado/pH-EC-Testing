/* pH Calibration
    author: Marciano C. Preciado
    date: 03-19-2018

    This sketch makes use of the Atlas Scientific pH probe and EZO pH Circuit.
    The communication method with EZO is I2C. The EZO is calibrated using the
    3-point method. This sketch assumes the EZO circuit is already set to I2C mode.

    Instructions (courtesy of Atlas Scientific)
    Midpoint Calibration
      1. Remove soaker bottle and rinse off pH probe.
      2. Pour a small amount of the calibration solution into a cup. (pH 7.00)
      3. Let the probe sit in calibration solution untill readings stabalize (1 – 2 minutes).
      4. Calibrate the midpoint value using the command "Cal,mid,n".
        Where "n" is any floating point value that represents the calibration midpoint.
      5. Do not pour the calibration solution back into the bottle.
    Lowpoint Calibration
      1. Rinse off pH probe.
      2. Pour a small amount of the calibration solution into a cup (pH 4.00)
      3. Let the probe sit in calibration solution untill readings stabalize (1 – 2 minutes).
      4. Calibrate the lowpoint value using the command "Cal,low,n".
         Where "n" is any floating point value that represents the calibration lowpoint.
      5. Do not pour the calibration solution back into the bottle.
    Highpoint Calibration
      1. Rinse off pH probe.
      2. Pour a small amount of the calibration solution into a cup (pH 10.00)
      3. Let the probe sit in calibration solution untill readings stabalize (1 – 2 minutes).
      4. Calibrate the highpoint value using the command "Cal,high,n".
         Where "n" is any floating point value that represents the calibration highpoint.
      5. Do not pour the calibration solution back into the bottle.
*/
#include <Wire.h>

// Default pH EZO address
const int addr = 0x63;
const int led_pin = 13;

// Function Prototypes
void midpoint_cal();
void lowpoint_cal();
void highpoint_cal();
float read_ph();
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
  while (analogRead(A7) > 100); // Wait for button press
  digitalWrite(led_pin, LOW);
  midpoint_cal();
  sleep_mode();
  digitalWrite(led_pin, HIGH);

  while (analogRead(A7) > 100); // Wait for button press for next calibration
  digitalWrite(led_pin, LOW);
  lowpoint_cal();
  sleep_mode();
  digitalWrite(led_pin, HIGH);
  
  while (analogRead(A7) > 100); // Wait for button press for next calibration
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
void midpoint_cal() {
  read_until_settled();
  Wire.beginTransmission(addr);
  Wire.write("Cal,mid,7.00");
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
  Wire.write("Cal,low,4.00");
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
  Wire.write("Cal,high,10.00");
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

/* Requests and returns a single pH reading from the EZO circuit
*/
float read_ph() {
  char ph_data[20];
  char next;
  float ph = 0.0;

  // Request reading
  Wire.beginTransmission(addr);
  Wire.write('R');
  Wire.endTransmission();
  delay(950);
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
        ph_data[i++] = next;
      }
      ph = atof(ph_data);
      break;
    case 2: // Failed reading
      ph = -1;
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
        ph_data[i++] = next;
      }
      ph = atof(ph_data);
      break;
    case 255: // No data
      ph = -1;
      break;
  };
  return ph;
}

/* Reads from pH probe until the readings reach equilibrium.
    Returns the last reading before settling is reached.
*/
float read_until_settled() {
  // Request Reading
  float ph0 = read_ph();
  float ph1 = read_ph();
  float ph2 = read_ph();
  float dph = finite_diff(ph2, ph1, ph0, 0.95);
  while (dph > 0.007) {
    ph0 = ph1;
    ph1 = ph2;
    ph2 = read_ph();
    dph = finite_diff(ph2, ph1, ph0, 0.95);
  }
  return ph2;
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

