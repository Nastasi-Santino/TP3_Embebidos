#include "pdb.h"
#include "hardware.h"

#define BUS_CLOCK_HZ (__CORE_CLOCK__ >> 1)

static PDB_Type * const pdb = PDB0;

static bool init;
static uint16_t configured_mod;

static void set_frequency(uint32_t frequency);

bool PDB_init(PDB_config_t config)
{
	if(config.frequency == 0 || config.trigger_source > PDB_TRIGGER_SOFTWARE || init)
	{
		return false;
	}

	SIM->SCGC6 |= SIM_SCGC6_PDB_MASK;

	pdb->SC = config.trigger_source << PDB_SC_TRGSEL_SHIFT;

	if(config.continuous)
	{
		pdb->SC |= PDB_SC_CONT_MASK;
	}

	set_frequency(config.frequency);


	init = true;

	return true;
}

static void set_frequency(uint32_t frequency)
{
    static const uint16_t prescaler_values[8] = {1, 2, 4, 8, 16, 32, 64, 128};

    static const uint8_t mult_values[4] = {1, 10, 20, 40};

    uint8_t best_prescaler = 0;
    uint8_t best_mult = 0;
    uint16_t best_mod = 0;

    uint64_t best_error = UINT64_MAX;

    if(frequency == 0)
    {
        return;
    }

    for(uint8_t prescaler = 0; prescaler < 8; prescaler++)
    {
        for(uint8_t mult = 0; mult < 4; mult++)
        {
            uint32_t divider = prescaler_values[prescaler] * mult_values[mult];
            uint64_t denominator = (uint64_t)frequency * divider;
            uint64_t count = ((uint64_t)BUS_CLOCK_HZ + denominator / 2) / denominator;

            if(count < 1)
            {
                count = 1;
            }

            if(count > 65536)
            {
                count = 65536;
            }

            uint64_t total_divider = (uint64_t)divider * count;

            uint64_t actual_frequency_mHz = ((uint64_t)BUS_CLOCK_HZ * 1000ULL +
                     total_divider / 2) / total_divider;

            uint64_t requested_frequency_mHz = (uint64_t)frequency * 1000ULL;

            uint64_t error;

            if(actual_frequency_mHz > requested_frequency_mHz)
            {
                error = actual_frequency_mHz - requested_frequency_mHz;
            }
            else
            {
                error = requested_frequency_mHz - actual_frequency_mHz;
            }

            if(error < best_error)
            {
                best_error = error;
                best_prescaler = prescaler;
                best_mult = mult;
                best_mod = (uint16_t)(count - 1);
            }
        }
    }


    pdb->SC &= ~(PDB_SC_PRESCALER_MASK | PDB_SC_MULT_MASK);

    pdb->SC |= PDB_SC_PRESCALER(best_prescaler) | PDB_SC_MULT(best_mult);

    configured_mod = best_mod;
    pdb->MOD = PDB_MOD_MOD(best_mod);

    if(pdb->SC & PDB_SC_PDBEN_MASK)
    {
        pdb->SC |= PDB_SC_LDOK_MASK;
    }
}

bool PDB_configure_adc_trigger(uint8_t adc_num, uint8_t pretrigger, uint16_t delay)
{
    if(!init || adc_num > 1 || pretrigger > 1 || delay > configured_mod)
    {
        return false;
    }

    pdb->CH[adc_num].C1 |= PDB_C1_EN(1U << pretrigger);

    pdb->CH[adc_num].C1 |= PDB_C1_TOS(1U << pretrigger);

    pdb->CH[adc_num].DLY[pretrigger] = PDB_DLY_DLY(delay);

    return true;
}

bool PDB_configure_dac_trigger(uint8_t dac_num, uint16_t interval)
{
	if(!init || dac_num > 1 || interval > configured_mod)
	{
	    return false;
	}

    pdb->DAC[dac_num].INTC = PDB_INTC_TOE_MASK;

    pdb->DAC[dac_num].INT = PDB_INT_INT(interval);

    return true;
}
bool PDB_enable(void)
{
	if(!init)
	{
		return false;
	}

	pdb->SC |= PDB_SC_PDBEN_MASK | PDB_SC_LDOK_MASK;

	return true;
}

bool PDB_disable(void)
{
	if(!init)
	{
		return false;
	}

	pdb->SC &= ~PDB_SC_PDBEN_MASK;

	return true;
}

bool PDB_software_trigger(void)
{

	if(!init)
	{
		return false;
	}

	if((pdb->SC & PDB_SC_PDBEN_MASK) == 0)
	{
		return false;
	}

	if((pdb->SC & PDB_SC_TRGSEL_MASK) != PDB_SC_TRGSEL(PDB_TRIGGER_SOFTWARE))
	{
		return false;
	}

	pdb->SC |= PDB_SC_SWTRIG_MASK;

	return true;
}
