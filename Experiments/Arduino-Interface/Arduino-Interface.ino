#include <Wire.h>

// Definition of message bytes
const byte START_BYTE = 1;
const byte STOP_BYTE = 27;
const byte PH_UP_BYTE = 21;
const byte PH_DOWN_BYTE = 22;
const byte EC_G_BYTE = 23;
const byte EC_V_BYTE = 24;
const byte TIME_ID_BYTE = 't';
const byte INPUT_ID_BYTE = 'i';
const byte NO_INPUT_ID_BYTE = 'n';
const byte ACK_BYTE = 6;

// Input type enumeration
enum Input {PH_UP = 21, PH_DOWN = 22, EC_G = 23, EC_V = 24};
enum Probe {PH, EC};

// Function Prototypes
void write_input(Input input, bool state);
float get_measurement(Probe probe);
uint16_t get_duration();
Input get_input();
void send_data(uint16_t t, float data, bool input);
float read_ph();
float read_ec();

// Global Variables
uint16_t pre_duration = 2000;   // Duration of data sampling before input
uint16_t post_duration = 10000; // Duration of data sampling after input
const int ph_addr = 0x63;
const int ec_addr = 0x64;

void setup() {
  Serial.begin(9600);
  Wire.begin();
}

void loop() {
  if (Serial.available() >= 4) {
    byte s = Serial.read();
    if (s == START_BYTE) {
      // Read input byte
      Input input_type = get_input();
      // Read duration lsb and msb, combine
      uint16_t duration = get_duration();

      Probe probe;
      if (input_type == PH_UP || input_type == PH_DOWN)
        probe = PH;
      else
        probe = EC;
      Serial.write(ACK_BYTE);
      uint16_t t0 = millis();
      uint16_t t = 0;
      uint16_t t_last = t0;
      uint16_t measurement = 0;

      // Send measurements before input is activated
      while (millis() - t0 <= pre_duration) {
        measurement = get_measurement(probe);
        t_last = millis();
        t = t_last - t0;
        send_data(t, measurement, 0);
      }
      // Send measurements while input is active
      write_input(input_type, true);
      uint16_t t1 = millis();
      while (millis() - t1 <= duration) {
        measurement = get_measurement(probe);
        t_last = millis();
        t = t_last - t0;
        send_data(t, measurement, 1);
      }
      // Send measurements after input is deactivated
      write_input(input_type, false);
      uint16_t t2 = millis();
      while (millis() - t2 <= post_duration) {
        measurement = get_measurement(probe);
        t_last = millis();
        t = t_last - t0;
        send_data(t, measurement, 0);
      }
      Serial.write(STOP_BYTE);
    }
  }
}


/* -------- FUNCTION DEFINITIONS --------- */

/* Activates/Deactivates the given input */
void write_input(Input input, bool state) {
  digitalWrite(input, state);
}

/* Gets measurement from the requested probe
    TODO: Implement measurement collection */
float get_measurement(Probe probe) {
  float measurement = 0;
  if (probe == PH) {
    measurement = read_ph();
  }
  else if (probe == EC) {
    measurement = read_ec();
  }
  return measurement;
}

/* Interprets the next two bytes in the serial buffer as a 16-bit number
  representing a duration in milliseconds. */
uint16_t get_duration() {
  uint16_t lsb = Serial.read();
  uint16_t msb = Serial.read();
  uint16_t duration = (msb << 8) | lsb;
  return duration;
}

/* Interprets the next byte as an input and returns the input type */
Input get_input() {
  byte i = Serial.read();
  Input input = 0;
  switch (i) {
    case PH_UP_BYTE:
      input = PH_UP;
      break;
    case PH_DOWN_BYTE:
      input = PH_DOWN;
      break;
    case EC_G_BYTE:
      input = EC_G;
      break;
    case EC_V_BYTE:
      input = EC_V;
      break;
    default:
      input = 3;
  };
  return input;
}

/* Sends an idenification byte as to whether the input is on, then sends
    the data in two bytes, LSB first */
void send_data(uint16_t t, float data, bool input) {
  byte lsb, msb, b[4];
  union Swap {
    float data;
    uint32_t raw;
  };

  unsigned char ascii[32];
  // Send input ID
  if (input)
    Serial.write(INPUT_ID_BYTE);
  else
    Serial.write(NO_INPUT_ID_BYTE);

  // Send time stamp
  lsb = 0xFF & t;
  msb = t >> 8;
  Serial.write(lsb);
  Serial.write(msb);

  // Convert float to unsigned 32 int so we can send it byte by byte
  Swap swap = {data};
  for (int i = 0; i < 4; i++)
    Serial.write((swap.raw >> i * 8) & 0xFF);
}

/* Requests and returns a single pH reading from the EZO circuit
*/
float read_ph() {
  char ph_data[20];
  char next;
  float ph = 0.0;

  // Request reading
  Wire.beginTransmission(ph_addr);
  Wire.write('R');
  Wire.endTransmission();
  delay(950);
  // Get Reading
  Wire.requestFrom(ph_addr, 20, true);
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
      Wire.requestFrom(ph_addr, 20, true);
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

/* Requests and returns a single EC reading from the EZO circuit
*/
float read_ec() {
  char ec_data[20];
  char next;
  float ec = 0.0;

  // Request reading
  Wire.beginTransmission(ec_addr);
  Wire.write('R');
  Wire.endTransmission();
  delay(600);
  // Get Reading
  Wire.requestFrom(ec_addr, 20, true);
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
      Wire.requestFrom(ec_addr, 20, true);
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

