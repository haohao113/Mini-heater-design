#include <mega16.h>
#include <delay.h>
#include <alcd.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define F_CPU 8000000UL

#define ADC_VREF_TYPE ((0 << REFS1) | (1 << REFS0) | (0 << ADLAR))

// Pin definitions
#define BUTTON1 0
#define BUTTON2 2
#define BUTTON3 3
#define BUTTON4 1

#define RELAY1 4
#define RELAY2 5

// Default values
#define DEFAULT_TEMPERATURE_THRESHOLD 30

#define TEMP_PIN 0 // C?m bi?n nhi?t d? k?t n?i t?i ch�n A0

#define TEMP_MIN 30 // Minimum temperature for fan to start
#define TEMP_MAX 40 // Maximum temperature for fan to run at full speed

char buffer_temp[16];

// Global variables
unsigned char mode = 0; // 0 for manual, 1 for automatic
unsigned int temp_value;
unsigned int temp_min = TEMP_MIN, temp_max = TEMP_MAX;
int temp_int;
int auto_temperature_threshold = DEFAULT_TEMPERATURE_THRESHOLD;

void ADC_init()
{
  // Ch?n AVCC l�m di?n �p tham chi?u, kh�ng k�ch ho?t ch?c nang left adjust
  ADMUX = (1 << REFS0);

  // K�ch ho?t ADC, kh�ng ch?n ch? d? chuy?n d?i li�n t?c
  ADCSRA = (1 << ADEN);

  // Ch?n ch? d? chia t? l? 64 (8MHz / 64 = 125kHz)
  ADCSRA |= (1 << ADPS2) | (1 << ADPS1);
}

unsigned int read_adc(unsigned char adc_input)
{
  // Switch ADC channel
  ADMUX = (ADMUX & 0xF8) | (adc_input & 0x07);
  // Wait for channel switch to take effect
  delay_us(10);
  // Start a new AD conversion
  ADCSRA |= (1 << ADSC);
  // Wait for the conversion to complete
  while ((ADCSRA & (1 << ADIF)) == 0)
    ;
  // Clear the ADIF bit
  ADCSRA |= (1 << ADIF);
  return ADCW;
}

unsigned int map(unsigned int x, unsigned int in_min, unsigned int in_max, unsigned int out_min, unsigned int out_max)
{
  return ((long)(x - in_min) * (long)(out_max - out_min)) / (in_max - in_min) + out_min;
}

float temperature;
float R1 = 10000;
float logR2, R2, T, temp;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;

float get_temp()
{
  float Vo = read_adc(TEMP_PIN);
  R2 = R1 * (1023.0 / Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2 * logR2 + c3 * logR2 * logR2 * logR2));
  return T - 273.15; // d? C
}

void main(void)
{

  // Initialize heater pin as output
  DDRD |= (1 << RELAY1);
  PORTD |= (1 << RELAY1);
  DDRD |= (1 << RELAY2);
  PORTD |= (1 << RELAY2);

  // Initialize button pins as input
  DDRD &= ~((1 << BUTTON1) | (1 << BUTTON2) | (1 << BUTTON3) | (1 << BUTTON4));
  PORTD |= ((1 << BUTTON1) | (1 << BUTTON2) | (1 << BUTTON3) | (1 << BUTTON4));

  // Initialize LCD
  lcd_init(16);
  ADC_init();

  while (1)
  {
    if (!(PIND & (1 << BUTTON1)))
    {
      mode++;
      if (mode > 2)
        mode = 0;
      while (!(PIND & (1 << BUTTON1)))
        ; // Wait until the button is released
    }

    if (mode == 0)
    {
      lcd_clear();
      lcd_gotoxy(5, 0);
      lcd_puts("Manual");

      temperature = get_temp();
      temp_int = (int)temperature;
      sprintf(buffer_temp, "Temp: %d\xDF"
                           "C",
              temp_int);
      lcd_gotoxy(0, 1);
      lcd_puts(buffer_temp);
      // If the manual button is pressed, toggle the heater
      if (!(PIND & (1 << BUTTON4)))
      {
        PORTD ^= (1 << RELAY1);
        while (!(PIND & (1 << BUTTON4)))
          ; // Wait until the button is released
      }
      if (!(PIND & (1 << BUTTON3)))
      {
        PORTD ^= (1 << RELAY2);
        while (!(PIND & (1 << BUTTON3)))
          ; // Wait until the button is released
      }
    }
    else if (mode == 1)
    {
      lcd_clear();
      lcd_gotoxy(6, 0);
      lcd_puts("Auto");

      // Read temperature from sensor
      temperature = get_temp();
      temp_int = (int)temperature;

      // Display temperature on LCD
      sprintf(buffer_temp, "T: %d\xDF"
                           "C",
              temp_int);
      lcd_gotoxy(0, 1);
      lcd_puts(buffer_temp);

      sprintf(buffer_temp, "M: %d\xDF"
                           "C",
              auto_temperature_threshold);
      lcd_gotoxy(8, 1);
      lcd_puts(buffer_temp);

      // If temperature is above the auto level, turn off the heater
      if (temp_int > auto_temperature_threshold)
      {
        PORTD |= (1 << RELAY1);
        PORTD &= ~(1 << RELAY2);
      }
      // If temperature is below the auto level, turn on the heater
      else if (temp_int < auto_temperature_threshold)
      {
        PORTD &= ~(1 << RELAY1);
        PORTD &= ~(1 << RELAY2);
      }
    }
    else
    {
      lcd_clear();
      lcd_gotoxy(0, 0);
      lcd_puts("Setting");

      // Display auto_temperature_threshold on LCD
      sprintf(buffer_temp, "Threshold: %d\xDF"
                           "C",
              auto_temperature_threshold);
      lcd_gotoxy(0, 1);
      lcd_puts(buffer_temp);

      // Adjust the auto temperature threshold
      if (!(PIND & (1 << BUTTON2)))
      {
        auto_temperature_threshold++;
        delay_ms(100);
        //                while (!(PIND & (1 << BUTTON2)))
        //                    ; // Wait until the button is released
      }
      if (!(PIND & (1 << BUTTON3)))
      {
        auto_temperature_threshold--;
        delay_ms(100);
        //                while (!(PIND & (1 << BUTTON3)))
        //                    ; // Wait until the button is released
      }
    }

    delay_ms(100);
  }
}