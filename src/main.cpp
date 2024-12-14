#define BLYNK_TEMPLATE_ID "TMPL6vNnqsw12"
#define BLYNK_TEMPLATE_NAME "Automatic Poultry Incubator"
char auth[] = "c-aLxAIZDCJ53ZX1bW7xzu4Od0b42NVs"; // Replace with your Blynk Auth Token
#include <EMailSender.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DHT.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <NTPClient.h>
#include <WiFiClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#define EEPROM_SIZE 20
// EEPROM addresses for settings
#define EEPROM_ADDR_TARGET_TEMP 0
#define EEPROM_ADDR_TARGET_HUMIDITY 4
#define EEPROM_ADDR_CALIBRATION_OFFSET 8
#define EEPROM_ADDR_HYSTERESIS 12

// Pin definitions
#define ONE_WIRE_BUS 13
#define BUTTON_UP 15
#define BUTTON_DOWN 18
#define BUTTON_SELECT 5

// Relay Pins
#define RELAY_HUMIDITY 32
#define RELAY_HEATING 33
#define RELAY_COOLING 23
#define RELAY_ADDITIONAL_HEATING 26
#define RELAY_TURN_LEFT 27
#define RELAY_TURN_RIGHT 14
#define RELAY_LIGHT 19 // Add the relay pin for the light

// DHT22 sensor
#define DHTPIN 4
#define DHTTYPE DHT22

// Wi-Fi credentials
const char *ssid[] = {"bkpoudel_fctwn", "Baral@patihaniNet", "lamsal-wifi11_fctwn_2.4g"};
const char *password[] = {"CLB2710A59", "9845023546", "9845144415P"};
const int numNetworks = 3;

// LCD
#define I2C_SDA 21
#define I2C_SCL 22

// Sensor and LCD objects
DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
LiquidCrystal_I2C lcd(0x27, 20, 4);
EMailSender emailSend("esp322376@gmail.com", "nejyqiqnkalzdtas");

// Variables
unsigned long previousMillis = 0;
const long interval = 1000; // Interval in milliseconds (1 second)
int menu = 0;
float targetTemp = 37.5;
float targetHumidity = 60.0;
float hysteresis = 0.5;
float offset = 0.0;
float humidityoffset = 0.0;
float humidityhysteresis = 10.0;
float additional_heater_threshold = 2.0;
bool heatingstatus = false;
bool coolingstatus = false;
bool optimumtemperaturereached = false;
bool email_condition_met = false;
unsigned long wifiReconnectPreviousMillis = 0;
const long wifiReconnectInterval = 600000; // 10 minutes in milliseconds

// NTP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 20700, 60000);

// Timing variables for egg turning
unsigned long lastTurnTime = 0;
unsigned long turnInterval = 1 * 60 * 60 * 1000; // 1 hour in milliseconds
unsigned long turnDuration = 1 * 60 * 1000;      // 1 minute in milliseconds
bool turnLeft = true;

// Email sending related variables
unsigned long lastEmailTime = 0;
unsigned long emailTimer = 0;
const unsigned long emailInterval = 10 * 60 * 1000; // 10 minutes in milliseconds

// Max/Min values
float maxTemp = -999.0;
float minTemp = 999.0;
float maxHumidity = -999.0;
float minHumidity = 999.0;

// Last email status
String lastEmailStatus = "No email sent";

