/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#line 1 "/home/mauricio/ucb-iot/UCB_DIT-530/slave/src/slave.ino"
/*
 * slave.ino
 *
 * Created on: Feb 20, 2021
 * Author: 
 */

/* includes ------------------------------------------------------------------*/

#include "Particle.h"

#include "sensors.h"
#include "debounce.h"
#include "Ubidots.h"

/* macros --------------------------------------------------------------------*/

void setup();
void loop();
#line 18 "/home/mauricio/ucb-iot/UCB_DIT-530/slave/src/slave.ino"
#define GET_SENSORS_TIME    2000    /*!< */
#define SAMPLES_NUMBER      10      /*!< */

#define UBIDOTS_TOKEN       "BBFF-IqtgnnPsgEM8emrSyayEzf9SEPfmdP"   /*!<  */
#define UBIDOTS_ID          "BBFF-8cfcac286a3d68dc293682fd2e8a0f886e0"  /*!<  */
#define UBIDOTS_DEVICE_LABEL    "smart_plug"

#define MAX_DELAY           0xFFFFFFFF                  /*!< Maximum delay value */

#define BUTTON_PIN          D3  /* Button pin */
#define POLLING_BUTTON      10  /* BUtton polling time in ms */

/* typedef -------------------------------------------------------------------*/

/* Application struct */
typedef enum
{
    SENSORS_DATA = 0, /*!< Sensors data type */
    ACTIVATOR_DATA,
    /* Add more data types here */
} data_type_e;

typedef struct
{
    data_type_e data_type;  /*!< Data type */
    void * data;            /*!< Pointer to data */
} app_t;


/* function declaration ------------------------------------------------------*/

/* RTOS functions */
static void app_sensors(void * arg);
static void app_manager(void * arg);
static void app_activator(void * arg);

/* Extern events */

/* Utilities */
static void average_samples(void);
static void button_callback(void);

/* data declaration ----------------------------------------------------------*/

bool button_flag = 0;

Ubidots ubidots(UBIDOTS_TOKEN, UBI_INDUSTRIAL, UBI_HTTP);

/* Components instances */
sensors_t sensors;
debounce_t button;

/* RTOS data */
os_queue_t manager_queue;

/* setup ---------------------------------------------------------------------*/

void setup()
{
    /* Enable thread functions */
    SYSTEM_THREAD(ENABLED);

    /* Initialize serial monitor */
    Serial.begin();
    Serial.println("");

    /* Initialize componentes instances */
    debounce_init(&button, button_callback, BUTTON_PIN, 1);

    /**/
    os_queue_create(&manager_queue, sizeof(app_t), 4, NULL);

    /* Initialize RTOS functionalities */
    os_thread_create(NULL, "Sensors Task", OS_THREAD_PRIORITY_DEFAULT + 1, app_sensors, (void *)&sensors, OS_THREAD_STACK_SIZE_DEFAULT);
    os_thread_create(NULL, "Activator Task", OS_THREAD_PRIORITY_DEFAULT + 2, app_activator, (void *)&button, OS_THREAD_STACK_SIZE_DEFAULT);
    os_thread_create(NULL, "Manager Task", OS_THREAD_PRIORITY_DEFAULT + 3, app_manager, NULL, OS_THREAD_STACK_SIZE_DEFAULT);
}

/* infinite Loop -------------------------------------------------------------*/

void loop()
{
    
}

/* function definition -------------------------------------------------------*/

/* RTOS functions */
static void app_sensors(void * arg)
{
    sensors_t * instance = (sensors_t *)arg;
    system_tick_t last_wake_time = 0;
    sensors_values_t values;
    app_t queue;

    /* Initialize app structure */
    queue.data_type = SENSORS_DATA;
    queue.data = (void *)&values;

    for(;;)
    {
        /* Get sensors values */
        values = sensors_get_values(instance);

        /* Send sensors values to Manager Task */
        os_queue_put(manager_queue, &queue, 0, NULL);

        /* Wait for GET_SENSORS_TIME */
        os_thread_delay_until(&last_wake_time, GET_SENSORS_TIME);
    }
}

static void app_manager(void * arg)
{
    app_t queue;
    sensors_values_t array[SAMPLES_NUMBER];  /* TODO: obtain the number with macros */

    for(;;)
    {
        /* Wait for messages */
        if(!os_queue_take(manager_queue, &queue, MAX_DELAY, NULL))
        {
            /*  */
            switch(queue.data_type)
            {
                case SENSORS_DATA:
                {
                    sensors_values_t * sensors_values = (sensors_values_t *)queue.data;

                    break;
                }

                case ACTIVATOR_DATA:
                {
                    Serial.println("Button pressed!");
                    
                    char * voltage_str = (char *)malloc(sizeof(char) * 15);
                    char * current_str = (char *)malloc(sizeof(char) * 15);
                    char * status_str = (char *)malloc(sizeof(char) * 15);

                    sprintf(voltage_str, "120");
                    sprintf(current_str, "1");
                    sprintf(status_str, "1");

                    // ubidots.addContext("voltage", voltage_str);
                    // ubidots.addContext("current", current_str);
                    // ubidots.addContext("status", status_str);

                    ubidots.add("voltage", 220);
                    ubidots.add("current", 2);
                    ubidots.add("status", 0);
                   

                    ubidots.send(UBIDOTS_DEVICE_LABEL);

                    free(voltage_str);
                    free(current_str);
                    free(status_str);

                    break;

                }

                /* Add more cases here */

                default:
                    Serial.println("Unknow");

                    break;
            }
        }
    }
}

static void app_activator(void * arg)
{
    debounce_t * instance = (debounce_t *)arg;
    system_tick_t last_wake_time = 0;
    app_t queue;

    /* Initialize app structure */
    queue.data_type = ACTIVATOR_DATA;
    queue.data = NULL;

    for(;;)
    {
        debounce_button(instance);

        /* Send sensors values to Manager Task if button_flag is 1 */
        if(button_flag)
        {
            os_queue_put(manager_queue, &queue, 0, NULL);
            button_flag = 0;    /* todo: clear if master response is received */
        }

        /* Wait for POLLING_BUTTON */
        os_thread_delay_until(&last_wake_time, POLLING_BUTTON);
    }
}

/* Utilities */
static void button_callback(void)
{
    button_flag = 1;    
}

/* Extern events */