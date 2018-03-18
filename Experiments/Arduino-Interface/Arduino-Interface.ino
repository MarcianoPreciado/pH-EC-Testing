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
uint16_t get_measurement(Probe probe);
uint16_t get_duration();
Input get_input();
void send_data(uint16_t t, uint16_t data, bool input);




// Global Variables
int f = 20;                     // Sampling frequency
uint16_t pre_duration = 2000;   // Duration of data sampling before input
uint16_t post_duration = 10000; // Duration of data sampling after input

void setup() {
  Serial.begin(9600);
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
        // Send samples at f Hz
        if (millis() - t_last > 1000 / f) {
          measurement = get_measurement(probe);
          t_last = millis();
          t = t_last - t0;
          send_data(t, measurement, 0);
        }
      }
      // Send measurements while input is active
      write_input(input_type, true);
      uint16_t t1 = millis();
      while (millis() - t1 <= duration) {
        // Send samples at f Hz
        if (millis() - t_last > 1000 / f) {
          measurement = get_measurement(probe);
          t_last = millis();
          t = t_last - t0;
          send_data(t, measurement, 1);
        }
      }
      // Send measurements after input is deactivated
      write_input(input_type, false);
      uint16_t t2 = millis();
      while (millis() - t2 <= post_duration) {
        // Send samples at f Hz
        if (millis() - t_last > 1000 / f) {
          measurement = get_measurement(probe);
          t_last = millis();
          t = t_last - t0;
          send_data(t, measurement, 0);
        }
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
uint16_t get_measurement(Probe probe) {
  uint16_t measurement = 0;
  if (probe == PH) {
    measurement = 200;
  }
  else if (probe == EC) {
    measurement = 100;
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
void send_data(uint16_t t, uint16_t data, bool input) {
  // Send input ID
  if (input)
    Serial.write(INPUT_ID_BYTE);
  else
    Serial.write(NO_INPUT_ID_BYTE);

  // Send time stamp
  byte lsb = 0xFF & t;
  byte msb = t >> 8;
  Serial.write(lsb);
  Serial.write(msb);

  // Send data
  lsb = data & 0xFF;
  msb = data >> 8;
  Serial.write(lsb);
  Serial.write(msb);
}