void setup()
{
    Serial.begin(115200);

    // Initialize sensors
    dht.begin();
    sensors.begin();
    // initialize EEPROM
    EEPROM.begin(EEPROM_SIZE);

    // Initialize LCD
    lcd.init();
    lcd.backlight();

    // Configure button pins with internal pullups
    pinMode(BUTTON_UP, INPUT_PULLUP);
    pinMode(BUTTON_DOWN, INPUT_PULLUP);
    pinMode(BUTTON_SELECT, INPUT_PULLUP);

    // Relay pin definitions
    pinMode(RELAY_HUMIDITY, OUTPUT);
    pinMode(RELAY_HEATING, OUTPUT);
    pinMode(RELAY_COOLING, OUTPUT);
    pinMode(RELAY_ADDITIONAL_HEATING, OUTPUT);
    pinMode(RELAY_TURN_LEFT, OUTPUT);
    pinMode(RELAY_TURN_RIGHT, OUTPUT);
    pinMode(RELAY_LIGHT, OUTPUT);

    // Set relay pins to LOW (Inverse Trigger Logic)
    digitalWrite(RELAY_HUMIDITY, HIGH);
    digitalWrite(RELAY_HEATING, HIGH);
    digitalWrite(RELAY_COOLING, HIGH);
    digitalWrite(RELAY_ADDITIONAL_HEATING, HIGH);
    digitalWrite(RELAY_TURN_LEFT, HIGH);
    digitalWrite(RELAY_TURN_RIGHT, HIGH);
    digitalWrite(RELAY_LIGHT, HIGH); // Set initial state (off)
    // Initialize variables using EEPROM values
    EEPROM.get(EEPROM_ADDR_TARGET_TEMP, targetTemp);
    EEPROM.get(EEPROM_ADDR_TARGET_HUMIDITY, targetHumidity);
    EEPROM.get(EEPROM_ADDR_CALIBRATION_OFFSET, offset);
    EEPROM.get(EEPROM_ADDR_HYSTERESIS, hysteresis);

    // Display Default Screen
    lcd.setCursor(0, 0);
    lcd.print("     Automatic");
    lcd.setCursor(0, 1);
    lcd.print(" Poultry Incubator");
    lcd.setCursor(0, 2);
    lcd.print("    Developed By");
    lcd.setCursor(0, 3);
    lcd.print("    Bibek Poudel");
    delay(2000);
    lcd.clear();

    // Connect to Wi-Fi
    connectWiFi();

    // Initialize NTP client
    timeClient.begin();
    Blynk.begin(auth, ssid[0], password[0]); // Connect to Blynk server

    delay(2000);
    lcd.clear();
}

void loop()
{
    unsigned long currentMillis = millis();
    if (!WiFi.isConnected() && (currentMillis - wifiReconnectPreviousMillis >= wifiReconnectInterval))
    {
        wifiReconnectPreviousMillis = currentMillis;
        connectWiFi();
    }
    // Maintain Blynk connection
    Blynk.run();

    // Read button states with debounce
    bool upPressed = !debounce(BUTTON_UP);
    bool downPressed = !debounce(BUTTON_DOWN);
    bool selectPressed = !debounce(BUTTON_SELECT);

    if (selectPressed)
    {
        displayMenu();
    }

    displayDefaultScreen();
    controlTemperature();
    controlHumidity();
    turnEggs(currentMillis);
    controlLight();

    if (email_condition_met)
    {
        if (currentMillis - emailTimer > emailInterval)
        {
            sendEmail("Optimal Condition Not Met for More Than 10 Minutes.");
            emailTimer = currentMillis; // Reset the timer
        }
    }

    delay(100);
}
void displayDefaultScreen()
{
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;
        sensors.requestTemperatures();
        float temperature = sensors.getTempCByIndex(0) + offset;
        float humidity = dht.readHumidity() + humidityoffset;

        lcd.setCursor(0, 0);
        lcd.print("Temp:");
        lcd.print(temperature, 1);
        lcd.print("C SV:");
        lcd.print(targetTemp, 1);
        lcd.print("C");

        lcd.setCursor(0, 1);
        lcd.print("Hum:");
        lcd.print(humidity, 1);
        lcd.print("% SV:");
        lcd.print(targetHumidity, 1);
        lcd.print("%");

        // Display operational status indicators
        lcd.setCursor(0, 2);
        lcd.print(digitalRead(RELAY_HEATING) == LOW ? "H " : " ");
        lcd.print(digitalRead(RELAY_COOLING) == LOW ? "C " : " ");
        lcd.print(digitalRead(RELAY_ADDITIONAL_HEATING) == LOW ? "A " : " ");
        lcd.print(digitalRead(RELAY_HUMIDITY) == LOW ? "Hum " : " ");
        lcd.print(digitalRead(RELAY_TURN_LEFT) == LOW ? "L " : " ");
        lcd.print(digitalRead(RELAY_TURN_RIGHT) == LOW ? "R " : " ");
        lcd.print(digitalRead(RELAY_LIGHT) == LOW ? "W " : " ");
        lcd.setCursor(0, 3);
        lcd.print("WiFi: ");
        lcd.print(WiFi.isConnected() ? "Connected" : "Disconnected");
        // Publish sensor data to Blynk widgets
        Blynk.virtualWrite(V0, temperature);    // Send temperature to Blynk app (Virtual Pin V1)
        Blynk.virtualWrite(V1, humidity);       // Send humidity to Blynk app (Virtual Pin V2)
        Blynk.virtualWrite(V3, targetTemp);     // Display target temperature (Virtual Pin V2)
        Blynk.virtualWrite(V2, targetHumidity); // Display target humidity (Virtual Pin V3)
    }
}

