/*
 * mcp401x.c
 *
 * Created on: Jan 13, 2021
 * Author: Mauricio Barroso Benavides
 */

/* inclusions ----------------------------------------------------------------*/

#include "sensors.h"

/* macros --------------------------------------------------------------------*/

#define TEMP_ERROR  80  /*!< Temperature error threshold in celsius */
#define HUM_ERROR   95  /*!< Humidity error threshold in percentage */

/* typedef -------------------------------------------------------------------*/

/* internal data declaration -------------------------------------------------*/

/* external data declaration -------------------------------------------------*/

/* internal functions declaration --------------------------------------------*/

/* external functions definition ---------------------------------------------*/


bool sensors_init(sensors_t * const me, uint16_t dht_pin)
{
  /* Initialize DHT */
  me->pins.dht = dht_pin;
  me->values.hum = 0;
  me->values.temp = 0;

  /* Return true if everything is OK */
  return true;
}

sensors_values_t sensors_get_values(sensors_t * const me)
{
  /* Initialize DHT instance */
  DHT dht(me->pins.dht, DHT_TYPE);

  /* Get and verify values */
  float temp_tmp = dht.getTempCelcius();
  if(!isnan(temp_tmp) && temp_tmp >= 0 && temp_tmp <= TEMP_ERROR)
    me->values.temp = temp_tmp;

  /* Get and verify values */
  float hum_tmp = dht.getHumidity();
  if(!isnan(hum_tmp) && hum_tmp >= 0 && hum_tmp <= HUM_ERROR)
    me->values.hum = hum_tmp;

  /* Return instance values */
  return me->values;
}

/* internal functions definition ---------------------------------------------*/

/* end of file ---------------------------------------------------------------*/
