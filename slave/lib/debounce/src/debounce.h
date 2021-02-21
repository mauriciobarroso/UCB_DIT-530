/*
 * debounce.h
 *
 * Created on: Jan 30, 2021
 * Author: Mauricio Barroso Benavides
 */

#ifndef _DEBOUNCE_H_
#define _DEBOUNCE_H_

/* inclusions ----------------------------------------------------------------*/

#include "Particle.h"

/* cplusplus -----------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* macros --------------------------------------------------------------------*/

#define DEBOUNCE_TIME 40  /* Debounce time in ms */

/* typedef -------------------------------------------------------------------*/

typedef void (* button_cb_t)(void);

typedef struct
{
  button_cb_t function_cb;          /* callback function */
  uint16_t pin;                     /* GPIO pin */
  bool state;                       /* pin state */
  bool last_state;                  /* pin last state */
  unsigned long last_bounce_time;   /* last debounce time */
} debounce_t;

/* external data declaration -------------------------------------------------*/

/* external functions declaration --------------------------------------------*/

void debounce_init(debounce_t * const button, button_cb_t function_cb, uint16_t pin, bool mode);
void debounce_button(debounce_t * const button);

/* cplusplus -----------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

/** @} doxygen end group definition */

/* end of file ---------------------------------------------------------------*/

#endif /* #ifndef _DEBOUNCE_H_ */
