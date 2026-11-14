#ifndef DAC_H_
#define DAC_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    DAC_REFERENCE_1 = 0,
    DAC_REFERENCE_2
} DAC_reference_t;

typedef struct
{
    uint8_t dac_num;
    DAC_reference_t reference;
    bool hardware_trigger;
} DAC_config_t;

typedef void (*DAC_callback_t)(volatile uint16_t *buffer, uint16_t length);

bool DAC_init(DAC_config_t config);

bool DAC_write(uint8_t dac_num, uint16_t value);

bool DAC_configure_dma(uint8_t dac_num, uint8_t dma_channel,
                       volatile uint16_t *buffer, uint16_t buffer_size, bool ping_pong);

bool DAC_set_callback(uint8_t dac_num, DAC_callback_t callback);

bool DAC_enable(uint8_t dac_num);
bool DAC_disable(uint8_t dac_num);

#endif /* DAC_H_ */
