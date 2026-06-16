#include <Arduino.h>
#include <Servo.h>

#include <Adafruit_NeoPixel.h>

#include <Bluewhale.h>
#include <jiangtun.h>

static const pin_size_t PIN_GAMECUBE = 0; // GP0 (RP2040-One)
static const pin_size_t PIN_SERVO = 18;   // unused (parked on a free GPIO)
static const pin_size_t PIN_RESET = 19;   // unused (parked on a free GPIO)

static const pin_size_t PIN_NEOPIXEL = 16; // WS2812 RGB LED (RP2040-One)

static CGamecubeConsole gamecube(PIN_GAMECUBE);
static Servo servo;
static Gamecube_Data_t gamecube_data = defaultGamecubeData;
static bool gamecube_data_reset = false;
static mutex_t gamecube_data_mtx;
static bool current_reset_state = true; // to ensure initial releasing
static bool write_at_least_once = false;
static mutex_t write_at_least_once_mtx;

static Adafruit_NeoPixel pixels(1, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
static jiangtun_board_t board;
static jiangtun_t j;

static jiangtun_bool_t serial_getc(jiangtun_board_t *board, unsigned char *c) {
    assert(board != NULL);
    assert(c != NULL);

    if (Serial.available() <= 0) {
        return JIANGTUN_FALSE;
    }
    int c_ = Serial.read();
    if (c_ < 0 || 255 < c_) {
        return JIANGTUN_FALSE;
    }
    *c = c_ & 0xFF;
    return JIANGTUN_TRUE;
}

static void serial_puts(jiangtun_board_t *board, const char *s) {
    assert(board != NULL);
    assert(s != NULL);

    Serial.print(s);
}

static jiangtun_bool_t gamecube_send(jiangtun_board_t *board,
                                     jiangtun_bool_t changed,
                                     jiangtun_report_mode3_t *report) {
    assert(board != NULL);
    assert(report != NULL);

    bool is_initialized = mutex_is_initialized(&write_at_least_once_mtx);
    if (is_initialized) {
        bool _write_at_least_once;
        do {
            mutex_enter_blocking(&write_at_least_once_mtx);
            _write_at_least_once = write_at_least_once;
            mutex_exit(&write_at_least_once_mtx);
        } while (!_write_at_least_once);
    }

    if (changed) {
        mutex_enter_blocking(&gamecube_data_mtx);
        gamecube_data.report.a = report->a ? 1 : 0;
        gamecube_data.report.b = report->b ? 1 : 0;
        gamecube_data.report.x = report->x ? 1 : 0;
        gamecube_data.report.y = report->y ? 1 : 0;
        gamecube_data.report.start = report->start ? 1 : 0;
        gamecube_data.report.dleft = report->dleft ? 1 : 0;
        gamecube_data.report.dright = report->dright ? 1 : 0;
        gamecube_data.report.ddown = report->ddown ? 1 : 0;
        gamecube_data.report.dup = report->dup ? 1 : 0;
        gamecube_data.report.z = report->z ? 1 : 0;
        gamecube_data.report.r = report->r ? 1 : 0;
        gamecube_data.report.l = report->l ? 1 : 0;
        gamecube_data.report.xAxis = (uint8_t)report->xAxis;
        gamecube_data.report.yAxis = (uint8_t)report->yAxis;
        gamecube_data.report.cxAxis = (uint8_t)report->cxAxis;
        gamecube_data.report.cyAxis = (uint8_t)report->cyAxis;
        gamecube_data.report.left = (uint8_t)report->left;
        gamecube_data.report.right = (uint8_t)report->right;

        gamecube_data_reset = report->reset ? true : false;
        mutex_exit(&gamecube_data_mtx);

        if (!is_initialized) {
            write_at_least_once = false;
            mutex_init(&write_at_least_once_mtx);
        } else {
            mutex_enter_blocking(&write_at_least_once_mtx);
            write_at_least_once = false;
            mutex_exit(&write_at_least_once_mtx);
        }
    }

    return JIANGTUN_TRUE;
}

static void led_set(jiangtun_board_t *board, jiangtun_bool_t state) {
    assert(board != NULL);

    if (state) {
        uint16_t hue = (millis() % 2000) * (65535 / 2000);
        pixels.setPixelColor(0, Adafruit_NeoPixel::ColorHSV(hue));
    } else {
        pixels.clear();
    }
    pixels.show();
}

static jiangtun_uint32_t get_millis(jiangtun_board_t *board) {
    assert(board != NULL);

    return (jiangtun_uint32_t)(millis() % JIANGTUN_UINT32_MAX);
}

void setup() {
    Serial.begin(115200);

    pixels.begin();
    pixels.clear();
    pixels.show();

    jiangtun_board_init(&board, serial_getc, serial_puts, gamecube_send,
                        led_set, get_millis);
    mutex_init(&gamecube_data_mtx);
    jiangtun_init(
        &j, &board,
        JIANGTUN_FEATURE_ENABLE_LED_BLINK | JIANGTUN_FEATURE_ENABLE_NXMC2 |
            JIANGTUN_FEATURE_ENABLE_POKECON | JIANGTUN_FEATURE_ENABLE_ORCA,
#ifndef NDEBUG
        JIANGTUN_LOG_LEVEL_DEBUG
#else
        JIANGTUN_LOG_LEVEL_INFO
#endif
    );
}

void loop() { jiangtun_loop(&j); }

void setup1() {
    servo.attach(PIN_SERVO, 500, 2400);
    /* Do not start `loop1` until the first `gamecube_send` (press start) */
    while (!mutex_is_initialized(&write_at_least_once_mtx))
        ;
}

void loop1() {
    mutex_enter_blocking(&gamecube_data_mtx);
    bool ret = gamecube.write(gamecube_data);
    bool reset = gamecube_data_reset;
    mutex_exit(&gamecube_data_mtx);

    if (!ret) {
        Serial.println("[core2]\tfailed to send report");
        return;
    }

    if (!(current_reset_state) && reset) {
        servo.write(65);
        pinMode(PIN_RESET, OUTPUT);
        digitalWrite(PIN_RESET, LOW);
    } else if (current_reset_state && !reset) {
        servo.write(90);
        pinMode(PIN_RESET, INPUT);
    }
    current_reset_state = reset;

    mutex_enter_blocking(&write_at_least_once_mtx);
    write_at_least_once = true;
    mutex_exit(&write_at_least_once_mtx);
}