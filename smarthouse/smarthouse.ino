#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#define LDR A0

ESP8266WiFiMulti wifiMulti;
boolean connectioWasAlive = true;
int timestamp = 0;
int pirPin = D0;
int relayPin = D1;

void setup()
{
  //------- Usado por todas las bibliotecas ---------//
  WiFi.mode(WIFI_OFF);    //Prevents reconnection issue (taking too long to connect)
  
  // start soft-serial 1
  Serial1.begin(9600); // Para depurar por monitor serial
  delay(100);
  setup_wifi();
  Serial1.print("Conexión Software-Serial establecida. ");
  pinMode(pirPin, INPUT);
  pinMode(relayPin, OUTPUT);
  
}

void loop()
{
  monitorWiFi();
  int light = analogRead(LDR);
  // si es de noche o esta oscuro
  Serial1.println(light);
  if(light < 600){
    if (digitalRead(pirPin) == LOW)
    {
      Serial1.println("No motion");
      digitalWrite(relayPin, HIGH); // apago el foco
    }
    else
    {
      Serial1.println("Motion detected  ALARM");
      digitalWrite(relayPin, LOW); // enciendo el foco
    }
  }
  else
  {
   Serial1.println("Es de dia y no se necesita detectar movimiento para encender la lampara");
  } 
  
  delay(2000);
}

//--------------------------
// METHODS AND FUNCTIONS
//--------------------------

void setup_wifi()
{
  Serial1.println("\nWait for WiFi... ");
  wifiMulti.addAP("FBWAY-B262", "Perez349");
  while (wifiMulti.run() != WL_CONNECTED)
  {
    Serial1.print(".");
    delay(500);
  }
  // connected
  Serial1.println("WiFi connected");
  Serial1.println("Assigned IP address: ");
  Serial1.print(WiFi.localIP());
}

void monitorWiFi()
{
  // Si Wemos-D1-Mini no esta conectado a Wi-Fi
  if (wifiMulti.run() != WL_CONNECTED)
  {
    //Si previamente estaba conectado a una señal Wi-FI
    if (connectioWasAlive == true)
    {
      connectioWasAlive = false; // Actualizamos. Ahora Wemos-D1-Mini no esta conectado a ningun Wi-Fi
      Serial1.print("Looking for WiFi ");
    }
    Serial1.print(".");
    delay(500);
  }
  else
      //Si previamente no estaba conectado a una señal Wi-FI
      if (connectioWasAlive == false)
  {
    connectioWasAlive = true; // Actualizamos. Ahora Wemos-D1-Mini si esta conectado a Wi-Fi
    Serial1.printf(" Connected to %s. IP address: ", WiFi.SSID().c_str());
    Serial1.print(WiFi.localIP());
  }
}
