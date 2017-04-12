#include "ch.h"
#include "hal.h"

#include "sensor.h"

#define NUMBER_OF_SENSORS 4

#define LINE_VALUE 2600

uint8_t line_hit[NUMBER_OF_SENSORS];

static const ADCConversionGroup s1 = {
    FALSE,
    4,
    NULL,
    NULL,
    ADC_CFGR1_RES_12BIT,
    ADC_TR(0, 0),
    ADC_SMPR_SMP_7P5,
    ADC_CHSELR_CHSEL5 | ADC_CHSELR_CHSEL6 | ADC_CHSELR_CHSEL7 | ADC_CHSELR_CHSEL9
};

void reset(void) {
    uint8_t i;

    for(i = 0; i < NUMBER_OF_SENSORS; i++) {
        line_hit[i] = 0;
    }
}

uint8_t get_hit_line(void) {
    uint8_t i, message = 0;
    
    for(i = 0; i < NUMBER_OF_SENSORS; i++) {
        message |= line_hit[i] << i;
    }

    return message;
}

THD_WORKING_AREA(waSensorThread, 64);
THD_FUNCTION(SensorThread, arg) {
    (void)arg;

    chRegSetThreadName("sensor");

    uint8_t i, led = 0;
    adcsample_t sensors_values[NUMBER_OF_SENSORS];

    while(1) {
        adcConvert(&ADCD1, &s1, sensors_values, 1);
        for(i = 0; i < NUMBER_OF_SENSORS; i++) {
            
            if(sensors_values[i] < LINE_VALUE) {
                line_hit[i] = 1;
                led += 1;
            } else {
                line_hit[i] = 0;
            }
        }

        if(led >= 1) {
//            palSetPad(GPIOA, 4);
            led = 0;
        } else {
  //          palClearPad(GPIOA, 4);
        }
    }

    chThdSleepMilliseconds(100);
}

void sensor_init(void) {
    palSetPadMode(GPIOA, 4, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPadMode(GPIOA, 5, PAL_MODE_INPUT_ANALOG);
    palSetPadMode(GPIOA, 6, PAL_MODE_INPUT_ANALOG);
    palSetPadMode(GPIOA, 7, PAL_MODE_INPUT_ANALOG);
    palSetPadMode(GPIOB, 1, PAL_MODE_INPUT_ANALOG);
    
    adcSTM32SetCCR(ADC_CCR_VBATEN | ADC_CCR_TSEN | ADC_CCR_VREFEN);
    adcStart(&ADCD1, NULL);

    chThdCreateStatic(waSensorThread, sizeof(waSensorThread), NORMALPRIO, SensorThread, NULL);
}
