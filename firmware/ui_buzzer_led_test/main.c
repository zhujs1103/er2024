/*
 * ER2024 external LED + active-low buzzer smoke test.
 *
 * Wiring used by this test:
 *   Buzzer module VCC -> 3.3 V
 *   Buzzer module GND -> GND
 *   Buzzer module I/O -> PA2
 *
 *   PA14 -> 1 kOhm resistor -> LED long leg/anode
 *   LED short leg/cathode -> GND
 *
 * The buzzer module is low-level triggered: PA2 high means silent, PA2 low
 * means beep. The external LED is active high.
 */

#include "ti_msp_dl_config.h"

#define UI_BUZZER_PORT  (GPIOA)
#define UI_BUZZER_PIN   (DL_GPIO_PIN_2)
#define UI_BUZZER_IOMUX (IOMUX_PINCM7)

#define UI_LED_PORT     (GPIOA)
#define UI_LED_PIN      (DL_GPIO_PIN_14)
#define UI_LED_IOMUX    (IOMUX_PINCM36)

#define BOARD_LED_PORT  (GPIO_LED_PORT)
#define BOARD_LED_PIN   (GPIO_LED_LED1_PIN)

static void busy_wait_cycles(volatile uint32_t cycles)
{
    while (cycles-- != 0U) {
        __NOP();
    }
}

static void delay_ms(uint32_t ms)
{
    while (ms-- != 0U) {
        busy_wait_cycles(3200U);
    }
}

static void uart_write_byte(uint8_t b)
{
    DL_UART_Main_transmitDataBlocking(UART_0_INST, b);
}

static void uart_write_string(const char *s)
{
    while (*s != '\0') {
        uart_write_byte((uint8_t) *s);
        s++;
    }
}

static void ui_outputs_init(void)
{
    DL_GPIO_initDigitalOutputFeatures(UI_BUZZER_IOMUX,
                                      DL_GPIO_INVERSION_DISABLE,
                                      DL_GPIO_RESISTOR_PULL_UP,
                                      DL_GPIO_DRIVE_STRENGTH_LOW,
                                      DL_GPIO_HIZ_DISABLE);
    DL_GPIO_initDigitalOutputFeatures(UI_LED_IOMUX,
                                      DL_GPIO_INVERSION_DISABLE,
                                      DL_GPIO_RESISTOR_PULL_DOWN,
                                      DL_GPIO_DRIVE_STRENGTH_LOW,
                                      DL_GPIO_HIZ_DISABLE);

    DL_GPIO_setPins(UI_BUZZER_PORT, UI_BUZZER_PIN);
    DL_GPIO_clearPins(UI_LED_PORT, UI_LED_PIN);
    DL_GPIO_enableOutput(UI_BUZZER_PORT, UI_BUZZER_PIN);
    DL_GPIO_enableOutput(UI_LED_PORT, UI_LED_PIN);
}

static void buzzer_on(void)
{
    DL_GPIO_clearPins(UI_BUZZER_PORT, UI_BUZZER_PIN);
}

static void buzzer_off(void)
{
    DL_GPIO_setPins(UI_BUZZER_PORT, UI_BUZZER_PIN);
}

static void led_on(void)
{
    DL_GPIO_setPins(UI_LED_PORT, UI_LED_PIN);
    DL_GPIO_setPins(BOARD_LED_PORT, BOARD_LED_PIN);
}

static void led_off(void)
{
    DL_GPIO_clearPins(UI_LED_PORT, UI_LED_PIN);
    DL_GPIO_clearPins(BOARD_LED_PORT, BOARD_LED_PIN);
}

static void beep(uint32_t on_ms, uint32_t off_ms)
{
    buzzer_on();
    delay_ms(on_ms);
    buzzer_off();
    delay_ms(off_ms);
}

static void blink_led(uint32_t on_ms, uint32_t off_ms)
{
    led_on();
    delay_ms(on_ms);
    led_off();
    delay_ms(off_ms);
}

int main(void)
{
    SYSCFG_DL_init();

    /* Keep the motor driver disabled while this UI-only test is running. */
    DL_GPIO_clearPins(GPIO_MOTOR_STBY_PORT, GPIO_MOTOR_STBY_STBY_PIN);

    ui_outputs_init();
    buzzer_off();
    led_off();

    uart_write_string("\r\n=== ER2024 UI buzzer/LED smoke test ===\r\n");
    uart_write_string("Buzzer: PA2, active low, VCC=3.3V\r\n");
    uart_write_string("External LED: PA14 -> 1k -> LED -> GND\r\n");
    uart_write_string("Pattern repeats: LED only, buzzer only, both.\r\n\r\n");

    while (1) {
        uart_write_string("[1] LED only\r\n");
        blink_led(1000U, 500U);

        uart_write_string("[2] Buzzer only\r\n");
        beep(250U, 250U);
        beep(250U, 750U);

        uart_write_string("[3] LED + buzzer together\r\n");
        for (uint8_t i = 0U; i < 5U; i++) {
            led_on();
            buzzer_on();
            delay_ms(120U);
            led_off();
            buzzer_off();
            delay_ms(180U);
        }

        uart_write_string("[idle] all off\r\n");
        led_off();
        buzzer_off();
        delay_ms(1000U);
    }
}
