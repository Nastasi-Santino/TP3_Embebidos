#ifndef _BOARD_H_
#define _BOARD_H_

#include "gpio.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/***** BOARD defines **********************************************************/

#define VERSION_PIN PORTNUM2PIN(PB, 9)

#define TP1_PIN		PORTNUM2PIN(PB, 23)
#define TP2_PIN		PORTNUM2PIN(PC, 2)
#define TP3_PIN		PORTNUM2PIN(PC, 3)
#define TP4_PIN		PORTNUM2PIN(PC, 4)
#define TP10_PIN	PORTNUM2PIN(PC, 10)

/*******************************************************************************
 ******************************************************************************/

#endif // _BOARD_H_
