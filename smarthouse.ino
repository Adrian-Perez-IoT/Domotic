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
    //------- Logica MQTT ---------//
    // Attempt to read a packet (Intento leer un frame Zigbee)
    xbee.readPacket();
    if (xbee.getResponse().isAvailable())
    {
      // got something (obtubimos algo). Si el Id del grame es "Respuesta de muestreo IO"
      if (xbee.getResponse().getApiId() == ZB_IO_SAMPLE_RESPONSE)
      {
        time_t t = now(); // apenas legue un paquete de tipo iosamples, asigno hora de su llegada (sin importar que sea muestreo del pir, magnetic o mq7).
        //a continuacion le asigno el tiempo transcurrido desde 1977. Luego en el server backend calculo la hora actual.  
        doc["time"] = t; 
        if (!client.connected())
        {
          // Serial1.println("Reconectando al Servidor MQTT Diveriot ...");
          reconnect();
        }
        client.loop(); //¿lo podria omitir?Para que el Wemos-D1-Mini procese los mensajes entrantes y mantenga su conexión al Servidor Backend.

        // Obtengo el Sample-IO del frame recibido y lo guardo en una variable llamada "ioSample"
        xbee.getResponse().getZBRxIoSampleResponse(ioSample);

        //Serial1.print("Received I/O Sample from: ");
        //Serial1.print(ioSample.getRemoteAddress64().getMsb(), HEX);
        //Serial1.print(ioSample.getRemoteAddress64().getLsb(), HEX);
        //Serial1.println("");

        //si ioSample proviene del Nodo Router
        if (routerXBeeMAC == ioSample.getRemoteAddress64())
        {
          //Serial1.println("Received I/O Sample from ROUTER");
          if (ioSample.containsDigital())
          {
            //Serial1.println("Sample contains digtal data");
            // check digital inputs desde el ¿DIO?0 hasta el ¿DIO?12)
            for (int i = 0; i <= 12; i++)
            {
              // si es DI0 (Sensor de movimiento conectado al pin DIO0)
              if (i == 0)
              {
                // si DIO0 (pin 20) esta seteado como Digital Input (previamente configurado con XCTU)
                if (ioSample.isDigitalEnabled(i))
                {
                  doc["sensor"] = "PIR";
                  if (ioSample.isDigitalOn(i) == 1)
                  {
                    //Serial1.println("Se detecto movimiento.");
                    doc["value"] = true;
                    serializeJson(doc, mybuffer);
                    client.publish(TOPIC_MOVIMIENTO, mybuffer);
                    Serial1.println(" ");
                    Serial1.print("Publish on topic ");
                    Serial1.print(TOPIC_MOVIMIENTO);
                    Serial1.print(" message:     ");
                    Serial1.print(mybuffer);
                    Serial1.println(" ");

                    // size_t memoryUsage() const;
                    // Serial1.println(doc.memoryUsage());     // 16 on AVR
                    // Serial1.println(mybuffer.memoryUsage());     // 16 on AVR

                    // snprintf(motion, 50, "{\"value\":\"True\", \"timestamp\":\"%d:%d:%d\"}", hour(t), minute(t), second(t));;
                    // Serial1.print("Publish message on topic 'Casa/Patio/Motion001': ");
                    // Serial1.println(motion);
                    // client.publish("Casa/Patio/Motion001", motion);
                  }
                  if (ioSample.isDigitalOn(i) == 0)
                  {
                    //Serial1.println("No se detecto movimiento");
                    doc["value"] = false;
                    serializeJson(doc, mybuffer);
                    client.publish(TOPIC_MOVIMIENTO, mybuffer);
                    Serial1.println(" ");
                    Serial1.print("Publish on topic ");
                    Serial1.print(TOPIC_MOVIMIENTO);
                    Serial1.print(" message:     ");
                    Serial1.print(mybuffer);
                    Serial1.println(" ");

                    //snprintf(motion, 50, "{\"value\":\"False\", \"timestamp\":\"%d:%d:%d\"}", hour(t), minute(t), second(t));;
                    //Serial1.print("Publish message on topic 'Casa/Patio/Motion001': ");
                    // Serial1.println(motion);
                  }
                }
                else
                {
                  Serial1.println("DIO0 (Pin20) no esta seteado como Digital-Input. Debe configurarlo con XCTU");
                }
              }
              /*-----------------------------*/
              // si es DI2 (Sensor de puerta)
              if (i == 2)
              {
                if (ioSample.isDigitalEnabled(i))
                {
                  doc["sensor"] = "MAGNETIC";
                  if (ioSample.isDigitalOn(i) == 1)
                  {
                    //Serial1.println("Puerta abierta");
                    doc["value"] = true;
                    serializeJson(doc, mybuffer);
                    client.publish(TOPIC_PORTON, mybuffer);
                    Serial1.println(" ");
                    Serial1.print("Publish on topic ");
                    Serial1.print(TOPIC_PORTON);
                    Serial1.print(" message:             ");
                    Serial1.print(mybuffer);
                    Serial1.println(" ");
                  }
                  if (ioSample.isDigitalOn(i) == 0)
                  {
                    //Serial1.println("Puerta cerrada");
                    doc["value"] = false;
                    serializeJson(doc, mybuffer);
                    client.publish(TOPIC_PORTON, mybuffer);
                    Serial1.println(" ");
                    Serial1.print("Publish on topic ");
                    Serial1.print(TOPIC_PORTON);
                    Serial1.print(" message:             ");
                    Serial1.print(mybuffer);
                    Serial1.println(" ");                    
                  }
                }
                else
                {
                  Serial1.println("DIO2 (Pin 18) no esta seteado como Digital-Input. Debe configurarlo con XCTU");
                }
              }
            }
          }
        }

        //Si el frame proviene del Nodo EndDevice
        if (endDeviceXBeeMAC == ioSample.getRemoteAddress64())
        {
          //Serial1.println("Received I/O Sample from END-DEVICE");
          //El frame contiene Sample Digital
          if (ioSample.containsDigital())
          {
            //Serial1.println("Sample contains digtal data");
            // check digital inputs
            for (int i = 0; i <= 12; i++)
            {
              // si es DI0 (Sensor de GAS)
              if (i == 0)
              {
                // si DIO0 (pin 20) esta seteado como Digital Input (previamente realizado con XCTU)
                if (ioSample.isDigitalEnabled(i))
                {
                  doc["sensor"] = "MQ7";
                  if (ioSample.isDigitalOn(i) == 0)
                  { 
                    //Serial1.println("Peligro!.Se detecto monoxido de carbono");
                    /*
                       Aunque con sprintf todo funciona bien, recomiendo utilizar snprintf() ya que a éste último
                       le debemos decir el tamaño de la variable a escribir y si los valores hacen la cadena más larga de lo que debería,
                       ésta será recortada, es decir, si reservamos 50 bytes para la cadena, con sprintf() tal vez se escriban más,
                       depende de los datos a escribir, pero snprintf() escribirá 50.
                       snprintf(gas, 50, "{\"value\":\"Normal\", \"timestamp\":\"%d:%d:%d\"}", hour(t), minute(t), second(t));
                    */                                        
                    doc["value"] = true;
                    serializeJson(doc, mybuffer);
                    client.publish(TOPIC_MONOXIDO_CARBONO, mybuffer);
                    Serial1.println(" ");
                    Serial1.print("Publish on topic ");
                    Serial1.print(TOPIC_MONOXIDO_CARBONO);
                    Serial1.print(" message:    ");
                    Serial1.print(mybuffer);
                    Serial1.println(" ");
                  }                  
                  if (ioSample.isDigitalOn(i) == 1)
                  {
                    //Serial1.println("No se detecto monoxido de carbono");
                    //Serial1.println("Puerta cerrada");
                    doc["value"] = false;
                    serializeJson(doc, mybuffer);
                    client.publish(TOPIC_MONOXIDO_CARBONO, mybuffer);
                    Serial1.println(" ");
                    Serial1.print("Publish on topic ");
                    Serial1.print(TOPIC_MONOXIDO_CARBONO);
                    Serial1.print(" message:    ");
                    Serial1.print(mybuffer);
                    Serial1.println(" ");    
                  }
                  //Serial1.print("Digital (DI");
                  //Serial1.print(i, DEC);
                  //Serial1.print(") is ");
                  //Serial1.println(ioSample.isDigitalOn(i), DEC);
                }
                else
                {
                  Serial1.println("Ningun PIN esta seteado como Digital-Input. Debe configurar el nodo EndDevice con la herramienta XCTU");
                }
              }
            }
          }
        }
      }
      else
      {
        Serial1.print("Expected I/O Sample, but got ");
        Serial1.print(xbee.getResponse().getApiId(), HEX);
      }
    }
    else if (xbee.getResponse().isError())
    {
      Serial1.print("Error reading packet.  Error code: ");
      Serial1.println(xbee.getResponse().getErrorCode());
    }
  }
}

