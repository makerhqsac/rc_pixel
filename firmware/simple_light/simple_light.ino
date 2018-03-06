#include <EnableInterrupt.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif


/* rc configuration */
#define RC_PULSE_CENTER         1500 // pulse width (microseconds) for RC center
#define PULSE_WIDTH_DEADBAND    100 // pulse width difference from RC_PULSE_CENTER us (microseconds) to ignore (to compensate trim and drift)
#define RC_PULSE_TIMEOUT        500 // in milliseconds

#define RC_SWITCH_CHANNEL       0
#define RC_SWITCH_INPUT         3

#define RC_NUM_CHANNELS         1


/* led strip configuration */
#define LED_COLOR_ON            Adafruit_NeoPixel::Color(0, 255, 0)
#define LED_COLOR_OFF           Adafruit_NeoPixel::Color(255, 50, 0)
#define LED_COLOR_NOSIG         Adafruit_NeoPixel::Color(0, 0, 50)
#define LED_COUNT               10
#define LED_TYPE                NEO_GRB + NEO_KHZ800

#define LED_OUTPUT              4



uint16_t rc_values[RC_NUM_CHANNELS];
uint32_t rc_start[RC_NUM_CHANNELS];
volatile uint16_t rc_shared[RC_NUM_CHANNELS];

Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, LED_OUTPUT, LED_TYPE);

long last_rc_pulse = 0;
int rc_switch_pulse = 0;


void rc_read_values() {
  noInterrupts();
  memcpy(rc_values, (const void *)rc_shared, sizeof(rc_shared));
  interrupts();

  rc_switch_pulse = (int)rc_values[RC_SWITCH_CHANNEL];

  if (millis() - last_rc_pulse > RC_PULSE_TIMEOUT) {
    rc_switch_pulse = 0;
  }
}

void calc_input(uint8_t channel, uint8_t input_pin) {
  if (digitalRead(input_pin) == HIGH) {
    rc_start[channel] = micros();
  } else {
    uint16_t rc_compare = (uint16_t)(micros() - rc_start[channel]);
    rc_shared[channel] = rc_compare;
  }
  last_rc_pulse = millis();
}

void calc_switch() { calc_input(RC_SWITCH_CHANNEL, RC_SWITCH_INPUT); }

void setup() {
  enableInterrupt(RC_SWITCH_INPUT, calc_switch, CHANGE);

  leds.begin();
}

void loop() {
  rc_read_values();

  if (rc_switch_pulse > RC_PULSE_CENTER + PULSE_WIDTH_DEADBAND) {
    setStripColor(LED_COLOR_ON);
  } else if (rc_switch_pulse > 0 && rc_switch_pulse < RC_PULSE_CENTER - PULSE_WIDTH_DEADBAND) {
    setStripColor(LED_COLOR_OFF);
  } else {
    setStripColor(LED_COLOR_NOSIG);
  }
  leds.show();
}

void setStripColor(uint32_t color) {
  for (int i = 0; i < LED_COUNT; i++) {
    leds.setPixelColor(i, color);
  }
}