bool debounce(int pin)
{
    bool state;
    bool previousState = digitalRead(pin);
    for (int counter = 0; counter < 10; counter++)
    {
        delay(1);
        state = digitalRead(pin);
        if (state != previousState)
        {
            counter = 0;
            previousState = state;
        }
    }
    return state;
}

void displayMenu()
{
    bool exitMenu = false;

    while (!exitMenu)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        switch (menu)
        {
        case 0:
            lcd.print(">Set Temperature");
            break;
        case 1:
            lcd.print(">Set Humidity");
            break;
        case 2:
            lcd.print(">Set Temp Hysteresis");
            break;
        case 3:
            lcd.print(">Set Temp Offset");
            break;
        case 4:
            lcd.print(">View Status");
            break;
        case 5:
            lcd.print(">Manual Turning");
            break;
        case 6:
            lcd.print(">Factory Reset");
            break;
        case 7:
            lcd.print(">Adjust Add Heat");
            break;
        case 8:
            lcd.print(">Egg Turn Time");
            break;
        case 9:
            lcd.print(">Egg Turn Once");
            break;
        case 10:
            lcd.print(">Hum Calibration");
            break;
        case 11:
            lcd.print(">Last Email Status");
            break;
        case 12:
            lcd.print(">Test Relays");
            break;
        case 13:
            lcd.print(">Connect to Wifi");
            break;
        case 14:
            lcd.print(">Humidity Hysteresis");
            break;
        case 15:
            lcd.print(">EXIT");
            break;
        }

        bool upPressed = !debounce(BUTTON_UP);
        bool downPressed = !debounce(BUTTON_DOWN);
        bool selectPressed = !debounce(BUTTON_SELECT);

        if (upPressed)
        {
            menu = (menu > 0) ? menu - 1 : 15; // Update to include exit option
            delay(200);
        }

        if (downPressed)
        {
            menu = (menu < 15) ? menu + 1 : 0; // Update to include exit option
            delay(200);
        }

        if (selectPressed)
        {
            if (menu == 15)
            {
                exitMenu = true; // Exit loop
            }
            else
            {
                executeAction();
                displayDefaultScreen(); // Refresh screen after executing action
            }
            delay(200);
        }

        delay(100);
    }
    menu = 0;
}

void executeAction()
{
    switch (menu)
    {
    case 0:
        setTargetTemperature();
        break;
    case 1:
        setTargetHumidity();
        break;
    case 2:
        setHysteresis();
        break;
    case 3:
        setOffset();
        break;
    case 4:
        viewSystemStatus();
        break;
    case 5:
        manualEggTurning();
        break;
    case 6:
        factoryReset();
        break;
    case 7:
        adjustAdditionalHeatingThreshold();
        break;
    case 8:
        setEggTurningInterval();
        break;
    case 9:
        setEggTurningDuration();
        break;
    case 10:
        humidityCalibration();
        break;
    case 11:
        viewLastEmailStatus();
        break;
    case 12:
        testRelays();
        break;
    case 13:
        connectWiFi();
        break;
    case 14:
        setHumHysteresis();
        break;
    }
}

