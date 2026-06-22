#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <stdlib.h>

// THREADS 
#define ACQ_THREAD_STACK_SIZE    1024
#define COM_THREAD_STACK_SIZE    1024
#define ACQ_THREAD_PRIORITY      3
#define COM_THREAD_PRIORITY      5

K_THREAD_STACK_DEFINE(acq_stack, ACQ_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(com_stack, COM_THREAD_STACK_SIZE);
static struct k_thread acq_thread_data;
static struct k_thread com_thread_data;


// ACELERÔMETRO
static const struct device *accel = DEVICE_DT_GET(DT_NODELABEL(mma8451q));


// MENSAGENS
struct sensor_data {
    int64_t timestamp;
    int32_t z_raw;
    int32_t z_filt;
};
K_MSGQ_DEFINE(data_queue, sizeof(struct sensor_data), 100, 4);


// FILTRO FIR
#define FIR_TAPS 4
static int32_t z_history[FIR_TAPS] = {0};
static int history_idx = 0;

int32_t apply_fir(int32_t new_val) {
    z_history[history_idx] = new_val;
    history_idx = (history_idx + 1) % FIR_TAPS;
    
    int32_t sum = 0;
    for (int i = 0; i < FIR_TAPS; i++) {
        sum += z_history[i];
    }
    return sum / FIR_TAPS;
}


// BOTÃO
#define BUTTON_NODE DT_NODELABEL(user_button_0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;
volatile int filter_enabled = 0; 

void button_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    filter_enabled = !filter_enabled;
    if (filter_enabled) {
        printk("--- FILTRO FIR LIGADO ---\n");
    } else {
        printk("--- FILTRO FIR DESLIGADO ---\n");
    }
}


// THREAD DE AQUISIÇÃO
void acq_thread(void *p1, void *p2, void *p3)
{
    struct sensor_value accel_z;
    struct sensor_data data;

    while (1) {
        int ret = sensor_sample_fetch(accel);

        if (ret == 0) {
            sensor_channel_get(accel, SENSOR_CHAN_ACCEL_Z, &accel_z);

            int32_t raw_z = (accel_z.val1 * 1000000) + accel_z.val2;

            data.timestamp = k_uptime_get();
            data.z_raw = raw_z;

            if (filter_enabled) {
                data.z_filt = apply_fir(raw_z);
            } else {
                data.z_filt = raw_z;
            }

            // Envia para a fila
            if (k_msgq_put(&data_queue, &data, K_NO_WAIT) != 0) {
                printk("Fila encheu!\n");
            }
        }
        
        k_sleep(K_MSEC(1)); // 1ms = 1000Hz
    }
}


// THREAD DE COMUNICAÇÃO
void com_thread(void *p1, void *p2, void *p3)
{
    struct sensor_data data;

    printk("timestamp,z_raw,z_filt\n");

    while (1) {
        k_msgq_get(&data_queue, &data, K_FOREVER);
        printk("%lld,%d,%d\n", data.timestamp, data.z_raw, data.z_filt);
    }
}


// MAIN
void main(void)
{
    printk("\n========== Atividade 8 ==========\n");

    // Botão
    if (!gpio_is_ready_dt(&button)) {
        printk("ERRO: Botao nao esta disponivel\n");
        return;
    }
    gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_FALLING);
    gpio_init_callback(&button_cb_data, button_isr, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    // Acelerômetro
    if (!device_is_ready(accel)) {
        printk("ERRO: Acelerometro nao esta pronto\n");
        return;
    }

    // Threads
    k_thread_create(&acq_thread_data, acq_stack, K_THREAD_STACK_SIZEOF(acq_stack),
                    acq_thread, NULL, NULL, NULL, ACQ_THREAD_PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&com_thread_data, com_stack, K_THREAD_STACK_SIZEOF(com_stack),
                    com_thread, NULL, NULL, NULL, COM_THREAD_PRIORITY, 0, K_NO_WAIT);

    while (1) {
        k_sleep(K_FOREVER);
    }
}