//--------------------------
// METHODS AND FUNCTIONS
//--------------------------

// Prints a nice time display
void digitalClockDisplay()
{
  // We'll grab the time so it doesn't change whilst we're printing it
  time_t t = now();

  //Now print all the elements of the time secure that it won't change under our feet
  //printDigits(hour(t)-3); // le sumo tres unidades para obtgener la hora correspondiente a mi zona (timezone=-3)
  printDigits(hour(t));
  Serial1.print(":");
  printDigits(minute(t));
  Serial1.print(":");
  printDigits(second(t));
  Serial1.print("    ");
  Serial1.print(DoW[weekday(t) - 1]);
  Serial1.print(" ");
  printDigits(day(t));
  Serial1.print("/");
  printDigits(month(t));
  Serial1.print("/");
  printDigits(year(t));
  Serial1.println();
}

void printDigits(int digits)
{
  // utility for digital clock display: prints leading 0
  if (digits < 10)
    Serial1.print('0');
  Serial1.print(digits);
}

// This is the function to contact the NTP pool and retrieve the time
//-----------------------------------------------------------------------------
time_t getNTPTime()
{
  // Send a UDP packet to the NTP pool address
  Serial1.print("\nSending NTP packet to Time-Server ");
  Serial1.println(timeServer);
  sendNTPpacket(timeServer); // Solicitamos al Servidor NTP los segundos transcurridos desde el año 1900 (para luego calcular la hora actua)

  // Wait to see if a reply is available - timeout after X seconds. At least
  // this way we exit the 'delay' as soon as we have a UDP packet to process
#define UDPtimeoutSecs 5
  int timeOutCnt = 0;
  while (Udp.parsePacket() == 0 && ++timeOutCnt < (UDPtimeoutSecs * 10))
  {
    delay(100);
    yield(); //Pasa el control a otras tareas cuando se llama. Idealmente, yield () debería usarse en funciones que tardarán un tiempo en completarse
  }

  // Is there UDP data present to be processed? Sneak a peek!
  if (Udp.peek() != -1)
  {
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

//-----------------------------------------------------------------------------
// Send an NTP request to the time server at the given address
//-----------------------------------------------------------------------------
void sendNTPpacket(const char *address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011; // LI, Version, Mode
  packetBuffer[1] = 0;          // Stratum, or type of clock
  packetBuffer[2] = 6;          // Polling Interval
  packetBuffer[3] = 0xEC;       // Peer Clock Precision
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

// Para que el Wemos-D1-Mini se suscriba a Topics y reciva los mensajes publicados desde el Servidor Backend. {No uttilizo esta funcion en la tesis. luego borrarla para peinar el codigo.}
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  //El WemosD1mini usa esta funcion callback para estar atento si hay nuevas publicaciones en el topic suscrito (casa/living/luz) y en caso afirmativo
  //enviar un frame al Xbee-Router para que activar/desactivar Relee(foco)

  // Switch on the LED if an 1 was received as first character//envia un frame al XBee-Router(XBee-terminal) con "1" para activar el Relee que enciende el foco de luz
  if ((char)payload[0] == '1')
  {
    digitalWrite(BUILTIN_LED, LOW); // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  }
  else
  {
    digitalWrite(BUILTIN_LED, HIGH); // Turn the LED off by making the voltage HIGH
  }
}

void reconnect()
{
  // Loop until we're reconnected to Server Backend (MQTT)
  while (!client.connected())
  {
    Serial1.print("\nAttempting MQTT connection...");
    // Create a random client ID
    String clientId = "Gateway-ESP8266-";
    clientId += String(random(0xffff), HEX); //cada vez que nos reconectamos, debemos hacerlo con un nuevo clientID (no el mismo).
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    { //c_str() Convierte el contenido de una cadena a una cadena de estilo C. Nunca debe modificar la cadena a través del puntero devuelto.
      Serial1.print("connected \n");
      // Once connected, publish an announcement...
      client.publish("Log", "Gateway-ESP8866 reconectado al servidor mqtt://mqtt.diveriot.com:1883");
      // ... and resubscribe
      /* client.subscribe("Casa/LivingRoom/Luz"); // Me suscribo al TOPIC cuando me conecto por primera vez al Server Backend. (NO LO USARE POR EL MOMENTO EN LA TESIS) */
    }
    else
    {
      Serial1.print("failed, rc=");
      Serial1.print(client.state());
      Serial1.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
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

