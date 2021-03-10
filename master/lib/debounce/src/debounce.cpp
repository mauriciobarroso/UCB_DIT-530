/*
 * debounce.cpp
 *
 * Created on: Feb 20, 2021
 * Author: Mauricio Barroso Benavides
 */

/* inclusions ----------------------------------------------------------------*/

#include "debounce.h"

/* macros --------------------------------------------------------------------*/

#define TEMP_ERROR  80  /*!< Temperature error threshold in celsius */
#define HUM_ERROR   95  /*!< Humidity error threshold in percentage */

/* typedef -------------------------------------------------------------------*/

/* internal data declaration -------------------------------------------------*/

/* external data declaration -------------------------------------------------*/

/* internal functions declaration --------------------------------------------*/

/* external functions definition ---------------------------------------------*/

void debounce_init(debounce_t * const me, button_cb_t function_cb, uint16_t pin, bool mode)
{

  /* Initialize variables */
  me->last_bounce_time = 0;
  me->function_cb = function_cb;
  me->pin = pin;

  /* Initialization state variables */
  if(mode)
  {
    me->last_state = LOW;
    me->state = HIGH;

    pinMode(pin, INPUT_PULLUP);
  }
  else
  {
    me->last_state = HIGH;
    me->state = LOW;

    pinMode(pin, INPUT_PULLDOWN);
  }
}

void debounce_button(debounce_t * const me)
{
  bool reading = digitalRead(me->pin);

  /* Get tick count if the input change */
  if(reading != me->last_state) 
    me->last_bounce_time = millis();

  /* Execute function callback if the button time pressed is greater than
   * bounce time */
  if((millis() - me->last_bounce_time) > DEBOUNCE_TIME) 
  {
    if(reading != me->state) 
    {
      me->state = reading;

      if(me->state == LOW) 
        me->function_cb();
    }
  }

  /* Update the button last state */
  me->last_state = reading;
}

/* internal functions definition ---------------------------------------------*/

/* end of file ---------------------------------------------------------------*/
