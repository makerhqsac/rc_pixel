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
#define LED_COLOR               Adafruit_NeoPixel::Color(0, 0, 40)
#define LED_COLOR_NO_RC         Adafruit_NeoPixel::Color(30, 0, 0)
#define LED_ANIM_TIME           800 // in milliseconds
#define LED_COUNT               10
#define LED_TYPE                NEO_GRB + NEO_KHZ800

#define LED_SHOW_INTERVAL       20

                                // swap these values if you need to reverse the lights
#define LED_LEFT_START          LED_COUNT/2
#define LED_RIGHT_START         LED_COUNT/2
#define LED_LEFT_REVERSE
//#define LED_RIGHT_REVERSE

#define LED_PIN                 4 // arduino pin with leds


enum state_t {
    STATE_IDLE,
    STATE_LEFT,
    STATE_RIGHT,
    STATE_NO_RC
};

uint16_t rc_values[RC_NUM_CHANNELS];
uint32_t rc_start[RC_NUM_CHANNELS];
volatile uint16_t rc_shared[RC_NUM_CHANNELS];

Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, LED_PIN, LED_TYPE);

volatile long last_rc_pulse = 0;
long anim_start = 0;
long debug_time = millis();
long show_time = millis();

volatile boolean rc_process = false;
volatile uint16_t rc_input = 0;
state_t state = STATE_IDLE;

void disable_rc_processing();
void enable_rc_processing();
void animate_leds(uint16_t start);
void run_state_idle();
void run_state_left();
void run_state_right();
void run_state_no_rc();
boolean is_rc_left();
boolean is_rc_right();
boolean is_rc_ok();

void rc_read_values() {
    noInterrupts();
    memcpy(rc_values, (const void *)rc_shared, sizeof(rc_shared));
    interrupts();

    rc_input = (int)rc_values[RC_INPUT_CHANNEL];

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
//    Serial.begin(9600);

    enableInterrupt(RC_INPUT_PIN, calc_rc_input, CHANGE);

#if defined (__AVR_ATtiny85__)
    if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif

    leds.begin();
}

void loop() {
    rc_read_values();

//    if (debug_time + 1000 < millis()) {
//        Serial.print("r/c: ");
//        Serial.print(rc_turn_pulse);
//        Serial.println("");
//        debug_time = millis();
//    }

    if (!is_rc_ok()) {
        state = STATE_NO_RC;
    }

    switch(state) {
        case STATE_IDLE:
            run_state_idle();
            break;
        case STATE_RIGHT:
            run_state_right();
            break;
        case STATE_LEFT:
            run_state_left();
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


void run_state_idle() {
    leds.clear();
    anim_start = 0;

    if (is_rc_left()) {
        state = STATE_LEFT;
    } else if (is_rc_right()) {
        state = STATE_RIGHT;
    }
}

void run_state_left() {
#ifdef LED_LEFT_REVERSE
  animate_leds(LED_LEFT_START, true); 
#else
  animate_leds(LED_LEFT_START, false);
#endif
    

    if (is_rc_right()) {
        state = STATE_RIGHT;
    } else if (is_rc_left()) {
        state = STATE_LEFT;
    } else {
        state = STATE_IDLE;
    }
}

void run_state_right() {
#ifdef LED_RIGHT_REVERSE
  animate_leds(LED_RIGHT_START, true);
#else
  animate_leds(LED_RIGHT_START, false);
#endif

    if (is_rc_right()) {
        state = STATE_RIGHT;
    } else if (is_rc_left()) {
        state = STATE_LEFT;
    } else {
        state = STATE_IDLE;
    }
}

void run_state_no_rc() {
    for (uint16_t i = 0; i < LED_COUNT; i++) {
        leds.setPixelColor(i, LED_COLOR_NO_RC);
    }

    if (is_rc_ok()) {
        state = STATE_IDLE;
    }
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


void animate_leds(uint16_t start, boolean reverse) {

    leds.clear();

    long now = millis();

    if (anim_start + LED_ANIM_TIME < now) {
        anim_start = now;
    }

    uint16_t led_count = (uint16_t)map(now - anim_start, 0, LED_ANIM_TIME, 1, LED_COUNT/2 + 1);
    
    if (reverse) {
      for (uint16_t i = start; i > start - led_count; i--) {
        leds.setPixelColor(i-1, LED_COLOR);
      }
    } else {
      for (uint16_t i = 0; i < led_count; i++) {
        leds.setPixelColor(start + i, LED_COLOR);
      }
    }
    
}