void setTargetTemperature()
{
    while (true)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Set Temp: ");
        lcd.print(targetTemp);
        lcd.print(" C");

        bool upPressed = !debounce(BUTTON_UP);
        bool downPressed = !debounce(BUTTON_DOWN);
        bool selectPressed = !debounce(BUTTON_SELECT);

        if (upPressed)
        {
            targetTemp += 0.1;
            delay(200);
        }

        if (downPressed)
        {
            targetTemp -= 0.1;
            delay(200);
        }

        if (selectPressed)
        {
            EEPROM.put(EEPROM_ADDR_TARGET_TEMP, targetTemp);
            EEPROM.commit();
            break;
        }

        delay(100);
    }
}

void setTargetHumidity()
{
    while (true)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Set Humidity: ");
        lcd.print(targetHumidity);
        lcd.print(" %");

        bool upPressed = !debounce(BUTTON_UP);
        bool downPressed = !debounce(BUTTON_DOWN);
        bool selectPressed = !debounce(BUTTON_SELECT);

        if (upPressed)
        {
            targetHumidity += 1.0;
            delay(200);
        }

        if (downPressed)
        {
            targetHumidity -= 1.0;
            delay(200);
        }

        if (selectPressed)
        {
            EEPROM.put(EEPROM_ADDR_TARGET_HUMIDITY, targetHumidity);
            EEPROM.commit();
            break;
        }

        delay(100);
    }
}

void setHysteresis()
{
    while (true)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Set Hyst: ");
        lcd.print(hysteresis, 1);
        lcd.print(" C");

        bool upPressed = !debounce(BUTTON_UP);
        bool downPressed = !debounce(BUTTON_DOWN);
        bool selectPressed = !debounce(BUTTON_SELECT);

        if (upPressed)
        {
            hysteresis += 0.1;
            delay(200);
        }

        if (downPressed)
        {
            hysteresis -= 0.1;
            delay(200);
        }

        if (selectPressed)
        {
            EEPROM.put(EEPROM_ADDR_HYSTERESIS, hysteresis);
            EEPROM.commit();
            break;
        }

        delay(100);
    }
}

void setOffset()
{
    while (true)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Set Offset: ");
        lcd.print(offset);
        lcd.print(" C");

        bool upPressed = !debounce(BUTTON_UP);
        bool downPressed = !debounce(BUTTON_DOWN);
        bool selectPressed = !debounce(BUTTON_SELECT);

        if (upPressed)
        {
            offset += 0.1;
            delay(200);
        }

        if (downPressed)
        {
            offset -= 0.1;
            delay(200);
        }

        if (selectPressed)
        {
            EEPROM.put(EEPROM_ADDR_CALIBRATION_OFFSET, offset);
            EEPROM.commit();
            break;
        }

        delay(100);
    }
}

void viewSystemStatus()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Max Temp:");
    lcd.print(maxTemp, 1);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("Min Temp:");
    lcd.print(minTemp, 1);
    lcd.print("C");

    lcd.setCursor(0, 2);
    lcd.print("Max Humidity: ");
    lcd.print(maxHumidity);
    lcd.print(" %");

    lcd.setCursor(0, 3);
    lcd.print("Min Humidity: ");
    lcd.print(minHumidity);
    lcd.print(" %");

    delay(3000); // Display for 3 seconds
}

