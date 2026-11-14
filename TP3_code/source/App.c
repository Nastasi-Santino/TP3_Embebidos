/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "version.h"
#include "hardware.h"
#include "dma.h"
#include "gpio.h"

#define DMA_CHANNEL         0U
#define DMA_REQUEST_PORTA   49U

#define SW3                 PORTNUM2PIN(PA, 4)
#define RED_LED             PORTNUM2PIN(PB, 22)

#define BUFFER_SIZE         4U
/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/


/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/
static void DMA_done_callback(void);
volatile bool button_state;

/*******************************************************************************
 * STATIC VARIABLES
 ******************************************************************************/

//static version_t version;

static uint32_t source_buffer[BUFFER_SIZE] =
{
    0x11111111,
    0x22222222,
    0x33333333,
    0x44444444
};

static volatile uint32_t destination_buffer[BUFFER_SIZE] =
{
    0,
    0,
    0,
    0
};

static volatile bool dma_init_ok;
static volatile bool dma_config_ok;
static volatile bool dma_callback_ok;
static volatile bool dma_enable_ok;

/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/


void App_Init (void)
{
	/*
	version = read_version();

	if(version == VERSION_1)
	{

	} else if(version == VERSION_2)
	{

	}
	*/

	 /* =========================================================
	     * LED ROJO - PTB22
	     * ========================================================= */

	    gpioMode(RED_LED, OUTPUT);

	    /* LED RGB activo en LOW */
	    gpioWrite(RED_LED, HIGH);


	    /* =========================================================
	     * SW3 - PTA4
	     * ========================================================= */

	    gpioMode(SW3, INPUT_PULLUP);

	    /*
	     * SW3:
	     * suelto     -> HIGH
	     * presionado -> LOW
	     *
	     * IRQC = 0010:
	     * DMA request on falling edge
	     */
	    PORTA->PCR[4] &= ~PORT_PCR_IRQC_MASK;
	    PORTA->PCR[4] |= PORT_PCR_IRQC(2);

	    /* Limpiar flag previo */
	    PORTA->ISFR = (1U << 4);


	    /* =========================================================
	     * DMA
	     * =========================================================

	    dma_init_ok = DMA_init();

	    dma_config_ok = DMA_configure_channel(
	            DMA_CHANNEL,
	            DMA_REQUEST_PORTA,

	            (volatile void *)source_buffer,
	            (volatile void *)destination_buffer,

	            sizeof(uint32_t),      // 4 bytes

	            sizeof(uint32_t),      // source avanza 4
	            sizeof(uint32_t),      // destination avanza 4

	            BUFFER_SIZE,           // 4 requests = major loop

	            false                  // sin ping-pong
	    );

	    dma_callback_ok =
	            DMA_set_callback(DMA_CHANNEL, DMA_done_callback);

	    dma_enable_ok =
	            DMA_enable_channel(DMA_CHANNEL);
	            */

}

/* Función que se llama constantemente en un ciclo infinito */
void App_Run (void)
{
	/*
	if(version == VERSION_1)
	{

	} else if(version == VERSION_2)
	{

	}
	*/
	 button_state = gpioRead(SW3);
}


/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

static void DMA_done_callback(void)
{
    /* Toggle LED rojo */
	gpioToggle(RED_LED);
}

/*******************************************************************************
 ******************************************************************************/
