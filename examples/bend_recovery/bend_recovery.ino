/* 
 *  Attempt to recover a sensor that is in an unknown state or update the firmware.
 *  By: Nick Rudh @ Nitto Bend Technologies
 *  Date: 1/15/2025
 *  
 * This software is provided "as is", without any warranty of any kind, express or implied,
 * including but not limited to the warranties of merchantability, fitness for a particular purpose,
 * and noninfringement. In no event shall the authors or copyright holders be liable for any claim,
 * damages, or other liability, whether in an action of contract, tort, or otherwise, arising from,
 * out of, or in connection with the software or the use or other dealings in the software.
 */

#include <Wire.h>
#include "ads_two_axis_fw_v2.h"

#define ADS_RESET_PIN      (3)           // Pin number attached to ads reset line.
#define ADS_INTERRUPT_PIN  (4)           // Not needed in polled mode.
#define SENSOR_ADDRESS    0x12

const uint32_t firmwareSize = sizeof(ads_two_axis_fw_v2); 

uint8_t writeToI2CDevice(uint8_t address, uint8_t* buffer, size_t size);
bool waitForAck(uint8_t address, char expectedAck, unsigned long timeout);
int scanI2CBus();
int uploadFirmware();
bool getUserConfirmation(const char* prompt);

void setup(){
  Serial.begin(115200);

  if (!getUserConfirmation("Do you want to attempt to recover/update the sensor? (y/n)")) {
    return;
  }

  pinMode(ADS_RESET_PIN, OUTPUT);
  pinMode(ADS_INTERRUPT_PIN, OUTPUT);

  Wire.begin();

  // Begin the recovery sequence
  digitalWrite(ADS_RESET_PIN, LOW);  // Put sensor in reset
  delay(100);  // Hold reset for a short period
  digitalWrite(ADS_INTERRUPT_PIN, LOW);  // Hold dataReady low
  digitalWrite(ADS_RESET_PIN, HIGH);  // Release reset
  delay(1000);  // Wait for the sensor to initialize
  pinMode(ADS_INTERRUPT_PIN, INPUT);
  digitalWrite(ADS_INTERRUPT_PIN, HIGH);

  // Send firmware size to the sensor
  uint8_t packet[4];
  packet[0] = (uint8_t)(firmwareSize & 0xff);
  packet[1] = (uint8_t)((firmwareSize >> 8) & 0xff);
  packet[2] = (uint8_t)((firmwareSize >> 16) & 0xff);
  packet[3] = (uint8_t)((firmwareSize >> 24) & 0xff);

  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(packet, 4);
  byte error = Wire.endTransmission();

  // Check if the sensor acknowledged the data
  if (error != 0) {
    Serial.println("Problem Recovering Sensor.");
    return;
  }else{
    if (!waitForAck(SENSOR_ADDRESS, 's', 2000)) {
      Serial.println("Sensor failed to ack Firmware Size.");
      return;
    }
  }

  Serial.println("Sensor acknowledged the firmware size.");
  if (!getUserConfirmation("Do you want to proceed with the recovery? (y/n)")) {
    Serial.println("Recovery aborted.");
    return;
  }

  // Recovery section
  if (uploadFirmware() != 0) {
    Serial.println("Firmware upload failed.");
    return;
  }
  Serial.println("Firmware upload successful.");

  delay(1000);
  // Reset device
  digitalWrite(ADS_RESET_PIN, LOW);  // Put sensor in reset
  delay(100);
  digitalWrite(ADS_RESET_PIN, HIGH);  // Release reset
  delay(1000);

  int currentAddress = scanI2CBus();
  if (currentAddress != -1) {
    Serial.print("Device found at address 0x");
    Serial.println(currentAddress, HEX);
  } else {
    Serial.println("Device not found on I2C bus.");
    return;
  }
}

void loop() {
  // Nothing to do here
}

bool waitForAck(uint8_t address, char expectedAck, unsigned long timeout) {
  unsigned long startTime = millis();
  while (millis() - startTime < timeout) {
    Wire.requestFrom(address, (uint8_t)1);
    if (Wire.available()) {
      char ack = Wire.read();
      if (ack == expectedAck) {
        return true;
      }
    }
  }
  return false;
}


uint8_t writeToI2CDevice(uint8_t address, uint8_t* buffer, size_t size) {
  Wire.beginTransmission(address); // Begin transmission to the specified address
  Wire.write(buffer, size); // Write the buffer to the I2C device
  uint8_t error = Wire.endTransmission(); // End transmission and get error code

  return error; // Return the error code (0 for no error)
}

int scanI2CBus() {
  uint8_t data[5] = {0x0A}; // Example data buffer
  size_t dataSize = sizeof(data) / sizeof(data[0]); // Size of the data buffer
  Serial.println("Scanning I2C bus...");
  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    Wire.write(data, dataSize);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      Serial.println(address, HEX);
      return address;
    }
  }
  Serial.println("No I2C devices found.");
  return -1;
}

int uploadFirmware() {
  uint8_t block_len = 64;
  uint32_t nb_blocks = firmwareSize / block_len;
  uint32_t rem_data = firmwareSize % block_len;
  uint8_t packet[block_len];

  for (uint32_t i = 0; i < nb_blocks; i++) {
    // Copy the next page
    memcpy(packet, &ads_two_axis_fw_v2[i * block_len], block_len);

    // Send the page
    writeToI2CDevice(SENSOR_ADDRESS, packet, block_len / 2);
    writeToI2CDevice(SENSOR_ADDRESS, &packet[block_len / 2], block_len / 2);

    // Get acknowledgement of the received page
    if (!waitForAck(SENSOR_ADDRESS, 's', 500)) {
      return -1;  // Timeout or error
    }
  }

  // Transfer the remainder and get acknowledgement
  memcpy(packet, &ads_two_axis_fw_v2[nb_blocks * block_len], rem_data);

  if (rem_data > block_len / 2) {
    writeToI2CDevice(SENSOR_ADDRESS, packet, block_len / 2);
    writeToI2CDevice(SENSOR_ADDRESS, &packet[block_len / 2], rem_data - block_len / 2);
  } else {
    writeToI2CDevice(SENSOR_ADDRESS, packet, rem_data);
  }

  if (!waitForAck(SENSOR_ADDRESS, 's', 500)) {
    return -1;  // Timeout or error
  }

  return 0;  // Success
}

bool getUserConfirmation(const char* prompt) {
  Serial.println(prompt);
  while (true) {
    if (Serial.available()) {
      char response = Serial.read();
      if (response == 'y' || response == 'Y') {
        Serial.flush();
        return true;
      } else if (response == 'n' || response == 'N') {
        Serial.flush();
        return false;
      }
    }
  }
}