void manualEggTurning()
{
    int option = 1; // Default option to Turn Left

    while (true)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Manual Egg Turn");
        lcd.setCursor(0, 1);

        // Display menu options based on current option
        switch (option)
        {
        case 1:
            lcd.print("> Turn Left     ");
            lcd.setCursor(0, 2);
            lcd.print("  Turn Right    ");
            lcd.setCursor(0, 3);
            lcd.print("  EXIT    ");
            break;
        case 2:
            lcd.print("  Turn Left     ");
            lcd.setCursor(0, 2);
            lcd.print("> Turn Right    ");
            lcd.setCursor(0, 3);
            lcd.print("  EXIT    ");
            break;
        case 3:
            lcd.print("  Turn Left     ");
            lcd.setCursor(0, 2);
            lcd.print("  Turn Right    ");
            lcd.setCursor(0, 3);
            lcd.print(" > EXIT    ");
            break;
        }

        bool selectPressed = !debounce(BUTTON_SELECT);
        bool upPressed = !debounce(BUTTON_UP);
        bool downPressed = !debounce(BUTTON_DOWN);

        if (selectPressed)
        {
            if (option == 1)
            {
                // Turn left relay on for 10 seconds
                digitalWrite(RELAY_TURN_LEFT, LOW);
                digitalWrite(RELAY_TURN_RIGHT, HIGH);
                delay(10000); // 10 seconds
                digitalWrite(RELAY_TURN_LEFT, HIGH);
                digitalWrite(RELAY_TURN_RIGHT, HIGH);
            }
            else if (option == 2)
            {
                // Turn right relay on for 10 seconds
                digitalWrite(RELAY_TURN_LEFT, HIGH);
                digitalWrite(RELAY_TURN_RIGHT, LOW);
                delay(10000); // 10 seconds
                digitalWrite(RELAY_TURN_LEFT, HIGH);
                digitalWrite(RELAY_TURN_RIGHT, HIGH);
            }
            else if (option == 3)
            {
                option = 1;
                break;
            }
        }

        if (upPressed)
        {
            option--;
            if (option < 1)
                option = 3; // Wrap around to the last option
            delay(200);
        }

        if (downPressed)
        {
            option++;
            if (option > 3)
                option = 1; // Wrap around to the first option
            delay(200);
        }

        delay(100);
    }

    // Turn off both relays before exiting
    digitalWrite(RELAY_TURN_LEFT, HIGH);
    digitalWrite(RELAY_TURN_RIGHT, HIGH);
}

void factoryReset()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Factory Reset");
    lcd.setCursor(0, 1);
    lcd.print("Confirm?");

    bool selectPressed = false;

    while (!selectPressed)
    {
        selectPressed = !debounce(BUTTON_SELECT);
        delay(100);
    }

    EEPROM.put(EEPROM_ADDR_TARGET_TEMP, 37.5);
    EEPROM.put(EEPROM_ADDR_TARGET_HUMIDITY, 60.0);
    EEPROM.put(EEPROM_ADDR_CALIBRATION_OFFSET, 0.0);
    EEPROM.put(EEPROM_ADDR_HYSTERESIS, 0.5);
    EEPROM.commit();

    targetTemp = 37.5;
    targetHumidity = 60.0;
    offset = 0.0;
    hysteresis = 0.5;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Settings Reset");
    delay(2000); // Display for 2 seconds
}

void adjustAdditionalHeatingThreshold()
{
    while (true)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Set Add Heat Th:");
        lcd.print(additional_heater_threshold);
        lcd.print(" C");

        bool upPressed = !debounce(BUTTON_UP);
        bool downPressed = !debounce(BUTTON_DOWN);
        bool selectPressed = !debounce(BUTTON_SELECT);

        if (upPressed)
        {
            additional_heater_threshold += 0.1;
            delay(200);
        }

        if (downPressed)
        {
            additional_heater_threshold -= 0.1;
            delay(200);
        }

        if (selectPressed)
        {
            break;
        }

        delay(100);
    }
}

void setEggTurningInterval()
{
    while (true)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Turn Int:");
        lcd.print(turnInterval / (60 * 1000)); // Convert milliseconds to minutes
        lcd.print(" min");

        bool upPressed = !debounce(BUTTON_UP);
        bool downPressed = !debounce(BUTTON_DOWN);
        bool selectPressed = !debounce(BUTTON_SELECT);

        if (upPressed)
        {
            turnInterval += 10 * 60 * 1000; // Increase by 10 minutes
            delay(200);
        }

        if (downPressed)
        {
            if (turnInterval > 0)
            {
                turnInterval -= 10 * 60 * 1000; // Decrease by 10 minutes
            }
            delay(200);
        }

        if (selectPressed)
        {
            break;
        }

        delay(100);
    }
}

void setEggTurningDuration()
{
    while (true)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Turn Time:");
        lcd.print(turnDuration / 1000); // Convert milliseconds to seconds
        lcd.print(" sec");

        bool upPressed = !debounce(BUTTON_UP);
        bool downPressed = !debounce(BUTTON_DOWN);
        bool selectPressed = !debounce(BUTTON_SELECT);

        if (upPressed)
        {
            turnDuration += 60 * 1000; // Increase by 10 seconds
            delay(200);
        }

        if (downPressed)
        {
            if (turnDuration > 0)
            {
                turnDuration -= 60 * 1000; // Decrease by 10 seconds
            }
            delay(200);
        }

        if (selectPressed)
        {
            break;
        }

        delay(100);
    }
}

