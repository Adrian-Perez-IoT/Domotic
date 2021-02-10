#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>



#include <ESP8266WiFiMulti.h>



// forward declarations
void connectToWifi();





ESP8266WiFiMulti wifiMulti;
boolean connectioWasAlive = true;



void setup()
{
  //------- Usado por todas las bibliotecas ---------//
  WiFi.mode(WIFI_OFF);    //Prevents reconnection issue (taking too long to connect)
  Serial.begin(9600);     // Inicio la comunicacion-serial entre el dispositivo XBeeProS2C (Nodo Zigbee Coordinador)
  
  // start soft-serial 1
  Serial1.begin(9600); // Inicio comunicacion-serial-1 para mostrar por el monitor serial los mensajes de depuración
  setup_wifi();

  
  Serial1.print("Se establecion una conexion SoftweareSerial exitosamente... ");
}

void loop()
{
  monitorWiFi();
  if ((wifiMulti.run() == WL_CONNECTED) && (connectioWasAlive == true))
  {
   Serial1.print("me conecte al wifi y la conecion esta viva");
  }
}

//--------------------------
// METHODS AND FUNCTIONS
//--------------------------

void setup_wifi()
{
  Serial1.println("\nWait for WiFi... ");
  wifiMulti.addAP("FBWAY-B262", "965EE62A32AC1825");
  wifiMulti.addAP("angelectronica", "4242714angel");
  wifiMulti.addAP("FacIngenieria", "wifialumnos");
  wifiMulti.addAP("BandySam", "bailavanidosa");
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

