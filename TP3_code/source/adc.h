#ifndef ADC_H_
#define ADC_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    ADC_RESOLUTION_8_BIT = 0,
    ADC_RESOLUTION_12_BIT,
    ADC_RESOLUTION_10_BIT,
    ADC_RESOLUTION_16_BIT
} ADC_resolution_t;

typedef enum
{
    ADC_REFERENCE_DEFAULT = 0,
    ADC_REFERENCE_ALTERNATE
} ADC_reference_t;

typedef enum
{
    ADC_IN_0 = 0,
    ADC_IN_1,
    ADC_IN_2,
    ADC_IN_3,
    ADC_IN_4,
    ADC_IN_5,
    ADC_IN_6,
    ADC_IN_7,
    ADC_IN_8,
    ADC_IN_9,
    ADC_IN_10,
    ADC_IN_11,
    ADC_IN_12,
    ADC_IN_13,
    ADC_IN_14,
    ADC_IN_15,
    ADC_IN_16,
    ADC_IN_17,
    ADC_IN_18,
    ADC_IN_19,
    ADC_IN_20,
    ADC_IN_21,
    ADC_IN_22,
    ADC_IN_23
} ADC_input_t;

typedef struct
{
    uint8_t adc_num;
    ADC_resolution_t resolution;
    ADC_reference_t reference;
    bool hardware_trigger;
} ADC_config_t;

typedef void (*ADC_callback_t)(volatile uint16_t *buffer, uint16_t length);

bool ADC_init(ADC_config_t config);

bool ADC_configure_channel(uint8_t adc_num, uint8_t pretrigger, ADC_input_t input);

bool ADC_configure_dma(uint8_t adc_num, uint8_t dma_channel, volatile uint16_t *buffer,
                       uint16_t buffer_size, bool ping_pong);

bool ADC_set_callback(uint8_t adc_num, ADC_callback_t callback);

bool ADC_enable(uint8_t adc_num);
bool ADC_disable(uint8_t adc_num);

#endif /* ADC_H_ */
