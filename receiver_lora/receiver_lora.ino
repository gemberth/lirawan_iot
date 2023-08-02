#include "LoRaWan_APP.h"
#include "Arduino.h"

//-----------------------------------------
#include "time.h"
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <WiFi.h>
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// // Insert your network credentials
// #define WIFI_SSID "REPLACE_WITH_YOUR_SSID"
// #define WIFI_PASSWORD "REPLACE_WITH_YOUR_PASSWORD"

// // Insert Firebase project API Key
// #define API_KEY "REPLACE_WITH_YOUR_PROJECT_API_KEY"

// // Insert Authorized Email and Corresponding Password
// #define USER_EMAIL "REPLACE_WITH_THE_USER_EMAIL"
// #define USER_PASSWORD "REPLACE_WITH_THE_USER_PASSWORD"

// // Insert RTDB URLefine the RTDB URL
// #define DATABASE_URL "REPLACE_WITH_YOUR_DATABASE_URL"
// Insert your network credentials

// #define WIFI_SSID "VILLAMAR"
// #define WIFI_PASSWORD "1108dayi2000"
#define WIFI_SSID "lorawam"
#define WIFI_PASSWORD "12345678"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAKy23WZonQ_dU9mtUAo9Of-mB3wse_4d8"

// Insert Authorized Email and Corresponding Password
#define USER_EMAIL "villamarpilosolisseth@gmail.com"
#define USER_PASSWORD "123456789"

#define HOSTNAME "https://monitoreo-humedad.web.app/"

// Insert RTDB URLefine the RTDB URL
#define DATABASE_URL "https://monitoreo-humedad-default-rtdb.firebaseio.com"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Database main path (to be updated in setup with the user UID)
String databasePath;
// Database child nodes
String tempPath = "/temperature";
String humPath = "/humidity";
String presPath = "/pressure";
String sensor1Value = "/sensor1Value";
String sensor2Value = "/sensor2Value";
String sensor3Value = "/sensor3Value";
String timePath = "/timestamp";

// Parent Node (to be updated in every loop)
String parentPath;

int timestamp;
FirebaseJson json;

const char* ntpServer = "pool.ntp.org";

float temperature;
float humidity;
float pressure;
float v1;
float v2;
float v3;

unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 1000;
// Initialize WiFi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  Serial.println();
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}


//------------------------------------------------------

#define RF_FREQUENCY                                433000000 // Hz

#define TX_OUTPUT_POWER                             14        // dBm

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

static RadioEvents_t RadioEvents;

int16_t txNumber;

int16_t rssi,rxSize;

bool lora_idle = true;

void setup() {
    Serial.begin(115200);
    Mcu.begin();
    
    txNumber=0;
    rssi=0;
  
    RadioEvents.RxDone = OnRxDone;
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                               LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                               LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                               0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
  initWiFi();
  configTime(0, 0, ntpServer); 
  // Assign the api key (required)
  config.api_key = API_KEY;

  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path
  databasePath = "/UsersData/" + uid + "/readings";

}


