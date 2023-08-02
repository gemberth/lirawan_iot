// #include <LoRaNode.h>
// #include <config.h>

// #include <LoRa.h>
// #include <SPI.h>
#include <LoRaWan_APP.h>

#include "Arduino.h"

#include <Adafruit_Sensor.h>
#include "DHT.h"

//Libraries for BME280
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
//BME280 definition
// #define SDA 21
// #define SCL 13
TwoWire I2Cone = TwoWire(1);
Adafruit_BME280 bme;
float ValHum = 0;
float ValTem = 0;
float ValPressure = 0;

// sensores de humedad config
// YL-69 sensor pins
const int sensor1 = 36;
const int sensor2 = 37;
const int sensor3 = 38;
int sensor1Value = 0;
int sensor2Value = 0;
int sensor3Value = 0;
int sensor4Value = 0;



// #define DHTPIN    13

// #define DHTTYPE DHT22       // DHT 22  (AM2302), AM2321
// DHT dht(DHTPIN, DHTTYPE);
// float ValHum,ValTem = 0;


#define RF_FREQUENCY                                433000000 // Hz

#define TX_OUTPUT_POWER                             5        // dBm

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false


#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30 // Define the payload size here

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

double txNumber;

bool lora_idle=true;

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );
// bmo config---------------------------------------------------
// void startBME(){
//   I2Cone.begin(SDA, SCL, 100000); 
//   bool status1 = bme.begin(0x76, &I2Cone);  
//   if (!status1) {
//     Serial.println("Could not find a valid BME280_1 sensor, check wiring!");
//     while (1);
//   }
// }
void startBME(){
  bool status = bme.begin(0x76);
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring or change I2C address!");
    while (1);
  }
}


void getReadings(){
  ValTem = bme.readTemperature();
  ValHum = bme.readHumidity();
  ValPressure = bme.readPressure() / 100.0F;
}
//-------------------------------------------------------------------------------

// nuevos sensores --------------------------------------------------
// void getReadings() {
//   sensor1Value = analogRead(sensor1);
//   sensor2Value = analogRead(sensor2);
//   sensor3Value = analogRead(sensor3);
// }


void setup() {
    Serial.begin(115200);
    pinMode(sensor1, INPUT_PULLDOWN);
    pinMode(sensor3, INPUT_PULLDOWN);
    pinMode(sensor2, INPUT_PULLDOWN);
    Mcu.begin();
    startBME();

    
	
    txNumber=0;

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 3000 ); 

    // dht.begin();
    

   }



void loop()
{
  sensor1Value = analogRead(sensor1);
  sensor2Value = analogRead(sensor2);
  sensor3Value = analogRead(sensor3);
	if(lora_idle == true)
	{
    delay(10000);

     //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    //  ValHum = dht.readHumidity();
    //   ValTem = dht.readTemperature();

    ValTem = bme.readTemperature();
    ValHum = bme.readHumidity();
    ValPressure = bme.readPressure() / 100.0F;            
      if (isnan(ValHum) || isnan(ValTem)) {
        ValHum = 0;
        ValTem = 0;
        ValPressure=0;
        Serial.println(F("Error de lectura del sensor DHT22!"));
      }

    // int sensor1 =  // Pin analógico A0 en la placa Heltec ESP32 LoRa
    // int sensor2 =  // Pin analógico A3 en la placa Heltec ESP32 LoRa
    // int sensor3 = 
    // int sensor1Porcentaje =(isnan(sensor1Value)) ? 0 : (100 - (sensor1Value * 100 / 4095.0)); 
    // int sensor2Porcentaje = (isnan(sensor2Value)) ? 0 : (100 - (sensor2Value * 100 / 4095.0)); 
    // int sensor3Porcentaje = (isnan(sensor3Value)) ? 0 : (100 - (sensor3Value * 100 / 4095.0)); 
    float sensor1Porcentaje =( 100 - ( (sensor1Value/4095.00) * 100 ) );
    float sensor2Porcentaje = ( 100 - ( (sensor2Value/4095.00) * 100 ) );
    float sensor3Porcentaje = ( 100 - ( (sensor3Value/4095.00) * 100 ) ); 
    float mm = 11.11; 
          
      Serial.print("Humedad: ");Serial.print(ValHum);Serial.print("%  Temperatura: ");
      Serial.print(ValTem);Serial.println("°C");
      Serial.print("Presion: ");Serial.print(ValPressure);
      // Serial.print("sensor1Porcentaje: ");Serial.print(sensor1Porcentaje);
      // Serial.print("sensor2Porcentaje: ");Serial.print(sensor2Porcentaje);
      // Serial.print("sensor3Porcentaje: ");Serial.print(sensor3Porcentaje);

      //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
		  // sprintf(txpacket,"Hmd@%0.2f@Tmp@%0.2f@Pre@%0.2f",ValHum,ValTem,ValPressure);  //start a package
		  sprintf(txpacket,"h@%0.2f@%0.2f@%0.2f@%0.2f@%0.2f@%0.2f@p",ValHum,ValTem,ValPressure,sensor1Porcentaje,sensor2Porcentaje,sensor3Porcentaje);  //start a package
		  Serial.printf("\r\nEnviando Paquete \"%s\" , longitud %d\r\n",txpacket, strlen(txpacket));
		  Radio.Send( (uint8_t *)txpacket, strlen(txpacket) ); //send the package out	
      lora_idle = false;
	}
  Radio.IrqProcess( );
}

void OnTxDone( void )
{
	Serial.println("TX done......");
	lora_idle = true;
}

void OnTxTimeout( void )
{
    Radio.Sleep( );
    Serial.println("TX Timeout......");
    lora_idle = true;
}