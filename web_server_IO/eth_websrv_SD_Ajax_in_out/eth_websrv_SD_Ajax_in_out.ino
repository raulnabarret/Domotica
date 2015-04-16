/*--------------------------------------------------------------
  Program:      eth_websrv_SD_Ajax_in_out

  Description:  Arduino web server that displays 4 analog inputs,
                the state of 3 switches and controls 4 outputs,
                2 using checkboxes and 2 using buttons.
                The web page is stored on the micro SD card.
  
  Hardware:     Arduino Uno and official Arduino Ethernet
                shield. Should work with other Arduinos and
                compatible Ethernet shields.
                2Gb micro SD card formatted FAT16.
                A2 to A4 analog inputs, pins 2, 3 and 5 for
                the switches, pins 6 to 9 as outputs (LEDs).
                
  Software:     Developed using Arduino 1.0.5 software
                Should be compatible with Arduino 1.0 +
                SD card contains web page called index.htm
  
  References:   - WebServer example by David A. Mellis and 
                  modified by Tom Igoe
                - SD card examples by David A. Mellis and
                  Tom Igoe
                - Ethernet library documentation:
                  http://arduino.cc/en/Reference/Ethernet
                - SD Card library documentation:
                  http://arduino.cc/en/Reference/SD

  Date:         4 April 2013
  Modified:     19 June 2013
                - removed use of the String class
 
  Author:       W.A. Smith, http://startingelectronics.com
--------------------------------------------------------------*/


#include <SPI.h>          //Include 
#include <Ethernet.h>     //Include para el Ethernet Shield
#include <SD.h>           //Include para tarjeta SD
#define REQ_BUF_SZ   60   // Tamaño del bufer usado para capturar pedidos HTTP 

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //Direccion MAC, por default puede utilizar esa
IPAddress ip(192, 168, 1, 68);                       //Direccion IP a asignar dentro de la red en la que se encuentra
EthernetServer server(80);                           //Creamos el servidor que escuchara en el puerto 80, por default
File webFile;                                        //Variable que almacenara la pagina web
char HTTP_req[REQ_BUF_SZ] = {0};                     //Pedido HTTP almacenado como null al terminar la cadena
char req_index = 0;                                  //Index dentro del bufer HTTP_req
boolean LED_state[4] = {0};                          //Almacena el valor de los LEDs

void setup()
{
    //Desactivamos el shield de Ethernet 
    pinMode(10, OUTPUT);       //Pin declarado como salida
    digitalWrite(10, HIGH);    //Encendemos la salida digital en el pin 10
    Serial.begin(9600);        //Inicializamos el puerto serial para debuggear
    
    //Inicializamos la memoria SD
    Serial.println("Inicializando la Memoria SD");
    if (!SD.begin(4)) {                                            //Si en el pin 4 no se ha inicializado la memoria SD
        Serial.println("404: No se encontro la memoria");          //Imprimimos el error
        return;                                                    //Salimos del metodo al encontrar el error
    }
    Serial.println("2OO: Memoria inicializada");                   //Si se inicializa la memoria SD  
    if (!SD.exists("index.htm")) {                                 //Buscamos el archivo index.htm, si no existe     
        Serial.println("404: No se encontro el archivo");          //Imprimimos el error
        return;                                                    //Salimos del metodo al encontrar el error
    }
    Serial.println("200: Se encontro el archivo");                 //Imprimimos si el archivo fue encontrado   
    //Declaramos los pines de SWITCHES como entradas
    pinMode(2, INPUT);
    pinMode(3, INPUT);
    pinMode(5, INPUT);
    //Declaramos los pines LEDs como salidas
    pinMode(6, OUTPUT);
    pinMode(7, OUTPUT);
    pinMode(8, OUTPUT);
    pinMode(9, OUTPUT);
    
    Ethernet.begin(mac, ip);                                       //Inicializamos el shield Ethernet con la direccion MAC e IP declaradas anteriormente
    server.begin();                                                //Empezamos a escuchar al cliente
}

