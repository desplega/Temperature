#include <OneWire.h>
#include <DallasTemperature.h>
#include <LoRa.h>
#include "board_def.h"

#define LORA_SENDER 0

// Config OLED
OLED_CLASS_OBJ display(OLED_ADDRESS, OLED_SDA, OLED_SCL);

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 23
#define TEMPERATURE_PRECISION 9 // Lower resolution

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

int numberOfDevices; // Number of temperature devices found

DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        if (deviceAddress[i] < 16)
            Serial.print("0");
        Serial.print(deviceAddress[i], HEX);
    }
}

void setup(void)
{
    // OLED config
    if (OLED_RST > 0)
    {
        pinMode(OLED_RST, OUTPUT);
        digitalWrite(OLED_RST, HIGH);
        delay(100);
        digitalWrite(OLED_RST, LOW);
        delay(100);
        digitalWrite(OLED_RST, HIGH);
    }

    Serial.println("VSC compilation");

    display.init();
    display.flipScreenVertically();
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    delay(2000);

    // start serial port
    Serial.begin(115200);
    Serial.println("Dallas Temperature IC Control Library Demo");

    // Start up the library
    sensors.begin();

    // Grab a count of devices on the wire
    //numberOfDevices = sensors.getDeviceCount(); // This function doesn't work :(
#if LORA_SENDER
    numberOfDevices = 2;
#else
    numberOfDevices = 0;
#endif

    // Display in OLED
    display.clear();
#if LORA_SENDER
    display.drawString(display.getWidth() / 2 - 22, display.getHeight() / 2 - 32, "Sender:");
#endif
    display.display();

    // locate devices on the bus
    Serial.print("Locating devices...");

    Serial.print("Found ");
    Serial.print(numberOfDevices, DEC);
    Serial.println(" devices.");

    // report parasite power requirements
    Serial.print("Parasite power is: ");
    if (sensors.isParasitePowerMode())
        Serial.println("ON");
    else
        Serial.println("OFF");

    // Loop through each device, print out address
    for (int i = 0; i < numberOfDevices; i++)
    {
        // Search the wire for address
        if (sensors.getAddress(tempDeviceAddress, i))
        {
            Serial.print("Found device ");
            Serial.print(i, DEC);
            Serial.print(" with address: ");
            printAddress(tempDeviceAddress);
            Serial.println();

            Serial.print("Setting resolution to ");
            Serial.println(TEMPERATURE_PRECISION, DEC);

            // set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
            sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);

            Serial.print("Resolution actually set to: ");
            Serial.print(sensors.getResolution(tempDeviceAddress), DEC);
            Serial.println();
        }
        else
        {
            Serial.print("Found ghost device at ");
            Serial.print(i, DEC);
            Serial.println(" but could not detect address. Check power and cabling");
        }
    }

    // LoRa setup
    SPI.begin(CONFIG_CLK, CONFIG_MISO, CONFIG_MOSI, CONFIG_NSS);
    LoRa.setPins(CONFIG_NSS, CONFIG_RST, CONFIG_DIO0);
    if (!LoRa.begin(BAND))
    {
        Serial.println("Starting LoRa failed!");
        while (1)
            ;
    }
    if (!LORA_SENDER)
    {
        display.clear();
        display.drawString(display.getWidth() / 2, display.getHeight() / 2, "LoraRecv Ready");
        display.display();
    }

    // LED
    pinMode(LED_BUILTIN, OUTPUT);
}

#if LORA_SENDER
// function to send the temperature through LoRa
void sendTemperature(DeviceAddress deviceAddress, int tIndex)
{
    float tempC = sensors.getTempC(deviceAddress);
    Serial.print("Temp C: ");
    Serial.println(tempC);

    // Send data through LoRa
    LoRa.beginPacket();
    if (tIndex == 0)
    {
        LoRa.print("Temp 1: " + String(tempC) + " C");
        display.clear();
        display.drawString(display.getWidth() / 2, display.getHeight() / 2 - 32, "Sender >>>>>>>");
        display.drawString(display.getWidth() / 2, display.getHeight() / 2 - 4, "Temp 1: " + String(tempC) + " C");
    }
    else
    {
        LoRa.print("Temp 2: " + String(tempC) + " C");
        display.drawString(display.getWidth() / 2, display.getHeight() / 2 + 16, "Temp 2: " + String(tempC) + " C");
        display.display();
    }
    LoRa.endPacket();
}
#else
void printTemperature(String recv, int value)
{
    // Print data from LoRa
    if (value % 2 == 0)
    {
        display.clear();
        display.drawString(display.getWidth() / 2, display.getHeight() / 2 - 32, ">>>>> Receiver");
        display.drawString(display.getWidth() / 2, display.getHeight() / 2 - 4, recv);
    }
    else
    {
        display.drawString(display.getWidth() / 2, display.getHeight() / 2 + 16, recv);
        display.display();
    }
}
#endif

int value = 0;
void loop(void)
{
#if LORA_SENDER
    // call sensors.requestTemperatures() to issue a global temperature
    // request to all devices on the bus
    Serial.print("Requesting temperatures...");
    sensors.requestTemperatures(); // Send the command to get temperatures
    Serial.println("DONE");

    // Loop through each device, print out temperature data
    for (int i = 0; i < numberOfDevices; i++)
    {
        // Search the wire for address
        if (sensors.getAddress(tempDeviceAddress, i))
        {
            // Output the device ID
            Serial.print("Temperature for device: ");
            Serial.println(i, DEC);

            // It responds almost immediately. Let's print out the data
            sendTemperature(tempDeviceAddress, i); // Use a simple function to print out the data
        }
        //else ghost device! Check your power requirements and cabling
    }
    delay(1000);
    // Toggle BUILTIN led
    if (value++ % 2 == 0)
        digitalWrite(LED_BUILTIN, HIGH); // Turn the LED on
    else
        digitalWrite(LED_BUILTIN, LOW); // Turn the LED off
#else
    if (LoRa.parsePacket())
    {
        String recv = "";
        while (LoRa.available())
        {
            recv += (char)LoRa.read();
        }

        // Print temperature
        printTemperature(recv, value);
        // Toggle BUILTIN led
        if (value++ % 2 == 0)
            digitalWrite(LED_BUILTIN, HIGH); // Turn the LED on
        else
            digitalWrite(LED_BUILTIN, LOW); // Turn the LED off
    }
#endif
}
