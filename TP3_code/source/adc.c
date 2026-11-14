#include "adc.h"
#include "dma.h"
#include "hardware.h"

#define ADC0_DMA_REQUEST_NUM 	40
#define ADC1_DMA_REQUEST_NUM	41

static ADC_Type * const adcs[] = ADC_BASE_PTRS;

static bool init[2];
static uint8_t status_sel[2];
static bool ping_pong_buffers[2];
static bool channel_configured[2];

static ADC_callback_t callbacks[2];
static volatile uint16_t * buffers[2];
static uint16_t buffers_size[2];

static bool first_buffer_active[2] = {true, true};

static uint8_t dma_channels[2];
static bool dma_configured[2];

static void ADC0_Handler(void);
static void ADC1_Handler(void);
static void ADC_Handler(uint8_t adc_num);

static bool ADC_configure_pin(uint8_t adc_num, ADC_input_t input);
static bool ADC_calibrate(ADC_Type *adc);

bool ADC_init(ADC_config_t config)
{
	if(config.adc_num > 1 || init[config.adc_num] || config.resolution > ADC_RESOLUTION_16_BIT
			|| config.reference > ADC_REFERENCE_ALTERNATE)
	{
		return false;
	}

	if(config.adc_num == 0)
	{
	    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;
	}
	else
	{
	    SIM->SCGC3 |= SIM_SCGC3_ADC1_MASK;
	}

	ADC_Type * adc = adcs[config.adc_num];

	adc->CFG1 = ADC_CFG1_MODE(config.resolution) | ADC_CFG1_ADICLK(1) | ADC_CFG1_ADIV(3);

	adc->SC2 = ADC_SC2_REFSEL(config.reference);

	if(!ADC_calibrate(adc))
	{
	    return false;
	}

	if(config.hardware_trigger)
	{
		adc->SC2 |= ADC_SC2_ADTRG_MASK;
	}

	init[config.adc_num] = true;

	return true;
}

static bool ADC_calibrate(ADC_Type *adc)
{
    uint32_t sum;

    adc->SC2 &= ~ADC_SC2_ADTRG_MASK;
    adc->SC3 |= ADC_SC3_CALF_MASK;

    adc->SC3 = ADC_SC3_AVGE_MASK | ADC_SC3_AVGS(3);

    adc->SC3 |= ADC_SC3_CAL_MASK;

    while(adc->SC3 & ADC_SC3_CAL_MASK)
    {
    }

    if(adc->SC3 & ADC_SC3_CALF_MASK)
    {
        return false;
    }

    sum =
        adc->CLP0 +
        adc->CLP1 +
        adc->CLP2 +
        adc->CLP3 +
        adc->CLP4 +
        adc->CLPS;

    sum /= 2U;
    sum |= 0x8000U;

    adc->PG = ADC_PG_PG(sum);

    sum =
        adc->CLM0 +
        adc->CLM1 +
        adc->CLM2 +
        adc->CLM3 +
        adc->CLM4 +
        adc->CLMS;

    sum /= 2U;
    sum |= 0x8000U;

    adc->MG = ADC_MG_MG(sum);

    adc->SC3 &= ~(ADC_SC3_AVGE_MASK |
                  ADC_SC3_AVGS_MASK);

    volatile uint32_t dummy = adc->R[0];
    (void)dummy;

    return true;
}

bool ADC_configure_channel(uint8_t adc_num, uint8_t pretrigger, ADC_input_t input)
{
	if(adc_num > 1 || !init[adc_num] || pretrigger > 1 || input > ADC_IN_23)
	{
		return false;
	}

	ADC_Type * adc = adcs[adc_num];

	if(!ADC_configure_pin(adc_num, input))
	{
	    return false;
	}

	adc->SC1[pretrigger] = ADC_SC1_ADCH(input);

	status_sel[adc_num] = pretrigger;
	channel_configured[adc_num] = true;

	return true;
}

bool ADC_configure_dma(uint8_t adc_num, uint8_t dma_channel, volatile uint16_t *buffer,
                       uint16_t buffer_size, bool ping_pong)
{
	if(adc_num > 1 || !init[adc_num] || !channel_configured[adc_num] ||
	   buffer == (uint16_t *)0 || buffer_size == 0)
	{
	    return false;
	}
	if(ping_pong && (buffer_size < 2 || (buffer_size % 2) != 0))
	{
	    return false;
	}

	ADC_Type * adc = adcs[adc_num];

	DMA_init();

	if(!DMA_configure_channel(
	        dma_channel,
	        adc_num == 0 ? ADC0_DMA_REQUEST_NUM : ADC1_DMA_REQUEST_NUM,
	        (void *)&adc->R[status_sel[adc_num]],
	        buffer,
	        sizeof(uint16_t),
			sizeof(uint16_t),
	        0,
	        sizeof(uint16_t),
			0,
			0,
	        buffer_size,
	        ping_pong))
	{
	    return false;
	}

	if(adc_num == 0)
	{
	    if(!DMA_set_callback(dma_channel, ADC0_Handler))
	    {
	        return false;
	    }
	}
	else
	{
	    if(!DMA_set_callback(dma_channel, ADC1_Handler))
	    {
	        return false;
	    }
	}

	ping_pong_buffers[adc_num] = ping_pong;

	buffers[adc_num] = buffer;
	buffers_size[adc_num] = buffer_size;

	dma_channels[adc_num] = dma_channel;
	dma_configured[adc_num] = true;
	first_buffer_active[adc_num] = true;

	return true;
}

