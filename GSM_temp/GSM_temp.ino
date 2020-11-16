#define TINY_GSM_MODEM_SIM800
#include <TinyGsmClient.h>
#include <BlynkSimpleSIM800.h>
#include <Wire.h>
#include <OneWire.h> 
#include <DallasTemperature.h>


// TTGO T-Call pin definitions
  #define MODEM_RST            5
  #define MODEM_PWKEY          4
  #define MODEM_POWER_ON       23
  #define MODEM_TX             27
  #define MODEM_RX             26
  #define I2C_SDA              21
  #define I2C_SCL              22
  #define IP5306_ADDR          0x75
  #define IP5306_REG_SYS_CTL0  0x00
  #define BATTERY_MIN 3200
  #define BATTERY_MAX 4350
  // Set serial for debug console (to the Serial Monitor, default speed 115200)
  #define SerialMon Serial
  // Hardware Serial on Mega, Leonardo, Micro
  #define SerialAT Serial1

// SIM Specific Information
  const char apn[]  = "TM";
  const char user[] = "";
  const char pass[] = "";
  TinyGsm modem(SerialAT);

//Blynk Specific information
  #define blynk_server "52.31.240.39"
  #define blynk_port 8080
  // const char auth[] = "iSkEJlD4NCjhvWCQnNTZfsukQV5-7biV"; //Device 1
  const char auth[] = "32LUzJ-2IJcHNn3kzqaJO7EL7Se7Srmr"; //Device 2
  // const char auth[] = "w-Xzo7tWDE89Lmi7F8SECMEQ0RM2nvZu"; //Device 3
  #define BLYNK_PRINT Serial    
  #define BLYNK_HEARTBEAT 30


// Data wire for temperature sensor is plugged into pin 2 on the Arduino 
  #define ONE_WIRE_BUS 14 
  OneWire oneWire(ONE_WIRE_BUS); 
  DallasTemperature sensors(&oneWire);

//Global Variables
float Celcius = 0;
float battery_voltage = 0;
float battery_percent = 0;

void setup()
{
  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);

  // Keep power when running from battery
  Wire.begin(I2C_SDA, I2C_SCL);
  bool   isOk = setPowerBoostKeepOn(1);
  SerialMon.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAIL"));

  // Set-up modem reset, enable, power pins
  pinMode(MODEM_PWKEY, OUTPUT);
  pinMode(MODEM_RST, OUTPUT);
  pinMode(MODEM_POWER_ON, OUTPUT);

  digitalWrite(MODEM_PWKEY, LOW);
  digitalWrite(MODEM_RST, HIGH);
  digitalWrite(MODEM_POWER_ON, HIGH);

  // Set GSM module baud rate and UART pins
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem: ");
  SerialMon.println(modemInfo);

  // Unlock your SIM card with a PIN
  //modem.simUnlock("1234");

   SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork(240000L)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" OK");

  if (modem.isNetworkConnected()) {
    SerialMon.println("Network connected");
  }

  SerialMon.print(F("Connecting to APN: "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn, user, pass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" OK");

  Blynk.begin(auth, modem, apn, user, pass, blynk_server, blynk_port);

  battery_voltage = analogRead(35)*2;
  battery_percent = (battery_voltage-BATTERY_MIN)/(BATTERY_MAX-BATTERY_MIN)*100;
  sensors.begin(); 
}

void loop()
{
  float temp_celcius = 0;
  float temp_voltage = 0;
  int measurement_loop = 10;
  for( uint16_t i = 0 ; i < measurement_loop; i++) {
    // call sensors.requestTemperatures() to issue a global temperature  
    sensors.requestTemperatures(); // Send the command to get temperature readings 
    // You can have more than one DS18B20 on the same bus.  
    // 0 refers to the first IC on the wire 
    temp_celcius = temp_celcius +sensors.getTempCByIndex(0);
    temp_voltage = temp_voltage + analogRead(35)*2;
    delay(200);
  }

  Celcius = temp_celcius/measurement_loop;
  battery_voltage = temp_voltage/measurement_loop; 
  battery_percent = round((battery_voltage-BATTERY_MIN)/(BATTERY_MAX-BATTERY_MIN)*100);
  //Publish to Blynk
  Blynk.virtualWrite(V1, Celcius);
  Blynk.virtualWrite(V2, battery_voltage/1000);
  Blynk.virtualWrite(V3, battery_percent);
  Blynk.run();

  SerialMon.println("Temperature : " + String(Celcius) + "*C\t Voltage is: " + String(battery_voltage/1000) + "\t Charge is: " + String(battery_percent)); 
  delay(60000); //run every 1 minute
}



bool setPowerBoostKeepOn(int en)
{
  Wire.beginTransmission(IP5306_ADDR);
  Wire.write(IP5306_REG_SYS_CTL0);
  if (en) {
    Wire.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
  } else {
    Wire.write(0x35); // 0x37 is default reg value
  }
  return Wire.endTransmission() == 0;
}
