#ifndef PDB_H_
#define PDB_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    PDB_TRIGGER_0 = 0,
    PDB_TRIGGER_1,
    PDB_TRIGGER_2,
    PDB_TRIGGER_3,
    PDB_TRIGGER_4,
    PDB_TRIGGER_5,
    PDB_TRIGGER_6,
    PDB_TRIGGER_7,
    PDB_TRIGGER_8,
    PDB_TRIGGER_9,
    PDB_TRIGGER_10,
    PDB_TRIGGER_11,
    PDB_TRIGGER_12,
    PDB_TRIGGER_13,
    PDB_TRIGGER_14,
    PDB_TRIGGER_SOFTWARE
} PDB_trigger_t;

typedef struct
{
    uint32_t frequency;
    PDB_trigger_t trigger_source;
    bool continuous;
} PDB_config_t;

bool PDB_init(PDB_config_t config);

bool PDB_configure_adc_trigger(uint8_t adc_num, uint8_t pretrigger, uint16_t delay);

bool PDB_configure_dac_trigger(uint8_t dac_num, uint16_t interval);

bool PDB_enable(void);
bool PDB_disable(void);

bool PDB_software_trigger(void);

#endif /* PDB_H_ */
