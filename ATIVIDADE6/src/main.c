#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/sensor.h>
#include <stdlib.h>

// THREADS 
#define ADC_THREAD_STACK_SIZE    1024
#define ACCEL_THREAD_STACK_SIZE  1024
#define ADC_THREAD_PRIORITY      5
#define ACCEL_THREAD_PRIORITY    4

K_THREAD_STACK_DEFINE(adc_stack, ADC_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(accel_stack, ACCEL_THREAD_STACK_SIZE);
static struct k_thread adc_thread_data;
static struct k_thread accel_thread_data;


// ADC E ACELERÔMETRO
#define ADC_RESOLUTION           12
#define ADC_GAIN                 ADC_GAIN_1
#define ADC_REFERENCE            ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME     ADC_ACQ_TIME_DEFAULT
#define ADC_CHANNEL_ID           0
#define ADC_VREF_MV              3300

static const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc0));
static int16_t sample_buffer;

static struct adc_channel_cfg adc_channel_cfg = {
    .gain = ADC_GAIN,
    .reference = ADC_REFERENCE,
    .acquisition_time = ADC_ACQUISITION_TIME,
    .channel_id = ADC_CHANNEL_ID,
    .differential = 0,
};

static struct adc_sequence adc_sequence_cfg = {
    .channels = BIT(ADC_CHANNEL_ID),
    .buffer = &sample_buffer,
    .buffer_size = sizeof(sample_buffer),
    .resolution = ADC_RESOLUTION,
};

static const struct device *accel = DEVICE_DT_GET(DT_NODELABEL(mma8451q));


// BOTÃO
#define BUTTON_NODE DT_NODELABEL(user_button_0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;


// FUNÇÃO (ISR)
volatile int complete_mode = 0; 
void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    complete_mode = !complete_mode;
    if (complete_mode) {
        printk("\n---> MODO COMPLETO: ADC + ACELEROMETRO <---\n\n");
    } else {
        printk("\n---> MODO ADC <---\n\n");
    }
}


// ADC e ACELERÔMETRO
void adc_thread(void *p1, void *p2, void *p3)
{
    while (1) {
        int err = adc_read(adc_dev, &adc_sequence_cfg);

        if (err != 0) {
            printk("[ADC] Falha na leitura: %d\n", err);
        } else {
            int32_t mv = sample_buffer;
            adc_raw_to_millivolts(ADC_VREF_MV, ADC_GAIN, ADC_RESOLUTION, &mv);
            printk("ADC: %d mV (raw: %d)\n", mv, sample_buffer);
        }

        k_sleep(K_MSEC(500));
    }
}

void accel_thread(void *p1, void *p2, void *p3)
{
    struct sensor_value accel_x, accel_y, accel_z;

    while (1) {
        if (complete_mode) {
            int ret = sensor_sample_fetch(accel);

            if (ret == 0) {
                sensor_channel_get(accel, SENSOR_CHAN_ACCEL_X, &accel_x);
                sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Y, &accel_y);
                sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Z, &accel_z);

                printk("ACCEL: X=%d.%06d | Y=%d.%06d | Z=%d.%06d\n", 
                        accel_x.val1, abs(accel_x.val2),
                        accel_y.val1, abs(accel_y.val2),
                        accel_z.val1, abs(accel_z.val2));
            }
        }
        k_sleep(K_MSEC(1000));
    }
}


// MAIN
void main(void)
{
    printk("\nIniciando Atividade 6 (PSI3441)\n");

    // Botão
    if (!gpio_is_ready_dt(&button)) {
        printk("ERRO: Botao nao esta disponivel\n");
        return;
    }
    gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_FALLING);
    gpio_init_callback(&button_cb_data, button_isr, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    // ADC e Acelerômetro
    if (!device_is_ready(adc_dev)) {
        printk("ERRO: ADC nao esta pronto\n");
        return;
    }
    adc_channel_setup(adc_dev, &adc_channel_cfg);

    if (!device_is_ready(accel)) {
        printk("ERRO: Acelerometro nao esta pronto\n");
        return;
    }

    // THREADS
    k_thread_create(&adc_thread_data, adc_stack, K_THREAD_STACK_SIZEOF(adc_stack),
                    adc_thread, NULL, NULL, NULL, ADC_THREAD_PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&accel_thread_data, accel_stack, K_THREAD_STACK_SIZEOF(accel_stack),
                    accel_thread, NULL, NULL, NULL, ACCEL_THREAD_PRIORITY, 0, K_NO_WAIT);

    while (1) {
        k_sleep(K_FOREVER);
    }
}