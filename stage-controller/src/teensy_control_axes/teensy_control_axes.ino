// written for use on Teensy 4.1
#include <SPI.h>  // include the SPI library:

const int CS_X = 36;
const int CS_Y = 37;
const int CS_Z = 38;
const int clkPin = 33;

void setup() {
  Serial.println("\n\nSetting up the TMC4361 axes...\n\n");

  // set the slaveSelectPin as an output:
  pinMode (CS_X, OUTPUT);
  pinMode (CS_Y, OUTPUT);
  pinMode (CS_Z, OUTPUT);

  digitalWrite(CS_X,HIGH);  // make sure X axis is OFF
  digitalWrite(CS_Y,HIGH);  // make sure Y axis is OFF
  digitalWrite(CS_Z,HIGH);  // make sure Z axis is OFF

  int32_t ext_clock_freq = 18750000;
  analogWriteFrequency(clkPin, ext_clock_freq);  // try to get 18.75MHz clock signal out for TMC4361
  analogWrite(clkPin, 128);
  Serial.println("External clock signal for TMC4631 set...");

  delay(500);  // wait a bit for the TMC4361 to figure itself out.

  SPISettings spi_settings(2000000, MSBFIRST, SPI_MODE3); 

  // initialize SPI:
  SPI1.setMISO(39);
  SPI1.begin(); 
  SPI1.setMISO(39);
  SPI1.beginTransaction(spi_settings);
  Serial.println("\n\nSPI hardware initialised...\n\n");
  
  check_SPI(CS_X);
  check_SPI(CS_Y);
  check_SPI(CS_Z);

  int CS_ACTIVE = CS_X;

  // set up motion axis X
  configure_axis(CS_ACTIVE);

  // write nonzero target position
  // byte target_data[] = {0x37, 0b00000000, 0x00, 0x04, 0xFF};  // signed 32 bit (so max 2^31)
  // write_spi(CS_ACTIVE, target_data);
  set_position(CS_ACTIVE, 1200);

  for (int i = 1; i < 10; i++) {
    print_position(CS_ACTIVE);
    delay(200);
  }

  // write nonzero target position (warning: stage moves)
  set_position(CS_ACTIVE, -1400);

  for (int i = 1; i < 40; i++) {
    print_position(CS_ACTIVE);
    delay(200);
  }
}

void loop() {
  
  delay(5000);
}

void configure_axis(int CS_pin) {
  /// Set up axis to the point where the move commands can be called

  // Set up the correct external clock speed
  // important to have velocity be in "physically accurate" steps/time 
  write_spi(CS_pin, 0x31, 0x11E11A30);  // see ext_clock_freq. Claimed to be 24:0 but that cannot be right? doesnt fit values

  // zero position position and target
  write_spi(CS_pin, 0x21, 0);  // position
  write_spi(CS_pin, 0x37, 0);  // target

  // extend the pulsewidth for the step/dir signals
  write_spi(CS_pin, 0x10, 0x00000000 + 0b00001111);  // set STEP duration to be 15 clock cycles, instead of just 1 

  // write ramp mode
  write_spi(CS_pin, 0x20, 0x00000000 + 0b00000101); // POSITION mode, with trapezoidal ramp

  // set max acceleration/deceleration
  write_spi(CS_pin, 0x28, 0x0000FFFF); // 23:0 are relevant
  write_spi(CS_pin, 0x29, 0x0000FFFF);

  // set max velocity
  write_spi(CS_pin, 0x24, 0x0002FF00); // 31:0 are relevant, 23 digits, 8 decimal places

  delay(200);  // can be removed, but means the POS=TARGET led will be lit for a bit
}

signed long get_position(int CS_pin) {
  return read_spi(CS_pin, 0x21);
}

void print_position(int CS_pin) {
  Serial.printf("Position is: %ld\n", get_position(CS_pin));
  Serial.printf("Position is: %x\n", get_position(CS_pin));
}

void set_position(int CS_pin, signed long position) {
  Serial.printf("> Moving axis %d to position %ld...\n",  CS_pin, position);
  write_spi(CS_pin, 0x37, position);
}

void check_SPI(int CS_pin) {
  /// checks if the TMC4361 at the CS_pin is giving the expected response.
  /// 
  /// It is recommend that you call this function before any SPI communication as the first response
  /// over SPI is nonsense or something. Anyway, this should make sure SPI is ready and working before
  /// you start going serious work over that serial interface.

  unsigned long response = read_spi(CS_pin, 0x00);  // read the general register (0x00)
  
  if (response == 0x00006020) {
    printf("SPI communication with CS %d is OK!\n", CS_pin);
  } else {
    printf("Did not get expected response over SPI for CS %d\n", CS_pin);
  }
}

unsigned long read_spi(int CS_pin, byte spi_register) {
  // first we must write the register we want to read
  digitalWrite(CS_pin,LOW);  

  SPI1.transfer(spi_register);
  for (int i = 1; i < 5; i++) {
    SPI1.transfer(0x00);
  } 

  // take the SS pin high to de-select the chip:
  digitalWrite(CS_pin,HIGH); 

  // and now we can read out the actual values. We send the same query, but we could pick any valid register.
  return read_spi_single(CS_pin, spi_register);
}

unsigned long read_spi_single(int CS_pin, byte spi_register) {

  unsigned long response;

  digitalWrite(CS_pin,LOW);  
  delayMicroseconds(10);

  SPI1.transfer(spi_register);
  for (int i = 1; i < 4; i++) {
    response |= SPI1.transfer(0x00);
    //Serial.printf("got response (same cycle): %x\n", response);
    response <<= 8;
  } 
  response |= SPI1.transfer(0x00);  // dont need a bit shift after last byte

  // take the SS pin high to de-select the chip:
  digitalWrite(CS_pin,HIGH);

  return response;
}

void write_spi(int CS_pin, byte address, unsigned long data) {
  //TMC4361 takes 5-byte bit data: 8 address and 32 data

  //Serial.printf("\nWriting SPI data %x to register %x\n", data, address);

  digitalWrite(CS_pin,LOW);
  delayMicroseconds(10);

  // to send write request we add 0b10000000 to the equivalent read address
  SPI1.transfer(address + 0x80);  // address
  SPI1.transfer((data >> 24) & 0xff);
  SPI1.transfer((data >> 16) & 0xff);
  SPI1.transfer((data >> 8) & 0xff);
  SPI1.transfer((data) & 0xff);
  digitalWrite(CS_pin,HIGH);

  // and we read back the value to check that it was set correctly
  unsigned long readback = read_spi_single(CS_pin, address);

  // compare values
  if (readback != data) {
    Serial.printf("> SPI write mismatch: read %x but expected %x \n", readback, data);
  }
}