#include "Arduino.h"
#include "ads_two_axis.h"

#include <bluefruit.h>
#include <string.h>

#define ADS_RESET_PIN       (27)        // Pin number attached to ads reset line.
#define ADS_INTERRUPT_PIN   (30)        // Pin number attached to the ads data ready line.  


BLEService        angms = BLEService(0x1820);
BLECharacteristic angmc = BLECharacteristic(0x2A70);


BLEDis bledis;    // DIS (Device Information Service) helper class instance
BLEBas blebas;    // BAS (Battery Service) helper class instance


// function prototypes
void startAdv(void);
void setupANGM(void);
void connect_callback(uint16_t conn_handle);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);
void ads_data_callback(float * sample);
void deadzone_filter(float * sample);
void signal_filter(float * sample);
void parse_serial_port(void);

float ang[2];
volatile bool newData = false;
ads_init_t ads_init;

void signal_filter(float * sample)
{
    static float filter_samples[2][6];

    for(uint8_t i=0; i<2; i++)
    {
      filter_samples[i][5] = filter_samples[i][4];
      filter_samples[i][4] = filter_samples[i][3];
      filter_samples[i][3] = (float)sample[i];
      filter_samples[i][2] = filter_samples[i][1];
      filter_samples[i][1] = filter_samples[i][0];
  
      // 20 Hz cutoff frequency @ 100 Hz Sample Rate
      filter_samples[i][0] = filter_samples[i][1]*(0.36952737735124147f) - 0.19581571265583314f*filter_samples[i][2] + \
        0.20657208382614792f*(filter_samples[i][3] + 2*filter_samples[i][4] + filter_samples[i][5]);   

      sample[i] = filter_samples[i][0];
    }
}

void deadzone_filter(float * sample)
{
  static float prev_sample[2];
  float dead_zone = 0.5f;

  for(uint8_t i=0; i<2; i++)
  {
    if(fabs(sample[i]-prev_sample[i]) > dead_zone)
      prev_sample[i] = sample[i];
    else
      sample[i] = prev_sample[i];
  }
}

