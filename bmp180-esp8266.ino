/*
 *  Modified for ESP8266     - Brian Ernst Feb 2021
 *  Added MQTT and WIFI code - Brian Ernst March 2021

    Connections for ESP 8266
    ===========
    Connect SCL to D1
    Connect SDA to D2
    Connect VDD to 3.3V DC
    Connect GROUND to common ground
 */

// INCLUDE

// includes for BMP180 temp sensor via I2C
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>

// includes for MQTT via WiFi
#include <ESP8266WiFi.h>        // WiFi
#include <PubSubClient.h>       // MQTT

// include for timer utility Chrono
#include <Chrono.h> 

// Constants for MQTT via WiFi
// Update these with values suitable for your network.

const char* ssid = "bouncyhouse";
const char* password = "bakabaka";
const char* mqtt_server = "192.168.2.10";

// global message buffer for mqtt
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];

// Instantiate MQTT and WiFi clients
WiFiClient espClient;
PubSubClient client(espClient);

// Instantiate sensor data object
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

// Instantiate a Chrono object.
Chrono timeToSample; 

/**************************************************************************/
/*
    Displays some basic information on this sensor from the unified
    sensor API sensor_t type (see Adafruit_Sensor for more information)
*/
/**************************************************************************/
void displaySensorDetails(void)
{
  sensor_t sensor;
  bmp.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" hPa");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" hPa");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" hPa");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// MQTT reconnect function - connects and reconnects to MQTT server
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void getSensorEvent()
{
	  sensors_event_t event;
	  bmp.getEvent(&event);
	 
	  /* Display the results (barometric pressure is measure in hPa) */
	  if (event.pressure)
	  {
	    /* Display atmospheric pressue in hPa */
	    Serial.print("Pressure:    ");
	    Serial.print(event.pressure);
	    Serial.println(" hPa");
	    
	    /* Calculating altitude with reasonable accuracy requires pressure    *
	     * sea level pressure for your position at the moment the data is     *
	     * converted, as well as the ambient temperature in degress           *
	     * celcius.  If you don't have these values, a 'generic' value of     *
	     * 1013.25 hPa can be used (defined as SENSORS_PRESSURE_SEALEVELHPA   *
	     * in sensors.h), but this isn't ideal and will give variable         *
	     * results from one day to the next.                                  *
	     *                                                                    *
	     * You can usually find the current SLP value by looking at weather   *
	     * websites or from environmental information centers near any major  *
	     * airport.                                                           *
	     *                                                                    *
	     * For example, for Paris, France you can check the current mean      *
	     * pressure and sea level at: http://bit.ly/16Au8ol                   */
	     
	    /* First we get the current temperature from the BMP085 */
	    float temperature;
	    bmp.getTemperature(&temperature);
	    Serial.print("Temperature: ");
	    Serial.print(temperature);
	    Serial.println(" C");
	
	    /* Then convert the atmospheric pressure, and SLP to altitude         */
	    /* Update this next line with the current SLP for better results      */
	    float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
	    Serial.print("Altitude:    "); 
	    Serial.print(bmp.pressureToAltitude(seaLevelPressure,
	                                        event.pressure)); 
	    Serial.println(" m");
	    Serial.println("");
	    
	    // send temp to mqtt
        snprintf (msg, MSG_BUFFER_SIZE, "temp: %3.1f C", temperature);
        Serial.print("Publish message: ");
        Serial.println(msg);
	    client.publish("temp01", msg);

	  }
	  else
	  {
	    Serial.println("Sensor error");
	  }
}

/**************************************************************************/
/*
    Arduino setup function (automatically called at startup)
*/
/**************************************************************************/
void setup(void) 
{
  Serial.begin(115200);
  Serial.println("Pressure Sensor Test"); Serial.println("");
  
  /* Initialize the sensor */
  if(!bmp.begin())
  {
    /* There was a problem detecting the BMP085 ... check your connections */
    Serial.print("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  
  /* Display some basic information on this sensor */
  displaySensorDetails();
  
// Setup the Wifi connection
  setup_wifi();

// Setup MQTT client
  client.setServer(mqtt_server, 1883);

}

/**************************************************************************/
/*
    Arduino loop function, called once 'setup' is complete
*/
/**************************************************************************/
void loop(void) 
{
  // Connect to MQTT server and reconnect if disconnected
  if (!client.connected()) {
    reconnect();
  }
  client.loop(); // non-blocking mqtt background updates

  if (timeToSample.hasPassed(1000)) // returns 1 if 1000 ms have passed
  {
	  timeToSample.restart();       // restart timer
	  /* Get a new sensor event */ 
	  getSensorEvent();
	  
  } // endif timeToSample.hasPassed
}
