#ifndef CMP_H_
#define CMP_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdbool.h>
#include <stdint.h>

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef enum
{
    CMP_IN_0 = 0,
    CMP_IN_1,
    CMP_IN_2,
    CMP_IN_3,
    CMP_IN_4,
    CMP_IN_5,
    CMP_IN_6,
    CMP_IN_7
} CMP_input_t;


typedef enum
{
    CMP_MODE_CONTINUOUS = 0,
    CMP_MODE_FILTERED,
    CMP_MODE_WINDOWED
} CMP_mode_t;


typedef enum
{
    CMP_HYSTERESIS_LEVEL_0 = 0,
    CMP_HYSTERESIS_LEVEL_1,
    CMP_HYSTERESIS_LEVEL_2,
    CMP_HYSTERESIS_LEVEL_3
} CMP_hysteresis_t;


typedef enum
{
    CMP_INT_DISABLED = 0,
    CMP_INT_FALLING_EDGE,
    CMP_INT_RISING_EDGE,
    CMP_INT_BOTH_EDGES
} CMP_interrupt_t;


typedef enum
{
    CMP_DAC_VREF_1 = 0,
    CMP_DAC_VREF_2
} CMP_dac_reference_t;


typedef void (*CMP_callback_t)(void);

typedef struct
{
    uint8_t cmp_num;
    CMP_input_t pos_in;
    CMP_input_t neg_in;
    CMP_mode_t mode;
    uint8_t filter_length;
    bool sampling_clock_internal;
    uint32_t sampling_frequency;
    CMP_hysteresis_t hysteresis;
    bool inv;
    CMP_interrupt_t interrupt;
    bool use_internal_dac;
    uint8_t dac_value;
    CMP_dac_reference_t dac_reference;
} CMP_config_t;


/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

bool CMP_INIT(CMP_config_t config);

bool CMP_enable(uint8_t cmp_num);
bool CMP_disable(uint8_t cmp_num);

bool CMP_read_output(uint8_t cmp_num, bool *output);
bool CMP_set_callback(uint8_t cmp_num, CMP_callback_t callback);

#endif /* CMP_H_ */
