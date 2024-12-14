# Chicken Egg Incubator
Bibek Poudel
This project is for creating a chicken egg incubator using PlatformIO.

## Libraries Used/Required

- `DHT.h`: For reading temperature and humidity from DHT sensors. [Download](https://github.com/adafruit/DHT-sensor-library)
- `Servo.h`: For controlling the servo motor. [Download](https://github.com/arduino-libraries/Servo)
- `LiquidCrystal.h`: For controlling the LCD display. [Download](https://github.com/arduino-libraries/LiquidCrystal)
- `Wire.h`: For I2C communication with sensors and the LCD display. [Download](https://github.com/arduino/ArduinoCore-avr/tree/master/libraries/Wire)
- `Adafruit_Sensor.h`: Unified sensor library for interfacing with various sensors. [Download](https://github.com/adafruit/Adafruit_Sensor)

## Features

- Temperature control
- Humidity control
- Automatic egg turning
- LCD display for status

## Requirements

- PlatformIO
- Arduino or compatible microcontroller
- Temperature and humidity sensors
- Servo motor for egg turning
- LCD display

## Installation

1. Clone this repository.
    ```sh
    git clone https://github.com/yourusername/chicken-egg-incubator.git
    ```
2. Open the project with PlatformIO.
3. Upload the code to your microcontroller.

## Usage

1. Connect the sensors and actuators to the microcontroller as per the schematic.
2. Power on the system.
3. Monitor the status on the LCD display.

## Contributing

Contributions are welcome! Please open an issue or submit a pull request.

## License

This project is licensed under the MIT License.

## Schematic

Please refer to the `schematic.png` file in the repository for the wiring diagram.

## Code Overview

The code is organized into several files:

- `main.cpp`: The main program file that initializes the system and contains the main loop.
- `temperature_control.cpp`: Contains functions for reading the temperature sensor and controlling the heating element.
- `humidity_control.cpp`: Contains functions for reading the humidity sensor and controlling the humidifier.
- `egg_turning.cpp`: Contains functions for controlling the servo motor to turn the eggs.
- `lcd_display.cpp`: Contains functions for updating the LCD display with the current status.

## Troubleshooting

If you encounter any issues, please check the following:

- Ensure all connections are secure and correct as per the schematic.
- Verify that the sensors and actuators are working properly.
- Check the serial monitor for any error messages.

## Support

If you need help with this project, please open an issue on GitHub or contact the project maintainer.

## Acknowledgements

- Thanks to the PlatformIO community for their support and resources.
- Special thanks to the contributors who helped improve this project.

## References

- [PlatformIO Documentation](https://docs.platformio.org/)
- [Arduino Documentation](https://www.arduino.cc/en/Guide/HomePage)
- [MIT License](https://opensource.org/licenses/MIT)