void loop()
{
    EthernetClient client = server.available();                    //Tratamos de conectar con el cliente

    if (client) {                                                  //Si existe cliente, es decir, si el servidor esta disponible
        boolean currentLineIsBlank = true;                         //Seteamos la bandera como verdadera
        while (client.connected()) {                               //Mientras que el servidor este disponible y
            if (client.available()) {                              //Si el servidor esta conectado
                char c = client.read();                            //Leemos y almacenamos en la variable c, char por char del servidor
                                                                   //Limitamos el tamaño del pedido HTTP almacenado y recivido 
                                                                   //Almacenamos la primera parte del pedido HTTP en la variable(array) HTTP_req 
                                                                   //Dejamos el ultimo elemento en el array como 0 a un string null (REQ_BUF_SZ - 1)
                if (req_index < (REQ_BUF_SZ - 1)) {                //Si el indice es menor que el tamaño del bufer menos 1
                    HTTP_req[req_index] = c;                       //El valor del char por char en la variable c es asignado al indice en el array HTTP_req
                    req_index++;                                   //Incrementamos en uno el indice
                }
                if (c == '\n' && currentLineIsBlank) {             //Si la ultima linea del servidor es un salto de linea y la bandera currentLineisBlank es verdadera
                    client.println("HTTP/1.1 200 OK");             //Enviamos la primera parte de una respuesta estandar OK
                    if (StrContains(HTTP_req, "ajax_inputs")) {    //Si el request HTTP contiene ajax_inputs, enviamos el resto del encabezado HTTP:
                        client.println("Content-Type: text/xml");  //Tipo de contenido xml
                        client.println("Connection: keep-alive");  //Mantener la conexion
                        client.println();
                        SetLEDs();                                 //Llamamos al metodo SetLEDs
                        XML_response(client);                      //Enviamos un XML conteniendo los estados de las entradas 
                    }
                    else {                                         //Si el request HTTP es de la pagina web, enviamos el resto del encabezado HTTP:
                        client.println("Content-Type: text/html"); //Tipo de contenido html
                        client.println("Connection: keep-alive");  //Mantener la conexion
                        client.println();                          //Enviamos la pagina web
                        webFile = SD.open("index.htm");            //Abrimos la pagina web que se almacena en la variable webFile, a su vez webFile, abre en la memoria SD el archivo index.htm
                        if (webFile) {                             //Si se abre el archivo index.htm
                            while(webFile.available()) {           //Mientras que index.htm este disponible
                                client.write(webFile.read());      //Escribimos en el cliente, la lectura del archivo index.htm
                            }
                            webFile.close();                       //Una vez que se ha acabado el archivo, cerramos index.htm
                        }
                    }
                    Serial.print(HTTP_req);                        //Imprimimos en el serial la solicitud HTTP 
                    req_index = 0;                                 //Reiniciamos req_index a 0
                    StrClear(HTTP_req, REQ_BUF_SZ);                //Limpiamos HTTP_req y REQ_BUF_SZ
                    break;
                }
                if (c == '\n') {                                   //Si la ultima linea recibida por el cliente es un salto de linea
                    currentLineIsBlank = true;                     //Ponemos la bandera currentLineIsBlank como verdadera
                } 
                else if (c != '\r') {                              //Si la ultima linea recibida no termina con un salto de linea
                    currentLineIsBlank = false;                    //Un caracter fue recibido desde el cliente y la bandera currentLineIsBlank se pone como falsa
                }
            } //Cerramos if (client.available())
        } //Cerramos while (client.connected())
        delay(1);                                                 //Creamos un delay de 1 ms para permitirle al servidor recibir datos
        client.stop();                                            //Cerramos la conexion
    } //Cerramos if (client)
}

void SetLEDs(void)                                       //Revisamos si un pedido HTTP de ON/OFF es recibido y tambien guardamos el estado de los LEDs
{
    // LED 1 (pin 6)
    if (StrContains(HTTP_req, "LED1=1")) {               //Si el url del HTTP_req coincide con el numero del LED
        LED_state[0] = 1;                                //Guardamos el estado   
        digitalWrite(6, HIGH);                           //Si recibe un 1, envia al pin una señal HIGH
    }
    else if (StrContains(HTTP_req, "LED1=0")) {          
        LED_state[0] = 0;                                //Guardamos el estado 
        digitalWrite(6, LOW);                            //Si recibe un 1, envia al pin una señal LOW
    }
//    // LED 2 (pin 7)
//    if (StrContains(HTTP_req, "LED2=1")) {
//        LED_state[1] = 1;  
//        digitalWrite(7, HIGH);
//    }
//    else if (StrContains(HTTP_req, "LED2=0")) {
//        LED_state[1] = 0;  
//        digitalWrite(7, LOW);
//    }
//    // LED 3 (pin 8)
//    if (StrContains(HTTP_req, "LED3=1")) {
//        LED_state[2] = 1;  
//        digitalWrite(8, HIGH);
//    }
//    else if (StrContains(HTTP_req, "LED3=0")) {
//        LED_state[2] = 0;  
//        digitalWrite(8, LOW);
//    }
//    // LED 4 (pin 9)
//    if (StrContains(HTTP_req, "LED4=1")) {
//        LED_state[3] = 1;  
//        digitalWrite(9, HIGH);
//    }
//    else if (StrContains(HTTP_req, "LED4=0")) {
//        LED_state[3] = 0;  
//        digitalWrite(9, LOW);
//    }
}

// send the XML file with analog values, switch status
//  and LED status
void XML_response(EthernetClient cl)
{
    int analog_val;            // stores value read from analog inputs
    int count;                 // used by 'for' loops
    int sw_arr[] = {2, 3, 5};  // pins interfaced to switches
    
    cl.print("<?xml version = \"1.0\" ?>");
    cl.print("<inputs>");
    // read analog inputs
    for (count = 2; count <= 5; count++) { // A2 to A5
        analog_val = analogRead(count);
        cl.print("<analog>");
        cl.print(analog_val);
        cl.println("</analog>");
    }
    // read switches
    for (count = 0; count < 3; count++) {
        cl.print("<switch>");
        if (digitalRead(sw_arr[count])) {
            cl.print("ON");
        }
        else {
            cl.print("OFF");
        }
        cl.println("</switch>");
    }
    // checkbox LED states
    // LED1
    cl.print("<LED>");
    if (LED_state[0]) {
        cl.print("checked");
    }
    else {
        cl.print("unchecked");
    }
    cl.println("</LED>");
    // LED2
    cl.print("<LED>");
    if (LED_state[1]) {
        cl.print("checked");
    }
    else {
        cl.print("unchecked");
    }
     cl.println("</LED>");
    // button LED states
    // LED3
    cl.print("<LED>");
    if (LED_state[2]) {
        cl.print("on");
    }
    else {
        cl.print("off");
    }
    cl.println("</LED>");
    // LED4
    cl.print("<LED>");
    if (LED_state[3]) {
        cl.print("on");
    }
    else {
        cl.print("off");
    }
    cl.println("</LED>");
    
    cl.print("</inputs>");
}

// sets every element of str to 0 (clears array)
void StrClear(char *str, char length)
{
    for (int i = 0; i < length; i++) {
        str[i] = 0;
    }
}

// searches for the string sfind in the string str
// returns 1 if string found
// returns 0 if string not found
char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return 1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}
