#include <Arduino.h>

#include <Wire.h>
#include "SSD1306Wire.h" 
#define OLED_CLASS_OBJ  SSD1306Wire
#define OLED_ADDRESS    0x3C
#define OLED_SDA    4
#define OLED_SCL    15
#define OLED_RST    16
#define CONFIG_MOSI 27
#define CONFIG_MISO 19
#define CONFIG_CLK  5
#define CONFIG_NSS  18
#define CONFIG_RST  14
#define CONFIG_DIO0 26
