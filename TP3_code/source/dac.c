#include "dac.h"
#include "hardware.h"
#include "dma.h"
#include "pdb.h"

#define DAC0_DMA_REQUEST_NUM 	45
#define DAC1_DMA_REQUEST_NUM	46

static DAC_Type * const dacs[] = DAC_BASE_PTRS;

static bool init[2];

static DAC_callback_t callbacks[2];
static volatile uint16_t * buffers[2];
static uint16_t buffers_size[2];

static bool first_buffer_active[2] = {true, true};

static bool ping_pong_buffers[2];
static uint8_t dma_channels[2];
static bool dma_configured[2];

static void DAC0_Handler(void);
static void DAC1_Handler(void);
static void DAC_Handler(uint8_t dac_num);

bool DAC_init(DAC_config_t config)
{
	if(config.dac_num > 1 || init[config.dac_num] || config.reference > DAC_REFERENCE_2)
	{
		return false;
	}

	DAC_Type * dac = dacs[config.dac_num];

	dac->C0 = DAC_C0_DACRFS(config.reference);

	if(!config.hardware_trigger)
	{
		dac->C0 |= DAC_C0_DACTRGSEL_MASK;
	}

	init[config.dac_num] = true;

	return true;
}

bool DAC_write(uint8_t dac_num, uint16_t value)
{
    if(dac_num > 1 || !init[dac_num] || value > 4095)
    {
        return false;
    }

	dacs[dac_num]->DAT[0].DATH = (uint8_t)(value >> 8);
	dacs[dac_num]->DAT[0].DATL = (uint8_t)value;

	return true;
}

bool DAC_configure_dma(uint8_t dac_num, uint8_t dma_channel,
                       volatile uint16_t *buffer, uint16_t buffer_size, bool ping_pong)
{
	if(dac_num > 1 || !init[dac_num] || buffer == (uint16_t *)0 || buffer_size == 0)
	{
		return false;
	}

	if(buffer_size < 16 || (buffer_size % 16) != 0)
	{
	    return false;
	}

	uint16_t major_count = buffer_size / 16;

	if(ping_pong && (major_count % 2) != 0)
	{
	    return false;
	}

	DAC_Type * dac = dacs[dac_num];

	dac->C1 &= ~DAC_C1_DACBFMD_MASK;
	dac->C1 |= DAC_C1_DACBFEN_MASK;

	dac->C2 =
	        DAC_C2_DACBFRP(0) |
	        DAC_C2_DACBFUP(15);

	dac->C0 &= ~(DAC_C0_DACBWIEN_MASK |
	             DAC_C0_DACBBIEN_MASK);

	dac->C0 |= DAC_C0_DACBTIEN_MASK;

	DMA_init();

	if(!DMA_configure_channel(
			dma_channel,
			dac_num == 0 ? DAC0_DMA_REQUEST_NUM : DAC1_DMA_REQUEST_NUM,
			buffer,
			(void *)&dac->DAT[0],
			sizeof(uint16_t),
			16*sizeof(uint16_t),
			sizeof(uint16_t),
			sizeof(uint16_t),
			0,
			5,
			major_count,
			ping_pong))
	{
		return false;
	}

	if(dac_num == 0)
	{
	    if(!DMA_set_callback(dma_channel, DAC0_Handler))
	    {
	        return false;
	    }
	}
	else
	{
	    if(!DMA_set_callback(dma_channel, DAC1_Handler))
	    {
	        return false;
	    }
	}

	ping_pong_buffers[dac_num] = ping_pong;

	buffers[dac_num] = buffer;
	buffers_size[dac_num] = buffer_size;

	dma_channels[dac_num] = dma_channel;
	dma_configured[dac_num] = true;
	first_buffer_active[dac_num] = true;

	return true;
}

bool DAC_set_callback(uint8_t dac_num, DAC_callback_t callback)
{
	if(callback == (DAC_callback_t)0 || dac_num > 1 || !init[dac_num])
	{
		return false;
	}

	callbacks[dac_num] = callback;

	return true;
}

bool DAC_enable(uint8_t dac_num)
{
    if(dac_num > 1 || !init[dac_num])
    {
        return false;
    }

    dacs[dac_num]->C0 |= DAC_C0_DACEN_MASK;

    if(dma_configured[dac_num])
    {
        if(!DMA_enable_channel(dma_channels[dac_num]))
            return false;

        dacs[dac_num]->C1 |= DAC_C1_DMAEN_MASK;
    }

    return true;
}


bool DAC_disable(uint8_t dac_num)
{
    if(dac_num > 1 || !init[dac_num])
    {
        return false;
    }

    dacs[dac_num]->C0 &= ~DAC_C0_DACEN_MASK;
    dacs[dac_num]->C1 &= ~DAC_C1_DMAEN_MASK;

    if(dma_configured[dac_num])
    {
        if(!DMA_disable_channel(dma_channels[dac_num]))
        {
            return false;
        }
    }

    return true;
}

static void DAC_Handler(uint8_t dac_num)
{
    volatile uint16_t *ready_buffer;
    uint16_t ready_size;

    if(ping_pong_buffers[dac_num])
    {
        ready_size = buffers_size[dac_num] / 2;

        if(first_buffer_active[dac_num])
        {
            ready_buffer = buffers[dac_num];
        }
        else
        {
            ready_buffer =
                buffers[dac_num] + buffers_size[dac_num] / 2;
        }

        first_buffer_active[dac_num] =
                !first_buffer_active[dac_num];
    }
    else
    {
        ready_buffer = buffers[dac_num];
        ready_size = buffers_size[dac_num];
    }

    if(callbacks[dac_num] != (DAC_callback_t)0)
    {
        callbacks[dac_num](ready_buffer, ready_size);
    }
}

static void DAC0_Handler(void)
{
	DAC_Handler(0);
}

static void DAC1_Handler(void)
{
	DAC_Handler(1);
}
