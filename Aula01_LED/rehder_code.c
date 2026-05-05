#include <zephyr.h>				// Funções do Zephyr
#include <device.h>				// API para obter e usar dispostivos
#include <drivers/gpio.h>		// API para controle de GPIO

#define LED_PORT		"GPIO_1"		// Nome do controlador GPIO
#define LED_PIN			18				// Pino PTB18 onde está o LED vermelho
#define SLEEP_TIME_MS	500				// Intervalo de piscar (ms)

void main(void)
{
	const struct device *port = device_get_binding(LED_PORT);
	// Obtém ponteiro para o controlador GPIO "GPIO_1"
	// (não é feito a cada iteração para economizar chamadas)

	gpio_pin_configure(port, LED_PIN, GPIO_OUTPUT_ACTIVE);
	// Configura o pino como saída ativa (LED apagado ou acesso depende de pull)

	while (1) {
		gpio_pin_toggle(port, LED_PIN);
		// Inverte o nível no pino (se estava alto, passa para baixo e vice-versa)

		k_msleep(SLEEP_TIME_MS);
		// Suspende a tarefa por SLEEP_TIME_MS
	}

}