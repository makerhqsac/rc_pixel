#include <EnableInterrupt.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif


/* rc configuration */
#define RC_PULSE_CENTER         1500 // pulse width (microseconds) for RC center
#define PULSE_WIDTH_DEADBAND    25 // pulse width difference from RC_PULSE_CENTER us (microseconds) to ignore (to compensate trim and drift)
#define RC_PULSE_TIMEOUT        500 // in milliseconds
// uncomment to switch left/right signal from RC
//#define RC_REVERSE

#define RC_TURN_CHANNEL         0
#define RC_TURN_INPUT           3

#define RC_NUM_CHANNELS         1


/* led strip configuration */
#define LED_COLOR               Adafruit_NeoPixel::Color(0, 255, 0)
#define LED_COLOR_NO_RC         Adafruit_NeoPixel::Color(0, 0, 50)
#define LED_ANIM_TIME           200 // in milliseconds
#define LED_COUNT               10
#define LED_TYPE                NEO_GRB + NEO_KHZ800

// uncomment to switch logic from the left side being first to the right side being first
//#define LED_RIGHT_FIRST

#define LED_OUTPUT              4


enum state_t {
    STATE_IDLE,
    STATE_LEFT,
    STATE_RIGHT,
    STATE_NO_RC
};

uint16_t rc_values[RC_NUM_CHANNELS];
uint32_t rc_start[RC_NUM_CHANNELS];
volatile uint16_t rc_shared[RC_NUM_CHANNELS];

Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, LED_OUTPUT, LED_TYPE);

long last_rc_pulse = 0;
long anim_start = 0;
uint16_t rc_turn_pulse = 0;
state_t state = STATE_IDLE;

void animate_turn_left();
void animate_turn_right();
void animate_leds(uint16_t start);
void animate_no_rc();

void rc_read_values() {
    noInterrupts();
    memcpy(rc_values, (const void *)rc_shared, sizeof(rc_shared));
    interrupts();

    rc_turn_pulse = (int)rc_values[RC_TURN_CHANNEL];

    if (millis() - last_rc_pulse > RC_PULSE_TIMEOUT) {
        rc_turn_pulse = 0;
    }
}

void calc_input(uint8_t channel, uint8_t input_pin) {
    if (digitalRead(input_pin) == HIGH) {
        rc_start[channel] = (uint32_t )micros();
    } else {
        uint16_t rc_compare = (uint16_t)(micros() - rc_start[channel]);
        rc_shared[channel] = rc_compare;
    }
    last_rc_pulse = millis();
}

void calc_turn() { calc_input(RC_TURN_CHANNEL, RC_TURN_INPUT); }

void setup() {
    enableInterrupt(RC_TURN_INPUT, calc_turn, CHANGE);

    leds.begin();
}

void loop() {
    rc_read_values();

    // turn right
    if (rc_turn_pulse > RC_PULSE_CENTER + PULSE_WIDTH_DEADBAND) {
#ifdef RC_REVERSE
        animateTurnLeft();
        state = STATE_LEFT;
#else
        animate_turn_right();
        state = STATE_RIGHT;
#endif

    // turn left
    } else if (rc_turn_pulse > 0 && rc_turn_pulse < RC_PULSE_CENTER - PULSE_WIDTH_DEADBAND) {
#ifdef RC_REVERSE
        animateTurnRight();
        state = STATE_RIGHT;
#else
        animate_turn_left();
        state = STATE_LEFT;
#endif

    // no r/c signal
    } else if (rc_turn_pulse <= 0) {
        state = STATE_NO_RC;
        animate_no_rc();

    } else {
        state = STATE_IDLE;
        anim_start = 0;
        leds.clear();
    }
    leds.show();
}

void animate_no_rc() {
    static boolean leds_on = false;

    if (anim_start > 0) {
        if (anim_start + LED_ANIM_TIME > millis()) {
            anim_start = 0;
            if (leds_on) {
                leds.clear();
            } else {
                for (uint16_t i = 0; i < LED_COUNT; i++) {
                    leds.setPixelColor(i, LED_COLOR_NO_RC);
                }
            }
            leds_on = !leds_on;
        }
    } else {
        anim_start = millis();
    }
}

void animate_turn_right() {
#ifdef LED_RIGHT_FIRST
    uint16_t start = 0;
#else
    uint16_t start = LED_COUNT / 2;
#endif
    animate_leds(start);
}

void animate_turn_left() {
#ifdef LED_RIGHT_FIRST
    uint16_t start = LED_COUNT / 2;
#else
    uint16_t start = 0;
#endif
    animate_leds(start);
}

void animate_leds(uint16_t start) {
    if (anim_start > 0) {
        if (anim_start + LED_ANIM_TIME > millis()) {
            anim_start = 0;
            leds.clear();
        } else {
            uint16_t led_count = (uint16_t)map(millis() - anim_start, anim_start, millis(), 0, LED_COUNT/2);
            for (uint16_t i = 0; i < led_count; i++) {
                leds.setPixelColor(start + i, LED_COLOR);
            }
        }
    } else {
        anim_start = millis();
    }
}