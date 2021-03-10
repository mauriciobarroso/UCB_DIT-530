/*
 * master.ino
 *
 * Created on: Feb 28, 2021
 * Author: Grupo 3
 */

/* includes ------------------------------------------------------------------*/

#include <Ubidots.h>
#include <Adafruit_DHT.h>
#include <SPI.h>
#include <MFRC522.h>

/* macros --------------------------------------------------------------------*/

#define DHTPIN 2     // what pin we're connected to (D2)
#define DHTTYPE DHT22	// DHT 22 (AM2302)
#define SLAVES_NUM 2

/* typedef -------------------------------------------------------------------*/

/* function declaration ------------------------------------------------------*/

/* data declaration ----------------------------------------------------------*/

constexpr uint8_t RST_PIN = D4; //Definición dle pin reset de RFID    
constexpr uint8_t SS_PIN = A5;  //Definición del pin SS de RFID
String sendt = "";
String sendh = "";
DHT dht(DHTPIN, DHTTYPE);

const char *WEBHOOK_NAME= "Ubitemp";       //Declaramos el webhook configurado en la consola de Particle
Ubidots ubidots("webhook", UBI_PARTICLE); //Se define el nombre de la función Ubidots
Ubidots ubidots_http("BBFF-eUcHIPZsJaF8wQh5BzC2dLmsOtOsRl",UBI_INDUSTRIAL,UBI_HTTP);
String mydeccard = ""; //Registro de ID del tag que se pase por el sensor RFID
int led = D7; //Primer led que indica la espera de confirmación de los esclavos
int led2 = D6; //Segundo led que indica inicio de sensado de temperatura y humedad
int sensado = 0; //Variable para iniciar a sensar temperatura y humedad
bool dev = 0; //Variable para detectar la confirmación de los esclavos
int slaves_state[SLAVES_NUM] ={0};

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

/* setup ---------------------------------------------------------------------*/

void setup() 
{
    //Se define los pin de leds como salida
    pinMode(led, OUTPUT); 
    pinMode(led2, OUTPUT);
    //Se inicia la comunicación serial
    Serial.begin(115200); 
    //en caso de no existir comunicación serial, no realizar ninguna acción
    while (!Serial);
    //Inicialización de SPI bus
    SPI.begin();    
    //Se inicializa el sensor DHT
    dht.begin();
    //Inicialización de RFID
    mfrc522.PCD_Init(); 
    // Subscribe to the integration response event
    //Particle.subscribe("maestro", confirmacion); // confirmación del maestro para iniciar tareas
    Particle.subscribe("alarm",alarma); //alarma de emergencia.
    //Particle.subscribe("temp2",temperatura2);// temperatura 2
    Particle.subscribe("state", listo); //estado de funcionamiento: 1 stop, 2 ready , 3 running
    ubidots_http.add("alarm", 0);
    ubidots_http.add("enable", 0);
    ubidots_http.send("master");
}

/* function definition -------------------------------------------------------*/

//Recibimos la temperatura del dispositivo 2
void temperatura2(const char *event, const char *data)
{
    digitalWrite(led2, HIGH);
}
//Recibimos el ok de parte de UbiDots para iniciar tareas
void confirmacion(const char *event, const char *data)
{
    int dato=atoi(data);
    Particle.publish(String(dato));
    digitalWrite(led2, HIGH);
}

//Leemos de la Particle Cloud la notificación de que el equipo 1 esta listo para iniciar su labor
void listo (const char *eventName, const char *data)
{
        int dato=atoi(data);
        int device_state=dato%10; //12 ----->2
        int device_id=dato/10; //12 ----->1
        //Particle.publish(String(digito2));
        //Serial.println(dato);
        //Particle.publish("Dispositivo esclavo listo");
        slaves_state[device_id-1]=device_state;
        int contador_ready=0;
        int contador_stop=0;
        int contador_running=0;
        for(int i=0;i<SLAVES_NUM;i++)
        {
            if(slaves_state[i]==1)
            {
                contador_stop++;
            }
            if(slaves_state[i]==2)
            {
                contador_ready++;
            }
            if(slaves_state[i]==3)
            {
                contador_running++;
            }
        }
        contador_ready==SLAVES_NUM ? dev=1 : dev=0;
        if(contador_running==SLAVES_NUM)
        {
            ubidots_http.add("enable", 0);
            ubidots_http.send("master");
        }
        if(contador_stop==SLAVES_NUM)
        {
            ubidots_http.add("alarm", 0);
            ubidots_http.send("master");
        }
}
//Cualquier evento que se lance a la particle cloud e inicie con el prefijo alarma nos llegará y activará esta rutina
//deteneiendo el funcionamiento de nuestras máquinas.
void alarma(const char *event, const char *data)
{
        //int dato=atoi(data);
        //Serial.println(dato);
        sensado=0;
        dev=0;
        digitalWrite(led2, LOW);
        mydeccard = "";
        ubidots_http.add("alarm", 1);
        ubidots_http.send("master");
        Particle.publish("Operación detenida");
        
}  
/*
 * Helper routine to dump a byte array as hex values to Serial.
 */
void dump_byte_array(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        //Serial.print(buffer[i] < 0x10 ? " 0" : " ");
        //Serial.print(buffer[i], HEX);
        mydeccard += buffer[i];
    }
}


//Se inicia el sensado de temperatura y humedad
void TH()
{
    while(sensado==1)
    {
        //LED de confirmación del sensado
        digitalWrite(led2, HIGH);
        float h = dht.getHumidity();
        float t = dht.getTempCelcius();
        sendh = String::format ("%.1f", h);
        sendt = String::format ("%.1f C", t);
        //En caso de existir errores de sensado enviar un mensaje
        if (isnan(h) || isnan(t)) 
        {
		    Particle.publish("No se leyó el sensor");
	    }
	    else if(t > 35)
	    {
	        alarma("a","a");
	    }
        else
        {
            //Publicación de la temperatura y humedad a Ubidots
            Particle.publish("temp",sendt, PRIVATE);
            ubidots_http.add("temp", t);
            ubidots_http.add("hum", h);
            ubidots_http.send("master");
        }
        delay(10000);
    }
}    

/* infinite Loop -------------------------------------------------------------*/

void loop() {
    digitalWrite(led, HIGH);
    //ubidots.add("enable", 1);
    //ubidots.send(WEBHOOK_NAME, PUBLIC);
    //En caso de recibir 2 confirmaciones se espera la detección de RFID
    if(dev==1)
    {
        digitalWrite(led, LOW);
        //Mensaje de confirmación de los dispositivos
        Particle.publish("Dispositivos listos");
        if ( ! mfrc522.PICC_IsNewCardPresent())
            {
                return;}
            // Select one of the cards
        if ( ! mfrc522.PICC_ReadCardSerial())
            {
                return;}
        //Función de detección del valor de la tarjeta RFID
        dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
        //En caso de detectar la tarjeta correcta en el RFID inciar el sensado
        if(mydeccard == "1443516250")
        {
            ubidots_http.add("enable", 1);
            ubidots_http.send("master");
            Particle.publish("Enable listo");
            sensado=1;
            delay(1000);
            mydeccard = "";
            TH();
        }
    }
  }
