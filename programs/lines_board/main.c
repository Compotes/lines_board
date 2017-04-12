#include "ch.h"
#include "hal.h"

#include <icu_lld.h>
#include "chprintf.h"
#include <inttypes.h>

#include "communication.h"
#include "sensor.h"

#define MAX_PULSE_LENGTH 3000
#define MIN_PULSE_LENGTH 400
#define NOT_SEEING_VALUE 3000
#define QUANTITY_OF_SENSORS 2

#define UNUSED(arg) (void)(arg)

int32_t sensors_values[QUANTITY_OF_SENSORS];

thread_t *commanderp;

void execute_master_command(uint16_t command_id, uint8_t *buff) {
    switch (command_id) {
        case 0xAA:  
//            led(GREEN, buff[0]);
            break;

        case 0xBA:
            reset();
            break;

        default:    
            break;
    }

}

uint8_t *send_data_command(uint16_t command_id, uint8_t data_length, uint8_t *buff) {
    uint32_t values = 0;
    uint8_t i;
    switch (command_id) {
        case 0xFF:
            palSetPad(GPIOA, 4);
            values = get_hit_line();

            for(i = 0; i < data_length; i++) {
                buff[i] = (values >> (4 * i)) & 0xf;
            }
            break;

        default:    
            break;           
    }
    return buff;
}

THD_WORKING_AREA(waCommanderThread, 128);
THD_FUNCTION(CommanderThread, arg) {
    UNUSED(arg);

    thread_t *serialp;
    message_data_t *messagep;
    uint16_t cmd, command_id;
    uint8_t data_buff[MAX_DATA_CHAR_LENGTH];
    uint8_t data_length, i, rw_bit;

//    led(GREEN, 0);

    palClearPad(GPIOA, 4);

    while (1) {
        serialp = chMsgWait();
        messagep = (message_data_t *)chMsgGet(serialp);
        
        cmd = messagep->operation;

        command_id = cmd & COMMAND_ID_MASK;
        rw_bit = ((cmd >> OPERATION_BIT_LENGTH) & 1) + 1;
        data_length = cmd >> (OPERATION_BIT_LENGTH + RW_BIT);
        
        if (rw_bit == PROCESS_DATA_FROM_MASTER) {
            for (i = 0; i < data_length; i++) {
                data_buff[i] = messagep->data[i];
            }
            execute_master_command(command_id, data_buff);
        } else {
            send_data_command(command_id, data_length, data_buff);
            for (i = 0; i < data_length; i++) {
                messagep->data[i] = data_buff[i];
            }
        }

        chMsgRelease(serialp, (msg_t)messagep);
    }
    chRegSetThreadName("commander");
    chThdSleepMilliseconds(100);
}

int main(void) {    
    halInit();
    chSysInit();
    
    communication_init();
    
    sensor_init();

    /*
     * Threads setup
     */
    commanderp = chThdCreateStatic(waCommanderThread, sizeof(waCommanderThread), 
                                   NORMALPRIO, CommanderThread, NULL);

    communication_thread(commanderp);

    while (1) {
        chThdSleepMilliseconds(100);
    }
}
