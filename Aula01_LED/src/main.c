#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define SLEEP_TIME_MS 500

// Define o LED usando Device Tree
#define GREEN_LED_NODE DT_ALIAS(led0)
#define BLUE_LED_NODE DT_ALIAS(led1)
#define RED_LED_NODE DT_ALIAS(led2)

// Verifica se o LED está definido no Device Tree
#if DT_NODE_HAS_STATUS(GREEN_LED_NODE, okay) || DT_NODE_HAS_STATUS(BLUE_LED_NODE, okay) || DT_NODE_HAS_STATUS(RED_LED_NODE, okay)
static const struct gpio_dt_spec green_led = GPIO_DT_SPEC_GET(GREEN_LED_NODE, gpios);
static const struct gpio_dt_spec blue_led = GPIO_DT_SPEC_GET(BLUE_LED_NODE, gpios);
static const struct gpio_dt_spec red_led = GPIO_DT_SPEC_GET(RED_LED_NODE, gpios);
#else
#error "Unsupported board: led0 devicetree alias is not defined"
#endif

void main(void)
{
	int ret;

	// Verifica se o device está pronto
	if (!gpio_is_ready_dt(&led)) {
		printk("Error: LED device %s is not ready\n", led.port->name);
		return;
	}

	// Configura o pino como saída
	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error %d: failed to configure LED pin\n", ret);
		return;
	}

	printk("LED blinking on %s pin %d\n", led.port->name, led.pin);

	while (1) {
		// Toggle do LED usando a nova API
		gpio_pin_toggle_dt(&led);
		k_msleep(SLEEP_TIME_MS);
	}
}