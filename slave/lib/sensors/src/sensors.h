/*
 * sensors.h
 *
 * Created on: Jan 30, 2021
 * Author: Mauricio Barroso Benavides
 */

#ifndef _SENSORS_H_
#define _SENSORS_H_

/* inclusions ----------------------------------------------------------------*/

#include "Particle.h"
#include "Adafruit_DHT.h"

/* cplusplus -----------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* macros --------------------------------------------------------------------*/

#define DHT_TYPE  DHT11 /*!< DHT sensor model */

/* typedef -------------------------------------------------------------------*/

typedef struct
{
  uint16_t dht; /*!< DHT pin */
  /* Add more sensors pins here */
} sensors_pins_t;

typedef struct
{
  float hum;      /*!< Humidity variable */
	float temp;     /*!< Temperature variable */
  /* Add more sensors values here */
} sensors_values_t;

typedef struct
{
  sensors_pins_t pins;      /*!< Sensors pins */ 
  sensors_values_t values;  /*!< Sensors values */ 
} sensors_t;

/* external data declaration -------------------------------------------------*/

/* external functions declaration --------------------------------------------*/

bool sensors_init(sensors_t * const me, uint16_t dht_pin);
sensors_values_t sensors_get_values(sensors_t * const me);

/* cplusplus -----------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

/** @} doxygen end group definition */

/* end of file ---------------------------------------------------------------*/

#endif /* #ifndef _SENSORS_H_ */
