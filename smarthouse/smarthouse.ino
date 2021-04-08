#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiUdp.h> // UDP library which is how we communicate with Time Server
#include <TimeLib.h>
#define LDR A0 // Pin sensor fotoresistencia
#define _resyncSeconds 300 //300 is 5 minutos // 3600 is 1 hour. 86400 is on day
#define _resyncErrorSeconds 15
#define _millisMinute 60000

ESP8266WiFiMulti wifiMulti;
boolean connectioWasAlive = true;
int pirPin = D0;
int relayPin = D1;
unsigned int localPort = 8888; // Just an open port we can use for the UDP packets coming back in
char timeServer[] = "uk.pool.ntp.org";//"time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP Udp; // A UDP instance to let us send and receive packets over UDP
const int timeZone = -3; // Argentine
char timestamp[50];
char nivelDeLuz[50];

// forward declarations
time_t getNTPTime();


void setup()
{  
  WiFi.mode(WIFI_OFF);    //Prevents reconnection issue (taking too long to connect)    
  Serial1.begin(9600); // Para depurar por monitor serial
  delay(100);
  setup_wifi();  
  pinMode(pirPin, INPUT);
  pinMode(relayPin, OUTPUT);
  //Sincronizar con Time-Server.
  Udp.begin(localPort); // What port will the UDP/NTP packet respond on?
  setSyncProvider(getNTPTime); // What is the function that gets the time (in ms since 01/01/1900)?  
  setSyncInterval(_resyncSeconds);
  
}

void loop()
{
  monitorWiFi();  
  time_t t = now(); // Obtengo HH:MM:SS actualizado
  snprintf(timestamp, 50, "timestamp: %d:%d:%d", hour(t), minute(t), second(t));                                          
  Serial1.println(timestamp);    
  snprintf(nivelDeLuz, 50, "Nivel de luz: %d", analogRead(LDR));                                          
  Serial1.println(nivelDeLuz);  
  //Pendiente: asignar franja horaria de forma remota (para que el usuario pueda elegir los horarios de funcionamiento para detectar movimiento y encender foco)
  //if ((hour(t) < 8) || (hour(t) > 18) ) {    
  if ((hour(t) <= 12) || (hour(t) >= 18) ) {    
    if (digitalRead(pirPin) == LOW) {
      Serial1.println("No motion");
      digitalWrite(relayPin, HIGH); // apago el foco. ¿Es necesario una resistencia 220 Ohm para evitar que el relay consuma demasiada correinte (amper) del wemos?
    }
    else {
      Serial1.println("Motion detected - LIGHT ON");
      digitalWrite(relayPin, LOW); // enciendo el foco
    }    
  }
  else {
    digitalWrite(relayPin, HIGH); // apago el foco. ¿Es necesario una resistencia 220 Ohm para evitar que el relay consuma demasiada correinte (amper) del wemos?
    Serial1.println("Es de día, no es necesario encender la luz");
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

time_t getNTPTime() {
  // Send a UDP packet to the NTP pool address
  Serial1.print("\nSending NTP packet to Time-Server ");
  Serial1.println(timeServer);
  sendNTPpacket(timeServer);  // Solicitamos al Servidor NTP los segundos transcurridos desde el año 1900 (para luego calcular la hora actua)

  // Wait to see if a reply is available - timeout after X seconds. At least
  // this way we exit the 'delay' as soon as we have a UDP packet to process
#define UDPtimeoutSecs 5
  int timeOutCnt = 0;
  while (Udp.parsePacket() == 0 && ++timeOutCnt < (UDPtimeoutSecs * 10)) {
    delay(100);
    yield(); //Pasa el control a otras tareas cuando se llama. Idealmente, yield () debería usarse en funciones que tardarán un tiempo en completarse
  }

  // Is there UDP data present to be processed? Sneak a peek!
  if (Udp.peek() != -1) {
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    // The time-stamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:
    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);

    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900)
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    //Serial1.print("Seconds since Jan 1 1900 = ");
    //Serial1.println(secsSince1900);
    Serial1.println("Date and hour sincronized");

    // now convert NTP time into everyday time:
    //Serial1.print("Unix time = ");

    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;

    // subtract seventy years and convert to time zone local:
    unsigned long epoch = secsSince1900 - seventyYears + timeZone * SECS_PER_HOUR;

    // Reset the interval to get the time from NTP server in case we previously changed it. Por ejemplo en caso de error del servidor que no responde y utilizo el _resyncErrorSeconds
    setSyncInterval(_resyncSeconds);

    // LED indication that all is well
    //digitalWrite(LED_BUILTIN,LOW);  // led de indicacion de que todo esta bien.

    return epoch;
  }

  // Failed to get an NTP/UDP response
  Serial1.println("No response from ServerTime. Failed to sync current time.");
  setSyncInterval(_resyncErrorSeconds);

  return 0;
}

void sendNTPpacket(const char* address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  // all NTP fields have been given values, now you can send a packet requesting a timestamp:
  // Note that Udp.begin will request automatic translation (via a DNS server) from a
  // name (eg pool.ntp.org) to an IP address. Never use a specific IP address yourself,
  // let the DNS give back a random server IP address
  Udp.beginPacket(address, 123); //NTP requests are to port 123

  // Get the data back
  Udp.write(packetBuffer, NTP_PACKET_SIZE);

  // All done, the underlying buffer is now updated
  Udp.endPacket();
}

// NOTA: Los mensajes de depuracion (Serial1.printf()) transferirlos al servidor backend (broker mqtt)
