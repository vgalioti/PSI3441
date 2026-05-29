#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "MKL25Z4.h"

void leds_init(void) {
    // PORTD -> BLUE ; PORTB -> GREEN
    SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTB_MASK;

    // Set blue LED
    PORTD->PCR[1] = PORT_PCR_MUX(1); // Função GPIO
    GPIOD->PDDR |= (1 << 1);         // Direção: Saída
    GPIOD->PSOR |= (1 << 1);         // Começa apagado (Nível Alto)

    // Set green LED
    PORTB->PCR[19] = PORT_PCR_MUX(1); // Função GPIO
    GPIOB->PDDR |= (1 << 19);         // Direção: Saída
    GPIOB->PSOR |= (1 << 19);         // Começa apagado (Nível Alto)
}


void ADC0_Init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;
    
    PORTB->PCR[0] = PORT_PCR_MUX(0);
    
    ADC0->CFG1 = ADC_CFG1_MODE(1) | ADC_CFG1_ADICLK(0);
    ADC0->SC2 = 0x00;
    ADC0->SC3 = 0x00;
}

uint16_t ADC0_Read(uint8_t channel) {
    ADC0->SC1[0] = channel;
    while (!(ADC0->SC1[0] & ADC_SC1_COCO_MASK)) {}
    return ADC0->R[0];
}

int main(void) {
    uint16_t sensor_value = 0;
    uint32_t voltage_mv = 0; 

    leds_init();
    ADC0_Init();

    while (1) {
        // Lê e converte o sensor
        sensor_value = ADC0_Read(8);
        voltage_mv = (sensor_value * 3300) / 4095;

        printk("Tensão: %u mV\n", voltage_mv);

        if (voltage_mv > 3000) {
            GPIOD->PCOR |= (1 << 1);  // Azul ON
            GPIOB->PSOR |= (1 << 19); // Verde OFF

        } else if (voltage_mv < 500) {
            GPIOD->PSOR |= (1 << 1);  // Azul OFF
            GPIOB->PCOR |= (1 << 19); // Verde ON

        } else {
            GPIOD->PSOR |= (1 << 1);  // Azul OFF
            GPIOB->PSOR |= (1 << 19); // Verde OFF
        }

        k_msleep(100);
    }

    return 0;
}