#include "cmp.h"
#include "hardware.h"

#define BUS_CLOCK_HZ 60000000U

static CMP_Type * const cmps[] = CMP_BASE_PTRS;
static IRQn_Type cmps_IRQn[] = CMP_IRQS;

static bool init[3];

static CMP_interrupt_t interrupt_config[3];

static CMP_callback_t callbacks[3];

static void CMP_IRQHandler(uint8_t cmp_num);
static bool CMP_configure_input_pin(uint8_t cmp_num, CMP_input_t input);

bool CMP_INIT(CMP_config_t config)
{
    if(config.cmp_num > 2 || config.pos_in > CMP_IN_7 || config.neg_in > CMP_IN_7 ||
       config.pos_in == config.neg_in || config.mode > CMP_MODE_WINDOWED ||
       config.hysteresis > CMP_HYSTERESIS_LEVEL_3 || config.interrupt > CMP_INT_BOTH_EDGES ||
       init[config.cmp_num])
    {
        return false;
    }

    if(config.mode == CMP_MODE_FILTERED)
    {
        if(config.filter_length < 1 || config.filter_length > 7)
        {
            return false;
        }

        if(config.sampling_clock_internal)
        {
            if(config.sampling_frequency == 0)
            {
                return false;
            }

            uint32_t filt_per =
                BUS_CLOCK_HZ / config.sampling_frequency;

            if(filt_per == 0 || filt_per > 255)
            {
                return false;
            }
        }
    }

    if(config.use_internal_dac)
    {
        if(config.dac_value < 1 || config.dac_value > 64 || config.dac_reference > CMP_DAC_VREF_2)
        {
            return false;
        }

        if(config.cmp_num == 2)
        {
            return false;
        }

        if(config.pos_in != CMP_IN_7 && config.neg_in != CMP_IN_7)
        {
            return false;
        }
    }

	CMP_Type * cmp = cmps[config.cmp_num];

	SIM->SCGC4 |= SIM_SCGC4_CMP_MASK;

	cmp->CR0 |= config.hysteresis << CMP_CR0_HYSTCTR_SHIFT;

	if(config.inv)
	{
		cmp->CR1 |= CMP_CR1_INV_MASK;
	}

	if(config.mode == CMP_MODE_FILTERED)
	{

		cmp->CR0 |= config.filter_length << CMP_CR0_FILTER_CNT_SHIFT;

		if(config.sampling_clock_internal)
		{
			uint32_t filt_per =
			    BUS_CLOCK_HZ / config.sampling_frequency;

			cmp->FPR = filt_per;
		} else
		{
			cmp->CR1 |= CMP_CR1_SE_MASK;
		}
	} else if(config.mode == CMP_MODE_WINDOWED)
	{
		cmp->CR1 |= CMP_CR1_WE_MASK;
	}

	interrupt_config[config.cmp_num] = config.interrupt;

	if(config.use_internal_dac)
	{
		cmp->DACCR |= CMP_DACCR_DACEN_MASK;
		cmp->DACCR |= config.dac_reference << CMP_DACCR_VRSEL_SHIFT;
		cmp->DACCR |= (config.dac_value - 1) << CMP_DACCR_VOSEL_SHIFT;
	}

	if(!CMP_configure_input_pin(config.cmp_num, config.pos_in) ||
	   !CMP_configure_input_pin(config.cmp_num, config.neg_in))
	{
	    return false;
	}

	cmp->MUXCR |= config.pos_in << CMP_MUXCR_PSEL_SHIFT;
	cmp->MUXCR |= config.neg_in << CMP_MUXCR_MSEL_SHIFT;

	init[config.cmp_num] = true;

	return true;
}

