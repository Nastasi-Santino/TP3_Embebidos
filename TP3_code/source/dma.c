#include "dma.h"
#include "hardware.h"

#define DMA_CHANNEL_COUNT 16

static DMA_Type * const dma = DMA0;
static DMAMUX_Type * const dmamux = DMAMUX;

static const IRQn_Type dma_IRQn[][DMA_CHANNEL_COUNT] = DMA_CHN_IRQS;

static bool init;
static bool channel_used[DMA_CHANNEL_COUNT];
static DMA_callback_t callbacks[DMA_CHANNEL_COUNT];

static void DMA_IRQHandler(uint8_t channel);

bool DMA_init(void)
{
	if(init)
	{
		return false;
	}

    SIM->SCGC7 |= SIM_SCGC7_DMA_MASK;
    SIM->SCGC6 |= SIM_SCGC6_DMAMUX_MASK;

    for(uint8_t i = 0; i < DMA_CHANNEL_COUNT; i++)
    {
        dmamux->CHCFG[i] = 0;
        callbacks[i] = (DMA_callback_t)0;
    }

    dma->CERQ = DMA_CERQ_CAER_MASK;

    dma->CERR = DMA_CERR_CAEI_MASK;
    dma->CINT = DMA_CINT_CAIR_MASK;

    dma->CR &= ~DMA_CR_EMLM_MASK;

    init = true;

    return true;
}


bool DMA_configure_channel(uint8_t channel, uint8_t request_source,
                           volatile void *source, volatile void *destination,
                           uint8_t transfer_size, uint32_t minor_bytes,
						   int16_t source_offset, int16_t destination_offset,
						   uint8_t source_modulo, uint8_t destination_modulo,
						   uint16_t major_count, bool ping_pong)
{
    if(!init || channel >= DMA_CHANNEL_COUNT || channel_used[channel] || source == (void *)0 ||
       destination == (void *)0 || major_count == 0 || transfer_size == 0 || transfer_size > 4
	   || transfer_size == 3)
    {
        return false;
    }

    if(ping_pong && (major_count < 2 || (major_count % 2) != 0))
    {
        return false;
    }

    if(minor_bytes == 0 ||
       (minor_bytes % transfer_size) != 0)
    {
        return false;
    }

    if(major_count == 0 || major_count > 0x7FFF)
    {
        return false;
    }

    if(((uintptr_t)source % transfer_size) != 0 ||
       ((uintptr_t)destination % transfer_size) != 0 ||
       (source_offset % transfer_size) != 0 ||
       (destination_offset % transfer_size) != 0)
    {
        return false;
    }

    if(source_modulo > 31 ||
       destination_modulo > 31)
    {
        return false;
    }

    if(source_modulo != 0)
    {
        uint32_t modulo_size = 1UL << source_modulo;

        if(((uintptr_t)source & (modulo_size - 1U)) != 0)
        {
            return false;
        }
    }

    if(destination_modulo != 0)
    {
        uint32_t modulo_size = 1UL << destination_modulo;

        if(((uintptr_t)destination & (modulo_size - 1U)) != 0)
        {
            return false;
        }
    }

	dma->TCD[channel].SADDR = (uint32_t)source;
	dma->TCD[channel].SOFF  = (uint16_t)source_offset;

	uint8_t size_code;

	switch(transfer_size)
	{
	    case 1:
	        size_code = 0;
	        break;

	    case 2:
	        size_code = 1;
	        break;

	    case 4:
	        size_code = 2;
	        break;

	    default:
	        return false;
	}

	dma->TCD[channel].ATTR = DMA_ATTR_DSIZE(size_code) |
			DMA_ATTR_DMOD(destination_modulo) |
			DMA_ATTR_SSIZE(size_code) |
			DMA_ATTR_SMOD(source_modulo);

	dma->TCD[channel].NBYTES_MLNO = minor_bytes;

	uint32_t transfers_per_minor = minor_bytes / transfer_size;

	int64_t source_major_advance = (int64_t)source_offset *
			(int64_t)transfers_per_minor * (int64_t)major_count;

	int64_t destination_major_advance = (int64_t)destination_offset *
	        (int64_t)transfers_per_minor * (int64_t)major_count;

	if(source_major_advance > INT32_MAX ||
	   source_major_advance < INT32_MIN)
	{
	    return false;
	}

	if(destination_major_advance > INT32_MAX ||
			destination_major_advance < INT32_MIN)
	{
	    return false;
	}

	if(source_modulo == 0)
	{
	    dma->TCD[channel].SLAST = (uint32_t)(-source_major_advance);
	}
	else
	{
	    dma->TCD[channel].SLAST = 0;
	}

	dma->TCD[channel].DADDR = (uint32_t)destination;
	dma->TCD[channel].DOFF  = (uint16_t)destination_offset;

	dma->TCD[channel].CITER_ELINKNO = DMA_CITER_ELINKNO_CITER(major_count);
	dma->TCD[channel].BITER_ELINKNO = DMA_BITER_ELINKNO_BITER(major_count);

	if(destination_modulo == 0)
	{
	    dma->TCD[channel].DLAST_SGA = (uint32_t)(-destination_major_advance);
	}
	else
	{
	    dma->TCD[channel].DLAST_SGA = 0;
	}

	uint16_t csr = DMA_CSR_INTMAJOR_MASK;
	if(ping_pong)
	{
		csr |= DMA_CSR_INTHALF_MASK;
	}

	dma->TCD[channel].CSR = csr;

	dmamux->CHCFG[channel] = DMAMUX_CHCFG_SOURCE(request_source);

	channel_used[channel] = true;

	return true;
}