void humidityCalibration()
{
    while (true)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Calibrate Hum:");
        lcd.print(humidityoffset);
        lcd.print(" C");

        bool upPressed = !debounce(BUTTON_UP);
        bool downPressed = !debounce(BUTTON_DOWN);
        bool selectPressed = !debounce(BUTTON_SELECT);

        if (upPressed)
        {
            humidityoffset += 1;
            delay(200);
        }

        if (downPressed)
        {
            humidityoffset -= 1;
            delay(200);
        }

        if (selectPressed)
        {
            break;
        }

        delay(100);
    }
}

void viewLastEmailStatus()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Last Email:");
    lcd.setCursor(0, 1);
    lcd.print(lastEmailStatus);
    delay(3000); // Display for 3 seconds
}

void testRelays()
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Testing Relays...");

    // Test all relays
    digitalWrite(RELAY_HUMIDITY, LOW);
    delay(1000);
    digitalWrite(RELAY_HUMIDITY, HIGH);
    delay(500);
    digitalWrite(RELAY_HEATING, LOW);
    delay(1000);
    digitalWrite(RELAY_HEATING, HIGH);
    delay(500);
    digitalWrite(RELAY_COOLING, LOW);
    delay(1000);
    digitalWrite(RELAY_COOLING, HIGH);
    delay(500);
    digitalWrite(RELAY_ADDITIONAL_HEATING, LOW);
    delay(1000);
    digitalWrite(RELAY_ADDITIONAL_HEATING, HIGH);
    delay(500);
    digitalWrite(RELAY_TURN_LEFT, LOW);
    delay(1000);
    digitalWrite(RELAY_TURN_LEFT, HIGH);
    delay(500);
    digitalWrite(RELAY_TURN_RIGHT, LOW);
    delay(1000);
    digitalWrite(RELAY_TURN_RIGHT, HIGH);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Relay Test Done");
    delay(2000); // Display for 2 seconds
}

void connectWiFi()
{
    int status = WL_IDLE_STATUS;
    int attempts = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    for (int i = 0; i < numNetworks; i++)
    {
        WiFi.begin(ssid[i], password[i]);
        unsigned long startAttemptTime = millis();

        // Try to connect to WiFi for 10 seconds
        while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000)
        {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            lcd.setCursor(0, 1);
            lcd.print("");
            lcd.setCursor(0, 2);

            lcd.print("WiFi connected");
            lcd.setCursor(0, 3);
            lcd.print("IP address: ");
            lcd.print(WiFi.localIP());
            return;
        }
    }
    lcd.setCursor(0, 2);
    lcd.print("");
    lcd.print("Failed to connect");
    lcd.clear();
}

void controlTemperature()
{
    // Read temperature from sensor
    sensors.requestTemperatures();
    float temperature = sensors.getTempCByIndex(0) + offset;

    // Update max and min values (assuming these are defined globally)
    if (temperature > maxTemp)
        maxTemp = temperature;
    if (temperature < minTemp)
        minTemp = temperature;

    // Logic for heater control
    if (temperature <= targetTemp - hysteresis)
    {
        digitalWrite(RELAY_COOLING, HIGH);
        digitalWrite(RELAY_HEATING, LOW);
        heatingstatus = true;
        coolingstatus = false;
        optimumtemperaturereached = false;
        if (temperature <= targetTemp - additional_heater_threshold)
        {
            digitalWrite(RELAY_ADDITIONAL_HEATING, LOW);
        }
    }
    else if (temperature >= targetTemp && heatingstatus == true)
    {
        digitalWrite(RELAY_HEATING, HIGH);
        digitalWrite(RELAY_ADDITIONAL_HEATING, HIGH);
        heatingstatus = false;
        optimumtemperaturereached = true;
    }
    else if (temperature >= targetTemp + hysteresis)
    {
        digitalWrite(RELAY_HEATING, HIGH);
        digitalWrite(RELAY_ADDITIONAL_HEATING, HIGH);
        digitalWrite(RELAY_COOLING, LOW);
        coolingstatus = true;
        heatingstatus = false;
        optimumtemperaturereached = false;
    }
    else if (temperature <= targetTemp && coolingstatus == true)
    {
        digitalWrite(RELAY_COOLING, HIGH);
        coolingstatus = false;
        optimumtemperaturereached = true;
    }

    if (!optimumtemperaturereached)
    {
        email_condition_met = true;
    }
    else
    {
        digitalWrite(RELAY_HEATING, HIGH);
        digitalWrite(RELAY_ADDITIONAL_HEATING, HIGH);
        digitalWrite(RELAY_COOLING, HIGH);
        email_condition_met = false;
    }
}

