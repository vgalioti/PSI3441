#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <stdlib.h>

#define PADEIRO_STACK_SIZE  1024
#define CLIENTE_STACK_SIZE  1024
#define PADEIRO_PRIORITY    5
#define CLIENTE_PRIORITY    5

K_THREAD_STACK_DEFINE(padeiro, PADEIRO_STACK_SIZE);
K_THREAD_STACK_DEFINE(cliente, CLIENTE_STACK_SIZE);
static struct k_thread padeiro_thread_data;
static struct k_thread cliente_thread_data;

volatile int saldo_vitrine = 0;
K_MUTEX_DEFINE(mutex_vitrine);

void padeiro_thread(void *arg1, void *arg2, void *arg3)
{
    while (1) {
        k_sleep(K_MSEC(1500));
        k_mutex_lock(&mutex_vitrine, K_FOREVER);

        saldo_vitrine++;
        printk("Padeiro: Pão pronto!\n");
        printk("Saldo da vitrine: %d\n", saldo_vitrine);
        printk("=====================================\n");

        k_mutex_unlock(&mutex_vitrine);
    }
}

void cliente_thread(void *arg1, void *arg2, void *arg3)
{
    while (1) {
        k_sleep(K_MSEC(1500));
        k_mutex_lock(&mutex_vitrine, K_FOREVER);

        saldo_vitrine--;
        printk("Cliente: Comprei pão!\n");
        printk("Saldo da vitrine: %d\n", saldo_vitrine);
        printk("=====================================\n");

        k_mutex_unlock(&mutex_vitrine);
    }
}

void main(void)
{
    printk("\n========== Atividade 7 - Parte 2 ==========\n");

    k_thread_create(&cliente_thread_data, cliente, CLIENTE_STACK_SIZE,
                    cliente_thread, NULL, NULL, NULL,
                    CLIENTE_PRIORITY, 0, K_NO_WAIT);

    k_thread_create(&padeiro_thread_data, padeiro, PADEIRO_STACK_SIZE,
                    padeiro_thread, NULL, NULL, NULL,
                    PADEIRO_PRIORITY, 0, K_NO_WAIT);

    while (1) {
        k_sleep(K_FOREVER);
    }
}