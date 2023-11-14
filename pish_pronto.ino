#include <Wire.h>
#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <addons/TokenHelper.h>
#include <NTPClient.h>
#include "ClosedCube_HDC1080.h"

#include <addons/RTDBHelper.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "TP-Link_BRUNO"
#define WIFI_PASSWORD "12345678k"

// For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino

/* 2. Define the API Key */
#define API_KEY "AIzaSyDp-6I41OyrK75U4j1I-bYAzVdaYM_rZFE"

/* 3. Define the RTDB URL */
#define DATABASE_URL "https://pish-3d992-default-rtdb.firebaseio.com/" //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "mikael.araya9500@gmail.com"
#define USER_PASSWORD "123321"

WiFiUDP udp;
NTPClient ntp(udp, "a.st1.ntp.br", -3 * 3600, 60000);/* Cria um objeto "NTP" com as configurações.utilizada no Brasil */
String hora; 
ClosedCube_HDC1080 hdc1080;

FirebaseData firebaseData;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;

unsigned long count = 0;
const int sensorPin = 35;
void setup()
{
  Wire.begin(21, 22);
  Serial.begin(9600);


  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  config.api_key = API_KEY;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.database_url = DATABASE_URL;

  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);

  // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  firebaseData.setBSSLBufferSize(4096 , 1024 );

  Firebase.begin(&config, &auth);

  Firebase.setDoubleDigits(5);

  esp_sleep_enable_timer_wakeup(600 * 1000000);

  ntp.begin();               
  ntp.forceUpdate();

  hdc1080.begin(0x40);
  hdc1080.setResolution(HDC1080_RESOLUTION_14BIT, HDC1080_RESOLUTION_14BIT);

  
}
void loop() {
  String hora = ntp.getFormattedTime();
  time_t rawTime = ntp.getEpochTime();
  struct tm *timeInfo;
  timeInfo = localtime(&rawTime);
  char dataBuffer[20];
  strftime(dataBuffer, 20, "%Y-%m-%d %H:%M:%S", timeInfo);
  String dataHora = dataBuffer;

  int sensorValue = analogRead(sensorPin);
  float percentHumidity = map(sensorValue, 1000, 4095, 100, 0); // Mapeie o valor lido para a faixa correta de 0 a 100

  String path_umidade_solo = "/umidade_solo_5/" + dataHora;
  String path_temperatura = "/temperatura_5/" + dataHora; // Caminho para a temperatura
  String path_umidade_ar = "/umidade_ar_5/" + dataHora; // Caminho para a umidade relativa do ar

  Serial.print("Umidade do Solo: ");
  Serial.print(percentHumidity);
  Serial.println(" ");

  Serial.print("T=");
  float temperatura = hdc1080.readTemperature(); // Lê a temperatura
  Serial.print(temperatura);
  Serial.print("°C, RH=");
  float umidade_ar = hdc1080.readHumidity(); // Lê a umidade relativa do ar
  Serial.print(umidade_ar);
  Serial.println("%");

  Firebase.setInt(firebaseData, path_umidade_solo.c_str(), percentHumidity);
  Firebase.setFloat(firebaseData, path_temperatura.c_str(), temperatura); // Envia temperatura para o Firebase
  Firebase.setFloat(firebaseData, path_umidade_ar.c_str(), umidade_ar); // Envia umidade relativa do ar para o Firebase



  esp_deep_sleep_start();
}