bool ADC_set_callback(uint8_t adc_num, ADC_callback_t callback)
{
	if(callback == (ADC_callback_t)0 || adc_num > 1 || !init[adc_num])
	{
		return false;
	}

	callbacks[adc_num] = callback;

	return true;
}


bool ADC_enable(uint8_t adc_num)
{
    if(adc_num > 1 || !init[adc_num] || !channel_configured[adc_num])
    {
        return false;
    }

    if(dma_configured[adc_num])
    {
        if(!DMA_enable_channel(dma_channels[adc_num]))
        {
            return false;
        }

        adcs[adc_num]->SC2 |= ADC_SC2_DMAEN_MASK;
    }

    return true;
}

bool ADC_disable(uint8_t adc_num)
{
    if(adc_num > 1 || !init[adc_num])
    {
        return false;
    }

    adcs[adc_num]->SC2 &= ~ADC_SC2_DMAEN_MASK;

    if(dma_configured[adc_num])
    {
        if(!DMA_disable_channel(dma_channels[adc_num]))
        {
            return false;
        }
    }

    return true;
}

static void ADC_Handler(uint8_t adc_num)
{
    volatile uint16_t *ready_buffer;
    uint16_t ready_size;

    if(ping_pong_buffers[adc_num])
    {
        ready_size = buffers_size[adc_num] / 2;

        if(first_buffer_active[adc_num])
        {
            ready_buffer = buffers[adc_num];
        }
        else
        {
            ready_buffer =
                buffers[adc_num] + buffers_size[adc_num] / 2;
        }

        first_buffer_active[adc_num] =
                !first_buffer_active[adc_num];
    }
    else
    {
        ready_buffer = buffers[adc_num];
        ready_size = buffers_size[adc_num];
    }

    if(callbacks[adc_num] != (ADC_callback_t)0)
    {
        callbacks[adc_num](ready_buffer, ready_size);
    }
}


static void ADC0_Handler(void)
{
	ADC_Handler(0);
}

static void ADC1_Handler(void)
{
	ADC_Handler(1);
}

static bool ADC_configure_pin(uint8_t adc_num, ADC_input_t input)
{
    PORT_Type *port = (PORT_Type *)0;
    uint8_t pin = 0;
    bool has_port = true;

    switch(adc_num)
    {
        case 0:
            switch(input)
            {
                case ADC_IN_4:
                    port = PORTC;
                    pin = 2;
                    adcs[0]->CFG2 |= ADC_CFG2_MUXSEL_MASK;
                    break;

                case ADC_IN_8:
                    port = PORTB;
                    pin = 0;
                    break;

                case ADC_IN_9:
                    port = PORTB;
                    pin = 1;
                    break;

                case ADC_IN_10:
                    port = PORTA;
                    pin = 7;
                    break;

                case ADC_IN_11:
                    port = PORTA;
                    pin = 8;
                    break;

                case ADC_IN_12:
                    port = PORTB;
                    pin = 2;
                    break;

                case ADC_IN_13:
                    port = PORTB;
                    pin = 3;
                    break;

                case ADC_IN_14:
                    port = PORTC;
                    pin = 0;
                    break;

                case ADC_IN_15:
                    port = PORTC;
                    pin = 1;
                    break;

                case ADC_IN_17:
                    port = PORTE;
                    pin = 24;
                    break;

                case ADC_IN_18:
                    port = PORTE;
                    pin = 25;
                    break;

                case ADC_IN_0:
                case ADC_IN_1:
                case ADC_IN_2:
                case ADC_IN_3:
                case ADC_IN_16:
                case ADC_IN_21:
                case ADC_IN_22:
                case ADC_IN_23:
                    has_port = false;
                    break;

                default:
                    return false;
            }
            break;


        case 1:
            switch(input)
            {
                case ADC_IN_4:
                    port = PORTC;
                    pin = 8;
                    adcs[1]->CFG2 |= ADC_CFG2_MUXSEL_MASK;
                    break;

                case ADC_IN_5:
                	port = PORTC;
                    pin = 9;
                    adcs[1]->CFG2 |= ADC_CFG2_MUXSEL_MASK;
                    break;

                case ADC_IN_6:
                    port = PORTC;
                    pin = 10;
                    adcs[1]->CFG2 |= ADC_CFG2_MUXSEL_MASK;
                    break;

                case ADC_IN_7:
                    port = PORTC;
                    pin = 11;
                    adcs[1]->CFG2 |= ADC_CFG2_MUXSEL_MASK;
                    break;

                case ADC_IN_8:
                    port = PORTB;
                    pin = 0;
                    break;

                case ADC_IN_9:
                    port = PORTB;
                    pin = 1;
                    break;

                case ADC_IN_10:
                    port = PORTB;
                    pin = 4;
                    break;

                case ADC_IN_11:
                    port = PORTB;
                    pin = 5;
                    break;

                case ADC_IN_12:
                    port = PORTB;
                    pin = 6;
                    break;

                case ADC_IN_13:
                    port = PORTB;
                    pin = 7;
                    break;

                case ADC_IN_14:
                    port = PORTB;
                    pin = 10;
                    break;

                case ADC_IN_15:
                    port = PORTB;
                    pin = 11;
                    break;

                case ADC_IN_0:
                case ADC_IN_1:
                case ADC_IN_2:
                case ADC_IN_3:
                case ADC_IN_16:
                case ADC_IN_18:
                case ADC_IN_23:
                    has_port = false;
                    break;

                default:
                    return false;
            }
            break;

        default:
            return false;
    }

    if(has_port)
    {
        port->PCR[pin] &= ~(PORT_PCR_MUX_MASK | PORT_PCR_PE_MASK);
    }

    return true;
}