void controlHumidity()
{
    float humidity = dht.readHumidity() + humidityoffset;

    // Update max and min values
    if (humidity > maxHumidity)
        maxHumidity = humidity;
    if (humidity < minHumidity)
        minHumidity = humidity;
    if (humidity < targetHumidity - humidityhysteresis)
    {
        digitalWrite(RELAY_HUMIDITY, LOW);
    }
    else if (humidity >= targetHumidity)
    {
        digitalWrite(RELAY_HUMIDITY, HIGH);
    }
    else if (isnan(humidity))
    {
        digitalWrite(RELAY_HUMIDITY, HIGH);
    }
    else
    {
        digitalWrite(RELAY_HUMIDITY, LOW);
    }
}

void turnEggs(unsigned long currentMillis)
{
    if (currentMillis - lastTurnTime >= turnInterval)
    {
        // Turn eggs
        if (turnLeft)
        {
            digitalWrite(RELAY_TURN_LEFT, LOW);
            delay(turnDuration);
            digitalWrite(RELAY_TURN_LEFT, HIGH);
            turnLeft = false;
        }
        else
        {
            digitalWrite(RELAY_TURN_RIGHT, LOW);
            delay(turnDuration);
            digitalWrite(RELAY_TURN_RIGHT, HIGH);
            turnLeft = true;
        }
        lastTurnTime = currentMillis; // Reset the timer
    }
}
void setHumHysteresis()
{
    while (true)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Set Hum Hys:");
        lcd.print(humidityhysteresis);
        lcd.print(" %");

        bool upPressed = !debounce(BUTTON_UP);
        bool downPressed = !debounce(BUTTON_DOWN);
        bool selectPressed = !debounce(BUTTON_SELECT);

        if (upPressed)
        {
            humidityhysteresis += 1.0;
            delay(200);
        }

        if (downPressed)
        {
            humidityhysteresis -= 1.0;
            delay(200);
        }

        if (selectPressed)
        {
            break;
        }

        delay(100);
    }
}

void sendEmail(const char *mess)
{
    EMailSender::EMailMessage message;
    message.subject = "Incubator Alert";
    message.message = mess;

    EMailSender::Response resp = emailSend.send("bkpoudel44@gmail.com", message);
    delay(4000); // delay 4s.
}
BLYNK_WRITE(V3)
{ // Target temperature slider (Virtual Pin V2)
    targetTemp = param.asFloat();
    EEPROM.put(EEPROM_ADDR_TARGET_TEMP, targetTemp);
    EEPROM.commit();
}

BLYNK_WRITE(V2)
{ // Target humidity slider (Virtual Pin V3)
    targetHumidity = param.asFloat();
    EEPROM.put(EEPROM_ADDR_TARGET_HUMIDITY, targetHumidity);
    EEPROM.commit();
}
void controlLight()
{
    Blynk.run();
}

BLYNK_WRITE(V4)
{
    int lightValue = param.asInt(); // Get slider value from Blynk app
    if (lightValue > 0)
    {
        // Turn on light (assuming RELAY_LIGHT is your relay pin for the light)
        digitalWrite(RELAY_LIGHT, LOW); // Assuming LOW activates the relay
    }
    else
    {
        // Turn off light
        digitalWrite(RELAY_LIGHT, HIGH); // Assuming HIGH deactivates the relay
    }
}