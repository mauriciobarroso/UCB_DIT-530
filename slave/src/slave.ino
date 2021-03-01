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

#define GET_SENSORS_TIME        3000    /*!< */
#define SEND_TIME               60000
#define SAMPLES_NUMBER          SEND_TIME / GET_SENSORS_TIME    /*!< */

#define UBIDOTS_TOKEN           "BBFF-IqtgnnPsgEM8emrSyayEzf9SEPfmdP"   /*!<  */
#define UBIDOTS_SLAVE_LABEL     "device_1"
#define UBIDOTS_MASTER_LABEL    "master"

#define MAX_DELAY               0xFFFFFFFF                  /*!< Maximum delay value */

#define BUTTON_PIN              D3  /* Button pin */
#define POLLING_ACTIVATOR       10  /* BUtton polling time in ms */

#define DHT_PIN                 A5  /* DHT sensor pin */

/* typedef -------------------------------------------------------------------*/

/* Application struct */
typedef enum
{
    SENSORS_DATA = 0,   /*!< Sensors data type */
    ACTIVATOR_DATA,     /*!< Pointer to data */
    RESPONSE_DATA,
    /* Add more data types here */
} data_type_e;

typedef struct
{
    data_type_e data_type;  /*!< Data type */
    void * data;            /*!< Pointer to data */
} app_t;

/* FSM */
typedef enum
{
    INITIAL_STATE = 0,
    STOP_STATE,         /*!< Pointer to data */
    READY_STATE,        /*!< Pointer to data */
    RUNNING_STATE       /*!< Pointer to data */
} fsm_state_e;

typedef enum
{
    INITIAL_EVENT = 0,
    ACTIVATOR_EVENT,    /*!< Pointer to data */
    ENABLE_EVENT,       /*!< Pointer to data */
    ALARM_EVENT         /*!< Pointer to data */
} fsm_event_e;

typedef struct
{
    fsm_state_e current_state;  /*!< Pointer to data */
    fsm_state_e next_state;     /*!< Pointer to data */
    fsm_event_e event;          /*!< Pointer to data */

} fsm_t;


/* function declaration ------------------------------------------------------*/

/* RTOS functions */
static void app_sensors(void * arg);
static void app_manager(void * arg);
static void app_activator(void * arg);
static void app_response(void * arg);
static void app_fsm(void * arg);

/* Utilities */
static void button_callback(void);

/* FSM functions */
fsm_state_e activator_handler(void);
fsm_state_e enable_handler(void);
fsm_state_e alarm_handler(void);

/* data declaration ----------------------------------------------------------*/

/**/
bool button_flag = 0;

/* Ubidots instance */
Ubidots ubidots(UBIDOTS_TOKEN, UBI_INDUSTRIAL, UBI_HTTP);

/* Components instances */
sensors_t sensors;
debounce_t button;

/* RTOS data */
os_queue_t manager_queue;

/* FSM data */
fsm_t fsm;


/* setup ---------------------------------------------------------------------*/

