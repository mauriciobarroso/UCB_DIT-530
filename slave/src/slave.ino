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

#define GET_SENSORS_TIME    3000    /*!< */
#define SEND_TIME           60000
#define SAMPLES_NUMBER      SEND_TIME / GET_SENSORS_TIME    /*!< */

#define UBIDOTS_TOKEN       "BBFF-IqtgnnPsgEM8emrSyayEzf9SEPfmdP"   /*!<  */
// #define UBIDOTS_ID          "BBFF-8cfcac286a3d68dc293682fd2e8a0f886e0"  /*!<  */
#define UBIDOTS_DEVICE_LABEL    "device_2"

#define MAX_DELAY           0xFFFFFFFF                  /*!< Maximum delay value */

#define BUTTON_PIN          D3  /* Button pin */
#define POLLING_ACTIVATOR      10  /* BUtton polling time in ms */

#define DHT_PIN             A5  /* DHT sensor pin */

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
static void app_response(void * arg);

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
os_thread_t activator_handle;
os_queue_t manager_queue;

/* setup ---------------------------------------------------------------------*/

void setup()
{
    /* Enable thread functions */
    SYSTEM_THREAD(ENABLED);

    /* Initialize serial monitor */
    Serial.begin();

    /* Initialize componentes instances */
    sensors_init(&sensors, DHT_PIN);
    debounce_init(&button, button_callback, BUTTON_PIN, 1);

    /**/
    os_queue_create(&manager_queue, sizeof(app_t), 4, NULL);

    /* Initialize RTOS functionalities */
    os_thread_create(NULL, "Sensors Task", OS_THREAD_PRIORITY_DEFAULT + 1, app_sensors, (void *)&sensors, OS_THREAD_STACK_SIZE_DEFAULT);
    os_thread_create(&activator_handle, "Activator Task", OS_THREAD_PRIORITY_DEFAULT + 3, app_activator, (void *)&button, OS_THREAD_STACK_SIZE_DEFAULT);
    os_thread_create(NULL, "Manager Task", OS_THREAD_PRIORITY_DEFAULT + 2, app_manager, NULL, OS_THREAD_STACK_SIZE_DEFAULT);
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
    sensors_values_t samples[SAMPLES_NUMBER];  /* TODO: obtain the number with macros */
    uint8_t samples_counter = 0;

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

                    /* Print data received */
                    Serial.print("temp: ");
                    Serial.println(sensors_values->temp);
                    Serial.print("hum: ");
                    Serial.println(sensors_values->hum);

                    /* Add sensors values samples to buffer */
                    samples[samples_counter++] = * sensors_values;

                    /* Send to Ubidots every SAMPLES_NUMBER samples obtained */
                    if(samples_counter == SAMPLES_NUMBER)
                    {
                        /* Average temp and hum samples */
                        float temp_average = 0;
                        float hum_average = 0;

                        for(uint8_t i = 0; i < samples_counter; i++)
                        {
                            temp_average += samples[i].temp;
                            hum_average += samples[i].hum;
                        }

                        temp_average = temp_average / samples_counter;
                        hum_average = hum_average / samples_counter;

                        /* Prepare and send data to Ubidots */
                        ubidots.add("temp", temp_average);
                        ubidots.add("hum", hum_average);

                        ubidots.send(UBIDOTS_DEVICE_LABEL);

                        /* Reset samples_counter variable */
                        samples_counter = 0;
                    }

                    break;
                }

                case ACTIVATOR_DATA:
                {
                    Serial.println("Button pressed!");
                    
                    ubidots.add("state", TRUE);
                    ubidots.send(UBIDOTS_DEVICE_LABEL);

                    os_thread_create(NULL, "Response Task", OS_THREAD_PRIORITY_DEFAULT + 4, app_response, NULL, OS_THREAD_STACK_SIZE_DEFAULT);

                    os_thread_exit(activator_handle);

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

        /* Wait for POLLING_ACTIVATOR */
        os_thread_delay_until(&last_wake_time, POLLING_ACTIVATOR);
    }
}

static void app_response(void * arg)
{
    system_tick_t last_wake_time = 0;

    for(;;)
    {
        Serial.println("waiting...");

        /* Wait for ... */
        os_thread_delay_until(&last_wake_time, 5000);
    }
}

/* Utilities */
static void button_callback(void)
{
    button_flag = 1;    
}

/* Extern events */