bool DMA_set_callback(uint8_t channel, DMA_callback_t callback)
{
	if(channel >= DMA_CHANNEL_COUNT || !channel_used[channel])
	{
		return false;
	}

	callbacks[channel] = callback;

	return true;
}

bool DMA_enable_channel(uint8_t channel)
{
	if(channel >= DMA_CHANNEL_COUNT || !channel_used[channel])
	{
		return false;
	}

	dmamux->CHCFG[channel] |= DMAMUX_CHCFG_ENBL_MASK;
	dma->SERQ = DMA_SERQ_SERQ(channel);

	NVIC_ClearPendingIRQ(dma_IRQn[0][channel]);
	NVIC_EnableIRQ(dma_IRQn[0][channel]);

	return true;
}

bool DMA_disable_channel(uint8_t channel)
{
	if(channel >= DMA_CHANNEL_COUNT || !channel_used[channel])
	{
		return false;
	}

	dmamux->CHCFG[channel] &= ~DMAMUX_CHCFG_ENBL_MASK;
	dma->CERQ = DMA_CERQ_CERQ(channel);

	NVIC_DisableIRQ(dma_IRQn[0][channel]);
	NVIC_ClearPendingIRQ(dma_IRQn[0][channel]);

	return true;
}

static void DMA_IRQHandler(uint8_t channel)
{
    dma->CINT = DMA_CINT_CINT(channel);

    if(callbacks[channel] != (DMA_callback_t)0)
    {
        callbacks[channel]();
    }
}

void DMA0_IRQHandler(void)
{
    DMA_IRQHandler(0);
}

void DMA1_IRQHandler(void)
{
    DMA_IRQHandler(1);
}

void DMA2_IRQHandler(void)
{
    DMA_IRQHandler(2);
}

void DMA3_IRQHandler(void)
{
    DMA_IRQHandler(3);
}

void DMA4_IRQHandler(void)
{
    DMA_IRQHandler(4);
}

void DMA5_IRQHandler(void)
{
    DMA_IRQHandler(5);
}

void DMA6_IRQHandler(void)
{
    DMA_IRQHandler(6);
}

void DMA7_IRQHandler(void)
{
    DMA_IRQHandler(7);
}

void DMA8_IRQHandler(void)
{
    DMA_IRQHandler(8);
}

void DMA9_IRQHandler(void)
{
    DMA_IRQHandler(9);
}

void DMA10_IRQHandler(void)
{
    DMA_IRQHandler(10);
}

void DMA11_IRQHandler(void)
{
    DMA_IRQHandler(11);
}

void DMA12_IRQHandler(void)
{
    DMA_IRQHandler(12);
}

void DMA13_IRQHandler(void)
{
    DMA_IRQHandler(13);
}

void DMA14_IRQHandler(void)
{
    DMA_IRQHandler(14);
}

void DMA15_IRQHandler(void)
{
    DMA_IRQHandler(15);
}