void ads_data_callback(float * sample)
{
  // Low pass IIR filter
  signal_filter(sample);

  // Deadzone filter
  deadzone_filter(sample);
  
  ang[0] = sample[0];
  ang[1] = sample[1];
  
  newData = true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Two Axis ADS BLE Example");
  Serial.println("-----------------------\n");

  Bluefruit.begin();
  
  delay(100);
  
  Serial.println("Initializing the Two Axis Angular Displacement Sensor");

  ads_init.sps = ADS_100_HZ;
  ads_init.ads_sample_callback = &ads_data_callback;
  ads_init.reset_pin = ADS_RESET_PIN;                 // Pin connected to ADS reset line
  ads_init.datardy_pin = ADS_INTERRUPT_PIN;           // Pin connected to ADS data ready interrupt

  // Initialize ADS hardware abstraction layer, and set the sample rate
  int ret_val = ads_two_axis_init(&ads_init);

  if(ret_val == ADS_OK)
  {
    Serial.println("Two Axis ADS initialization succeeded");
  }
  else
  {
    Serial.print("Two Axis ADS initialization failed with reason: ");
    Serial.println(ret_val);
  }
  
  // Set the advertised device name
  Serial.println("Setting Device Name to 'two_axis_ads'");
  Bluefruit.setName("two_axis_ads");


  // Set the connect/disconnect callback handlers
  Bluefruit.setConnectCallback(connect_callback);
  Bluefruit.setDisconnectCallback(disconnect_callback);

  Bluefruit.setConnIntervalMS(7.5,200);

  // Configure and Start the Device Information Service
  //Serial.println("Configuring the Device Information Service");
  bledis.setManufacturer("Bend Labs");
  bledis.setModel("Two Axis Demo");
  bledis.setHardwareRev("REV: 0.0.0");
  bledis.setSoftwareRev("SD: 132.2.0.0");
  bledis.begin();

  // Start the BLE Battery Service and set it to 100%
  //Serial.println("Configuring the Battery Service");
  blebas.begin();
  blebas.write(100);

  // Setup the Angle Measurement service using
  // BLEService and BLECharacteristic classes
  //Serial.println("Configuring the Heart Rate Monitor Service");
  setupANGM();

  // Setup the advertising packet(s)
  //Serial.println("Setting up the advertising payload(s)");
  startAdv();
  
  Serial.println("\nAdvertising"); 
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  //Bluefruit.Advertising.addTxPower();

  // Include Angle Measurement Service UUID
  Bluefruit.Advertising.addService(angms);

  // Include Name
  Bluefruit.Advertising.addName();

  Bluefruit.Advertising.addAppearance(113);
  
  /* Start Advertising
   * - Enable auto advertising if disconnected
   * - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
   * - Timeout for fast mode is 30 seconds
   * - Start(timeout) with timeout = 0 will advertise forever (until connected)
   * 
   * For recommended advertising interval
   * https://developer.apple.com/library/content/qa/qa1931/_index.html   
   */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(500,800);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

void setupANGM(void)
{
  // Configure the Angle Measurement service
  // Supported Characteristics:
  // Name                         UUID    Requirement Properties
  // ---------------------------- ------  ----------- ----------
  // Angle Measurement            0x2A70  Mandatory   Notify/Write    
  angms.begin();

  // Configure the Angle Measurement characteristic
  // Properties = Notify
  // Min Len    = 4
  // Max Len    = 4
  // Little Endian Float
  angmc.setProperties(CHR_PROPS_NOTIFY|CHR_PROPS_WRITE);
  angmc.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  angmc.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
  angmc.setWriteCallback(write_callback);
  angmc.begin();
  uint8_t ang_initial[] = {0,0,0,0,0,0,0,0};
  angmc.notify(ang_initial, 8);                   // Use .notify instead of .write!
}

void connect_callback(uint16_t conn_handle)
{
    Serial.print("Connected");
    ads_two_axis_run(true);
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{

  Serial.println(reason);
  (void) conn_handle;
  (void) reason;

  Serial.println("Disconnected");
  Serial.println("Advertising!");

  ads_two_axis_run(false);
}

void write_callback(BLECharacteristic& chr, unsigned char * rx, short unsigned len, short unsigned dah)
{
  if(len == 1)
  {
    if(rx[0] == 0)
    {
      ads_two_axis_calibrate(ADS_CALIBRATE_FIRST, 0);
    }
    else if(rx[0] == 1)
    {
      ads_two_axis_calibrate(ADS_CALIBRATE_FLAT, 90);
    }
    else if(rx[0] == 2)
    {
      ads_two_axis_calibrate(ADS_CALIBRATE_PERP, 90);
    }
    else if(rx[0] == 3)
    {
      ads_two_axis_calibrate(ADS_CALIBRATE_CLEAR, 0);
    }
    else if(rx[0] == 0x07)
    {
      NVIC_SystemReset();
    }
  }
  else if(len == 2)
  {
    uint16_t sps = ads_uint16_decode(rx);
    
    ads_two_axis_set_sample_rate((ADS_SPS_T)sps);
  }
}

void cccd_callback(BLECharacteristic& chr, uint16_t cccd_value)
{
    // Display the raw request packet
    //Serial.print("CCCD Updated: ");
    //Serial.printBuffer(request->data, request->len);
    //Serial.print(cccd_value);
    //Serial.println("");

    // Check the characteristic this CCCD update is associated with in case
    // this handler is used for multiple CCCD records.
    if (chr.uuid == angmc.uuid) {
        if (chr.notifyEnabled()) {
            Serial.println("Angle Measurement 'Notify' enabled");
        } else {
            Serial.println("Angle Measurement 'Notify' disabled");
        }
    }
}

void parse_serial_port(void)
{
    char key = Serial.read();
    
    if(key == '0')
      ads_two_axis_calibrate(ADS_CALIBRATE_FIRST, 0);
    else if(key == 'f')
      ads_two_axis_calibrate(ADS_CALIBRATE_FLAT, 90);
    else if(key == 'p')
      ads_two_axis_calibrate(ADS_CALIBRATE_PERP, 90);
    else if(key == 'c')
      ads_two_axis_calibrate(ADS_CALIBRATE_CLEAR, 0);
    else if(key == 'r')
      ads_two_axis_run(true);
    else if(key == 's')
      ads_two_axis_run(false);
    else if(key == 'f')
      ads_two_axis_set_sample_rate(ADS_200_HZ);
    else if(key == 'u')
      ads_two_axis_set_sample_rate(ADS_10_HZ);
    else if(key == 'n')
      ads_two_axis_set_sample_rate(ADS_100_HZ);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  if ( newData ) 
  {
    if(Bluefruit.connected())
    {
      uint8_t ang_encoded[8];
      memcpy(&ang_encoded[0], &ang[0], sizeof(float));
      memcpy(&ang_encoded[4], &ang[1], sizeof(float));
      angmc.notify(ang_encoded, sizeof(ang_encoded));
    }
    newData = false;

    Serial.print(ang[0]); 
    Serial.print(","); 
    Serial.println(ang[1]);
  }

  if(Serial.available())
  {
    parse_serial_port();
  } 
  
  delay(1);
}

void rtos_idle_callback(void)
{
  // Don't call any other FreeRTOS blocking API()
  // Perform background task(s) here
}