void setup()
{
    /* Enable thread functions */
    SYSTEM_THREAD(ENABLED);

    /* Initialize serial monitor */
    Serial.begin();

    /**/
    pinMode(D7, OUTPUT);

    /* Initialize componentes instances */
    sensors_init(&sensors, DHT_PIN);
    debounce_init(&button, button_callback, BUTTON_PIN, 1);

    /**/
    os_queue_create(&manager_queue, sizeof(app_t), 4, NULL);

    /* Initialize RTOS functionalities */
    os_thread_create(NULL, "Sensors Task", OS_THREAD_PRIORITY_DEFAULT + 1, app_sensors, (void *)&sensors, OS_THREAD_STACK_SIZE_DEFAULT);
    os_thread_create(NULL, "Activator Task", OS_THREAD_PRIORITY_DEFAULT + 3, app_activator, (void *)&button, OS_THREAD_STACK_SIZE_DEFAULT);
    os_thread_create(NULL, "Manager Task", OS_THREAD_PRIORITY_DEFAULT + 2, app_manager, NULL, OS_THREAD_STACK_SIZE_DEFAULT);
    os_thread_create(NULL, "FSM Task", OS_THREAD_PRIORITY_DEFAULT + 4, app_fsm, NULL, OS_THREAD_STACK_SIZE_DEFAULT);
    os_thread_create(NULL, "Response Task", OS_THREAD_PRIORITY_DEFAULT + 5, app_response, NULL, OS_THREAD_STACK_SIZE_DEFAULT);
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
                    // Serial.print("temp: ");
                    // Serial.println(sensors_values->temp);
                    // Serial.print("hum: ");
                    // Serial.println(sensors_values->hum);

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
                        ubidots.add("state", fsm.current_state);

                        ubidots.send(UBIDOTS_SLAVE_LABEL);

                        /* Reset samples_counter variable */
                        samples_counter = 0;
                    }
                    else
                    {
                        ubidots.add("state", fsm.current_state);
                        ubidots.send(UBIDOTS_SLAVE_LABEL);
                    }

                    break;
                }

                case ACTIVATOR_DATA:
                {
                    Serial.println("Button pressed!");

                    fsm.event = ACTIVATOR_EVENT;

                    break;
                }

                case RESPONSE_DATA:
                {
                    // const char * variables_list = "enable,alarm";
                    // tcpMap my_map = ubidots.getMultipleValues(UBIDOTS_MASTER_LABEL, variables_list);

                    // if((bool)my_map[0])
                    //     fsm.event = ENABLE_EVENT;
                    
                    // if((bool)my_map[1])
                    //     fsm.event = ALARM_EVENT;
                    if(fsm.current_state == READY_STATE)
                    {
                        if((bool)ubidots.get(UBIDOTS_MASTER_LABEL, "enable"))
                        {
                            if(fsm.event != ENABLE_EVENT)
                                fsm.event = ENABLE_EVENT;
                        }
                    }
                    
                    else if(fsm.current_state == RUNNING_STATE)
                    {
                        if((bool)ubidots.get(UBIDOTS_MASTER_LABEL, "alarm"))
                        {
                            if(fsm.event != ALARM_EVENT)
                                fsm.event = ALARM_EVENT;
                        }
                    }

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
    app_t queue;

    /* Initialize app structure */
    queue.data_type = RESPONSE_DATA;
    queue.data = NULL;
    // bool master_response = 0;

    for(;;)
    {
        // master_response = (bool)ubidots.get(UBIDOTS_MASTER_LABEL, "enable");

        // if(master_response)
        // {
        //     Serial.println("Enabled");
        //     fsm.event = ENABLE_EVENT;
        // }
        // else
        // {
        //     Serial.println("Disabled");
        //     fsm.event = ALARM_EVENT;


        // }

        os_queue_put(manager_queue, &queue, 0, NULL);

        /* Wait for ... */
        os_thread_delay_until(&last_wake_time, 3000);
    }
}

static void app_fsm(void * arg)
{
    system_tick_t last_wake_time = 0;
    bool led_status = 0;
    
    fsm.current_state = INITIAL_STATE;
            Serial.println(fsm.current_state);

    for(;;)
    {
        switch(fsm.current_state)
        {
            case INITIAL_STATE:
                fsm.next_state = STOP_STATE;

                break;

            case STOP_STATE:
            {
                digitalWrite(D7, LOW);

                fsm.next_state = activator_handler();

                break;
            }

            case READY_STATE:
            {
                led_status = !led_status;
                digitalWrite(D7, led_status);


                fsm.next_state = enable_handler();

                break;
            }


            case RUNNING_STATE:
            {
                digitalWrite(D7, HIGH);

                fsm.next_state = alarm_handler();

                break;
            }    
            
            default:
                fsm.next_state = INITIAL_STATE;

                break;
        }

        if(fsm.next_state != fsm.current_state)
        {
            fsm.current_state = fsm.next_state;
            Serial.print("State: ");
            Serial.println(fsm.current_state);
        }

        os_thread_delay_until(&last_wake_time, 500);   /* todo: get time with macros */
    }
}

/* Utilities */

static void button_callback(void)
{
    button_flag = 1;    
}

/* FSM functions */
fsm_state_e activator_handler(void)
{
    if(fsm.event == ACTIVATOR_EVENT)
        return READY_STATE;

    return STOP_STATE;
}

fsm_state_e enable_handler(void)
{
    if(fsm.event == ENABLE_EVENT)
        return RUNNING_STATE;

    return READY_STATE;
}

fsm_state_e alarm_handler(void)
{
    if(fsm.event == ALARM_EVENT)
        return STOP_STATE;

    return RUNNING_STATE;
}