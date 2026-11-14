#include "version.h"
#include "board.h"

version_t read_version(void)
{
	gpioMode(VERSION_PIN, INPUT);
	if(gpioRead(VERSION_PIN) == HIGH)
	{
		return VERSION_1;
	} else
	{
		return VERSION_2;
	}
}
