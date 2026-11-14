#ifndef DMA_H_
#define DMA_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

#include <stdbool.h>
#include <stdint.h>

/*******************************************************************************
 * ENUMERATIONS AND STRUCTURES AND TYPEDEFS
 ******************************************************************************/

typedef void (*DMA_callback_t)(void);

/*******************************************************************************
 * FUNCTION PROTOTYPES WITH GLOBAL SCOPE
 ******************************************************************************/

bool DMA_init(void);

bool DMA_configure_channel(uint8_t channel, uint8_t request_source,
                           volatile void *source, volatile void *destination,
                           uint8_t transfer_size, uint32_t minor_bytes,
						   int16_t source_offset, int16_t destination_offset,
						   uint8_t source_modulo, uint8_t destination_modulo,
						   uint16_t major_count, bool ping_pong);

bool DMA_set_callback(uint8_t channel, DMA_callback_t callback);

bool DMA_enable_channel(uint8_t channel);
bool DMA_disable_channel(uint8_t channel);

#endif /* DMA_H_ */