static bool CMP_configure_input_pin(uint8_t cmp_num, CMP_input_t input)
{
    switch(cmp_num)
    {
        case 0:
            switch(input)
            {
                case CMP_IN_0:
                    PORTC->PCR[6] &= ~PORT_PCR_MUX_MASK;
                    break;

                case CMP_IN_1:
                    PORTC->PCR[7] &= ~PORT_PCR_MUX_MASK;
                    break;

                case CMP_IN_2:
                    PORTC->PCR[8] &= ~PORT_PCR_MUX_MASK;
                    break;

                case CMP_IN_3:
                    PORTC->PCR[9] &= ~PORT_PCR_MUX_MASK;
                    break;

                case CMP_IN_4:
                    break;

                case CMP_IN_5:
                    break;

                case CMP_IN_6:
                    break;

                case CMP_IN_7:
                    break;

                default:
                    return false;
            }
            break;


        case 1:
            switch(input)
            {
                case CMP_IN_0:
                    PORTC->PCR[2] &= ~PORT_PCR_MUX_MASK;
                    break;

                case CMP_IN_1:
                    PORTC->PCR[3] &= ~PORT_PCR_MUX_MASK;
                    break;

                case CMP_IN_2:
                    break;

                case CMP_IN_3:
                    break;

                case CMP_IN_4:
                    return false;

                case CMP_IN_5:
                    break;

                case CMP_IN_6:
                    break;

                case CMP_IN_7:
                    break;

                default:
                    return false;
            }
            break;


        case 2:
            switch(input)
            {
                case CMP_IN_0:
                    PORTA->PCR[12] &= ~PORT_PCR_MUX_MASK;
                    break;

                case CMP_IN_1:
                    PORTA->PCR[13] &= ~PORT_PCR_MUX_MASK;
                    break;

                case CMP_IN_2:
                    break;

                case CMP_IN_3:
                    break;

                case CMP_IN_4:
                case CMP_IN_5:
                case CMP_IN_7:
                    return false;

                case CMP_IN_6:
                    break;

                default:
                    return false;
            }
            break;
        default:
            return false;
    }

    return true;
}

bool CMP_enable(uint8_t cmp_num)
{
	if(cmp_num > 2 || !init[cmp_num])
	{
		return false;
	}

	CMP_Type * cmp = cmps[cmp_num];

    cmp->SCR |= CMP_SCR_CFR_MASK | CMP_SCR_CFF_MASK;

	if(interrupt_config[cmp_num] != CMP_INT_DISABLED)
	{
		if(interrupt_config[cmp_num] == CMP_INT_FALLING_EDGE ||
				interrupt_config[cmp_num] == CMP_INT_BOTH_EDGES)
		{
			cmp->SCR |= CMP_SCR_IEF_MASK;
		}

		if(interrupt_config[cmp_num] == CMP_INT_RISING_EDGE ||
				interrupt_config[cmp_num] == CMP_INT_BOTH_EDGES)
		{
			cmp->SCR |= CMP_SCR_IER_MASK;
		}

		NVIC_ClearPendingIRQ(cmps_IRQn[cmp_num]);
		NVIC_EnableIRQ(cmps_IRQn[cmp_num]);
	}

	cmp->CR1 |= CMP_CR1_EN_MASK;

	return true;
}

bool CMP_disable(uint8_t cmp_num)
{
	if(cmp_num > 2 || !init[cmp_num])
	{
		return false;
	}

	CMP_Type * cmp = cmps[cmp_num];

	if(interrupt_config[cmp_num] != CMP_INT_DISABLED)
	{
		if(interrupt_config[cmp_num] == CMP_INT_FALLING_EDGE ||
				interrupt_config[cmp_num] == CMP_INT_BOTH_EDGES)
		{
			cmp->SCR &= ~CMP_SCR_IEF_MASK;
		}

		if(interrupt_config[cmp_num] == CMP_INT_RISING_EDGE ||
				interrupt_config[cmp_num] == CMP_INT_BOTH_EDGES)
		{
			cmp->SCR &= ~CMP_SCR_IER_MASK;
		}

		NVIC_DisableIRQ(cmps_IRQn[cmp_num]);
		NVIC_ClearPendingIRQ(cmps_IRQn[cmp_num]);
	}

	cmp->CR1 &= ~CMP_CR1_EN_MASK;

	return true;
}

bool CMP_read_output(uint8_t cmp_num, bool *output)
{
	if(cmp_num > 2 || output == (bool *)0 || !init[cmp_num])
	{
		return false;
	}

	*output = cmps[cmp_num]->SCR & CMP_SCR_COUT_MASK;

	return true;
}

bool CMP_set_callback(uint8_t cmp_num, CMP_callback_t callback)
{
	if(cmp_num > 2 || !init[cmp_num])
	{
		return false;
	}

	callbacks[cmp_num] = callback;

	return true;
}

static void CMP_IRQHandler(uint8_t cmp_num)
{
    CMP_Type *cmp = cmps[cmp_num];

    uint8_t flags = cmp->SCR & (CMP_SCR_CFR_MASK | CMP_SCR_CFF_MASK);

    cmp->SCR |= flags;

    if(callbacks[cmp_num] != (CMP_callback_t)0)
    {
        callbacks[cmp_num]();
    }
}

void CMP0_IRQHandler(void)
{
	CMP_IRQHandler(0);
}

void CMP1_IRQHandler(void)
{
	CMP_IRQHandler(1);
}

void CMP2_IRQHandler(void)
{
	CMP_IRQHandler(2);
}