void loop()
{
  if(lora_idle)
  {
    lora_idle = false;
    Serial.println("into RX mode");
    Radio.Rx(0);
  }
  //-------------------------------------------------------------------
  // Send new readings to database
    Radio.IrqProcess();
  

}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    rssi=rssi;
    rxSize=size;
    memcpy(rxpacket, payload, size );
    rxpacket[size]='\0';
    Radio.Sleep( );
    Serial.printf("\r\nPaquete Recibido \"%s\" con RSSI %d , LONGITUD %d\r\n",rxpacket,rssi,rxSize);

    
    // byte pos_char[] = {0, 0, 0};
    
    // float f_ValHmd = ValHmd.toFloat();
    // float f_ValTmp = ValTmp.toFloat();
    if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    //byte pos_char[] = {1, 7, 9, 15, 17, 24, 26, 32, 34, 40, 42};
    byte numChar = 1;
      // Convertir el array de int a byte
    int originalArray[] = {1, 7, 9, 15, 17, 24, 26, 32, 34, 40, 42};
    byte pos_char[11];
    for (int i = 0; i < 11; i++) {
      pos_char[i] = (byte)originalArray[i];
    }

   //Hmd@65.20@Tmp@21.10
   //h@71.09@T@26.45@P@998.37@T@88.91@T@98.22@T@100.00
  // 0123456789012345678901234567890123456789012345678
    //         1         2         3         4    
    for(byte i=0; i<rxSize; i ++) 
    {
      //--------------------------------------------------------------------TEM 1 2 3
      // if((rxpacket[i]=='@')&&(numChar == 11)){pos_char[10]= i,numChar=12;}
      // if((rxpacket[i]=='@')&&(numChar == 10)){pos_char[9]= i,numChar=11;}
      // if((rxpacket[i]=='@')&&(numChar == 9)){pos_char[8]= i,numChar=10;}
      // if((rxpacket[i]=='@')&&(numChar == 8)){pos_char[7]= i,numChar=9;}
      // if((rxpacket[i]=='@')&&(numChar == 7)){pos_char[6]= i,numChar=8;}
      // if((rxpacket[i]=='@')&&(numChar == 6)){pos_char[5]= i,numChar=7;}
      // //-----------------------------------------------------------------------------BMO
      // if((rxpacket[i]=='@')&&(numChar == 5)){pos_char[4]= i,numChar=6;}
      // if((rxpacket[i]=='@')&&(numChar == 4)){pos_char[3]= i,numChar=5;}
      // if((rxpacket[i]=='@')&&(numChar == 3)){pos_char[2]= i,numChar=4;}
      // if((rxpacket[i]=='@')&&(numChar == 2)){pos_char[1]= i,numChar=3;}
      // if((rxpacket[i]=='@')&&(numChar == 1)){pos_char[0]= i,numChar=2;}
      //Serial.println("dd");
    }

    // Serial.printf("\r\nnumChar1 \"%d\" numChar2 %d , numChar3 %d\r\n",pos_char[0],pos_char[1],pos_char[2]);
    // Imprime por el puerto serie las posiciones de los caracteres '@' encontrados en la cadena
    // Serial.printf("\r\nnumChar1 \"%d\" numChar2 %d, numChar3 %d, numChar4 %d\r\n", pos_char[0], pos_char[1], pos_char[2], pos_char[3]);
    // Serial.printf("\r\nnumChar1 \"%d\" numChar2 %d, numChar3 %d, numChar4 %d, numChar5 %d\r\n", pos_char[0], pos_char[1], pos_char[2], pos_char[3], pos_char[4]);
    Serial.printf("\r\nnumChar1 \"%d\" numChar2 %d, numChar3 %d, numChar4 %d, numChar5 %d, numChar6 %d, numChar7 %d, numChar8 %d, numChar9 %d, numChar10 %d, numChar11 %d\r\n",
                   pos_char[0], pos_char[1], pos_char[2], pos_char[3], pos_char[4], pos_char[5], pos_char[6], pos_char[7], pos_char[8], pos_char[9], pos_char[10]);


    // String ValHmd;
    // for(byte i=pos_char[0]+1; i<pos_char[1]; i ++) 
    // {
    //  ValHmd = ValHmd + char(rxpacket[i]);
    // }

    // String ValTmp;
    // for(byte i=pos_char[2]+1; i<rxSize; i ++) 
    // {
    //  ValTmp = ValTmp + char(rxpacket[i]);
    // }
    // Extraer ValHmd de rxpacket
    //Hmd@68.23@Tmp@26.06@Pre@997.12@T1@%0.2f@T2@%0.2f@T3@%0.2f
    //Hmd@65.20@Tmp@21.10
    //012345678901234567890123456789
    String ValT1;
    String ValT2;
    String ValT3;
    String ValHmd;
    for (byte i = pos_char[0] + 1; i < pos_char[1]; i++)
    {
        ValHmd += char(rxpacket[i]);
    }

    String ValTmp;
    // Extraer ValTmp de rxpacket
    for (byte i = pos_char[2] + 1; i < pos_char[3]; i++)
    {
        ValTmp += char(rxpacket[i]);
    }

    String ValPressure;
    // Extraer ValPressure de rxpacket
// Extraer ValPressure de rxpacket
    for (byte i = pos_char[4] + 1; i < pos_char[5]; i++)
    {
        ValPressure += rxpacket[i];
    }
    //Hmd@68.23@Tmp@26.06@Pre@997.12@T1@%0.2f@T2@%0.2f@T3@%0.2f
    //Hmd@65.20@Tmp@21.10
    //012345678901234567890123456789
 //      0     1   2     3   4      5  6     7  8     9  10     rxSize

    for (byte i = pos_char[6] + 1; i < pos_char[7]; i++)
    {
        ValT1 += rxpacket[i];
    }


        for (byte i = pos_char[8] + 1; i < pos_char[9]; i++)
    {
        ValT2 += rxpacket[i];
    }
        for (byte i = pos_char[10] + 1; i < rxSize; i++)
    {
        ValT3 += rxpacket[i];
    }
     humidity = ValHmd.toFloat();
     temperature = ValTmp.toFloat();
     pressure = ValPressure.toFloat();
     v1 = ValT1.toFloat();
     v2 = ValT2.toFloat();
     v3 = ValT3.toFloat();

    //Get current timestamp
    timestamp = getTime();
    Serial.print ("time: ");
    Serial.println (timestamp);

    parentPath= databasePath + "/" + String(timestamp);
 
    json.set(tempPath.c_str(),ValTmp); 
    json.set(humPath.c_str(),ValHmd); 
    json.set(presPath.c_str(),ValPressure); 
//------------------------------------------------------------------
    json.set(sensor1Value.c_str(),ValT1); 
    json.set(sensor2Value.c_str(),ValT2); 
    json.set(sensor3Value.c_str(),ValT3); 
    json.set(timePath, String(timestamp));
    Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
   }  

    Serial.print("f_ValHmd: ");
    Serial.println(humidity);
    Serial.print("f_ValTmp: ");
    Serial.println(temperature);
    Serial.print("f_ValPressure: ");
    Serial.println(pressure);
    Serial.print("ValT1: ");
    Serial.println(v1);
    Serial.print("ValT2: ");
    Serial.println(v2);
    Serial.print("v3: ");
    Serial.println(v3);

    // Serial.print("f_ValHmd: ");Serial.println(f_ValHmd);
    // Serial.print("f_ValTmp: ");Serial.println(f_ValTmp);

    lora_idle = true;
}