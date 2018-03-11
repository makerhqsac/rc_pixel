#include <EnableInterrupt.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif


/* rc configuration */
#define RC_PULSE_CENTER         1500 // pulse width (microseconds) for RC center
#define PULSE_WIDTH_DEADBAND    100 // pulse width difference from RC_PULSE_CENTER us (microseconds) to ignore (to compensate trim and drift)
#define RC_PULSE_TIMEOUT        1000 // in milliseconds

#define RC_INPUT_CHANNEL        0
#define RC_INPUT_PIN            3 // arduino pin connected to R/C receiver

#define RC_NUM_CHANNELS         1


/* led strip configuration */
#define LED_COLOR_ON            Adafruit_NeoPixel::Color(0, 255, 0)
#define LED_COLOR_OFF           Adafruit_NeoPixel::Color(0, 0, 0)
#define LED_COLOR_NO_RC         Adafruit_NeoPixel::Color(255, 0, 0)
#define LED_COUNT               10
#define LED_TYPE                NEO_GRB + NEO_KHZ800

#define LED_SHOW_INTERVAL       20

#define LED_PIN                 4 // arduino pin with leds


enum state_t {
    STATE_ON,
    STATE_OFF,
    STATE_NO_RC
};

uint16_t rc_values[RC_NUM_CHANNELS];
uint32_t rc_start[RC_NUM_CHANNELS];
volatile uint16_t rc_shared[RC_NUM_CHANNELS];

Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, LED_PIN, LED_TYPE);

volatile boolean rc_process = false;
volatile long last_rc_pulse = 0;
volatile uint16_t rc_input = 0;
state_t state = STATE_OFF;
long show_time = millis();


void disable_rc_processing();
void enable_rc_processing();
boolean is_rc_left();
boolean is_rc_right();
boolean is_rc_ok();
void setStripColor(uint32_t color);
void run_state_on();
void run_state_off();
void run_state_no_rc();

void rc_read_values() {
  noInterrupts();
  memcpy(rc_values, (const void *)rc_shared, sizeof(rc_shared));
  interrupts();

  rc_input = (uint16_t)rc_values[RC_INPUT_CHANNEL];

  if (millis() - last_rc_pulse > RC_PULSE_TIMEOUT) {
    rc_input = 0;
  }
}

void calc_pulse(uint8_t channel, uint8_t input_pin) {
  if (rc_process) {
    if (digitalRead(input_pin) == HIGH) {
      rc_start[channel] = (uint32_t )micros();
    } else {
      if (rc_start[channel] > 0) {
        uint16_t usecs = (uint16_t) (micros() - rc_start[channel]);
        if (usecs > 900 && usecs < 2100) {
          rc_shared[channel] = usecs; // only use sane value
        }

      }

    }
    last_rc_pulse = millis();
  }
}

void calc_rc_input() { calc_pulse(RC_INPUT_CHANNEL, RC_INPUT_PIN); }

void setup() {
  enableInterrupt(RC_INPUT_PIN, calc_rc_input, CHANGE);

#if defined (__AVR_ATtiny85__)
  if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif

  leds.begin();
  
  state = STATE_OFF;
}

void loop() {
  rc_read_values();

  if (rc_input <= 0) {
    state = STATE_NO_RC;
  }

  switch(state) {
    case STATE_ON:
      run_state_on();
          break;
    case STATE_OFF:
      run_state_off();
          break;
    case STATE_NO_RC:
      run_state_no_rc();
          break;
  }

  if (show_time + LED_SHOW_INTERVAL < millis()) {
    disable_rc_processing();
    leds.show();
    show_time = millis();
    enable_rc_processing();
  }
}

void disable_rc_processing() {
  rc_process = false;
  for (int i = 0; i < RC_NUM_CHANNELS; i++) {
    rc_start[i] = 0;
  }
}

void enable_rc_processing() {
  rc_process = true;
}


boolean is_rc_left() {
    return rc_input > 0 && rc_input < (RC_PULSE_CENTER - PULSE_WIDTH_DEADBAND);
}

boolean is_rc_right() {
    return rc_input > (RC_PULSE_CENTER + PULSE_WIDTH_DEADBAND);
}

boolean is_rc_ok() {
    return rc_input > 0;
}

void run_state_on() {
  setStripColor(LED_COLOR_ON);

  if (is_rc_left()) {
    state = STATE_OFF;
  }
}

void run_state_off() {
  setStripColor(LED_COLOR_OFF);

  if (is_rc_right()) {
    state = STATE_ON;
  }
}

void run_state_no_rc() {
  setStripColor(LED_COLOR_NO_RC);

  if (is_rc_ok()) {
    state = STATE_OFF;
  }
}

void setStripColor(uint32_t color) {
  for (uint16_t i = 0; i < LED_COUNT; i++) {
    leds.setPixelColor(i, color);
  }
}
