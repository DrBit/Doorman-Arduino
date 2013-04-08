
#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>

// Outputs
int DO_LED_GREEN = 9;               // ~ LED Green - Power
const int porta_principal = 8;            //   Relay porta 1
const int porta_jardi = 7;                //   Relay porta 2
const int porta_garatge = 6;              //   Relay porta 3
const int porta_extra = 13;               //   Relay porta 3

const int temperature_pin = 0;
const int ldr_pin = 1;

// Misc
bool isConnectedToLAN = false;

// Ethernet
byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDA, 0x02};
IPAddress ip(10,0,0,2);
//IPAddress ip(192,168,2,2);
// EthernetServer server = EthernetServer(443);
int port = 80;
EthernetServer server(80);

// Accept pending connections
EthernetClient client;

String receivedData;
boolean start_recording = false;

bool clientButtonPressed = false;
bool buttonPressed = false;
bool buttonPressedLast = false;
bool buttonIsActive = false;

unsigned long portaP_time_delay = 0;
unsigned long portaP_start_open = 0;
unsigned long portaJ_time_delay = 0;
unsigned long portaJ_start_open = 0;
unsigned long portaG_time_delay = 0;
unsigned long portaG_start_open = 0;
unsigned long portaE_time_delay = 0;
unsigned long portaE_start_open = 0;


void setup() {
    Serial.begin(9600);

    Serial.println(F("* Starting Garage Door Controller *"));
    
    pinMode(DO_LED_GREEN, OUTPUT);
    pinMode(porta_principal, OUTPUT);
    pinMode(porta_jardi, OUTPUT);
    pinMode(porta_garatge, OUTPUT);
    pinMode(porta_extra, OUTPUT);
    
    digitalWrite(DO_LED_GREEN, HIGH);
    delay(2000);
    digitalWrite(DO_LED_GREEN, LOW);

    while (!isConnectedToLAN) {
        try_lan_connection ();
    }

    Serial.println(F("* Ready"));
   
}


void try_lan_connection () {
    Serial.println (F("Connecting..."));
    Ethernet.begin(mac, ip);
    server.begin();
    // Start listening for connections
    isConnectedToLAN = true;
    Serial.print (F("IP: "));
    Serial.print(Ethernet.localIP());
    Serial.print (F(":"));
    Serial.println (port);
    // Turn on the power LED to indicate that the device is functioning properly
    digitalWrite(DO_LED_GREEN, HIGH);
}


boolean portaP_oberta = false;
boolean portaJ_oberta = false;
boolean portaG_oberta = false;
boolean portaE_oberta = false;
boolean abort_reading = false;
boolean received_slash = false;


void loop() {
    client = server.available();
    // Receive data
    if (client) {
        Serial.println(F("Client connected"));
        boolean currentLineIsBlank = true;
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                //Serial.write(c);

                if (c == '\n' && currentLineIsBlank) {
                    Serial.println(F("Serve Website"));
                    // send a standard http response header
                    client.println(F("HTTP/1.1 200 OK"));
                    client.println(F("Content-Type: text/html"));
                    client.println(F("Connnection: close"));
                    client.println(F(""));
                    client.println(F("<!DOCTYPE HTML>"));
                    client.println(F("<html>"));
                    // add a meta refresh tag, so the browser pulls again every 5 seconds:
                    // client.println("<meta http-equiv=\"refresh\" content=\"20\">");
                    break;
                }
                if (c == '\n') {
                    // you're starting a new line
                    currentLineIsBlank = true;
                    start_recording = false;
                } else if (c != '\r') {
                    // you've gotten a character on the current line
                    currentLineIsBlank = false;
                }

                if (c == '*' && !abort_reading) {
                    start_recording = true;
                    if (receivedData == "PortaPrincipal") {
                        Serial.println(F("||Obre porta principal||"));
                        portaP_oberta = true;
                    }
                    if (receivedData == "PortaJardi") {
                        Serial.println(F("||Obre porta jardi||"));
                        portaJ_oberta = true;
                    }
                    if (receivedData == "PortaGaratge") {
                        Serial.println(F("||Obre/Tanca porta garatge||"));
                        portaG_oberta = true;
                    }
                    if (receivedData == "PortaExtra") {
                        Serial.println(F("||Obre/Tanca porta extra||"));
                        portaE_oberta = true;
                    }
                    receivedData = "";
                }

                if (start_recording && (c != '*')) {
                    receivedData += c;
                }

                if (c == '/') {
                    received_slash = true;
                }

                if (received_slash && (c == '?')) {
                    abort_reading = true;
                    //Serial.print(F("||abort_reading||"));
                }else if (c != '/'){
                    received_slash = false;
                }
            }
        }

        if (portaP_oberta) {
            printHTMLbuttonAction ("Obrint porta principal", "torna");
            obre_sesam (porta_principal,3500);
            portaP_oberta = false;
        }else if (portaJ_oberta) {
            printHTMLbuttonAction ("Obrint porta jardi", "torna");
            obre_sesam (porta_jardi,3500);
            portaJ_oberta = false;
        }else if (portaG_oberta) {
            printHTMLbuttonAction ("Obrint/Tancant la porta del garatge", "torna");
            obre_sesam (porta_garatge,2000);
            portaG_oberta = false;
        }else if (portaE_oberta) {
            printHTMLbuttonAction ("Obrint/Tancant la porta extra", "torna");
            obre_sesam (porta_extra,2000);
            portaE_oberta = false;
        }else{
            // Print all buttons
            printHTMLbutton ("PortaPrincipal","Obre Porta Principal");
            printHTMLbutton ("PortaJardi","Obre Porta Jardi");
            printHTMLbutton ("PortaGaratge","Obre/Tanca Porta Garatge");
            printHTMLbutton ("PortaExtra","Obre/Tanca Porta Extra");
        }
        HTMLend ();

        client.stop();
        Serial.println(F("Close Connection"));
        abort_reading = false;
    }else{
        // Check timings and see if we have to switch of a previously opened door
        check_timings ();
    }
}

void obre_sesam (int porta, int temps) {
    switch (porta) {
        case porta_principal: {
            portaP_start_open = millis();
            portaP_time_delay = temps;
        break;}

        case porta_jardi: {
            portaJ_start_open = millis();
            portaJ_time_delay = temps;
        break;}

        case porta_garatge: {
            portaG_start_open = millis();
            portaG_time_delay = temps;
        break;}

        case porta_extra: {
            portaE_start_open = millis();
            portaE_time_delay = temps;
        break;}
    }
    digitalWrite (porta, HIGH);
    Serial.print (F("Obre porta: "));
    Serial.print (porta);
    Serial.print (F(" - Temps: "));
    Serial.println (temps);
}

void check_timings () {
    if (portaP_time_delay > 0) {
        if ((millis() - portaP_time_delay) > portaP_start_open) {
            digitalWrite (porta_principal, LOW);
            portaP_time_delay = 0;
            Serial.println (F("PortaPrincipal off"));
        }
    }

    if (portaJ_time_delay > 0) {
        if ((millis() - portaJ_time_delay) > portaJ_start_open) {
            digitalWrite (porta_jardi, LOW);
            portaJ_time_delay = 0;
            Serial.println (F("PortaJardi off"));
        }
    }

    if (portaG_time_delay > 0) {
        if ((millis() - portaG_time_delay) > portaG_start_open) {
            digitalWrite (porta_garatge, LOW);
            portaG_time_delay = 0;
            Serial.println (F("Porta_garatge off"));
        }
    }

    if (portaE_time_delay > 0) {
        if ((millis() - portaE_time_delay) > portaE_start_open) {
            digitalWrite (porta_extra, LOW);
            portaE_time_delay = 0;
            Serial.println (F("Porta_extra off"));
        }
    }
}

void printHTMLbutton (char* buttoncall, char* buttontext) {

    client.println(F("<BR>"));
    
    client.println(F("<CENTER>"));

    client.print(F("<FORM ACTION=\"http://"));
    client.print(Ethernet.localIP());
    client.print(F("/"));
    client.print(F("*"));
    client.print(buttoncall);
    client.print(F("*\""));
    client.println(F(" method=get >"));

    client.print(F("<INPUT TYPE=SUBMIT NAME=\"submit\" VALUE=\""));
    client.print(buttontext);
    client.println(F("\" style=\"height:150px;width:400px;font-size:30px;font-weight:bold;\">"));
    
    client.println(F("</FORM>"));
    client.println(F(""));
}

void printHTMLbuttonAction (char* accio, char* returntext) {

    client.println(F("<CENTER><BR><font size=\"30\">"));
    client.println(F("FET!"));
    client.println(F("<BR>"));
    client.println(accio);
    client.println(F("<BR>"));
    
    client.print(F("<FORM ACTION=\"http://"));
    client.print(Ethernet.localIP());
    client.print(F("/\" method=get >"));


    client.print(F("<INPUT TYPE=SUBMIT VALUE=\""));
    client.print(returntext);
    client.println(F("\" style=\"height:150px;width:400px;font-size:30px;font-weight:bold;\">"));
    
    client.println(F("</FORM>"));
    client.println(F("</font></CENTER>"));
}

void HTMLend () {
    //EthernetClient client = server.available();
    client.println(F("</html>"));
    client.println(F(""));
}

fload get_temp () {
    float temp;
    temp = (5.0 * analogRead(temperature_pin) * 100.0) / 1024;
    return temp;
}

fload get_light () {
    float light;
    light = (5.0 * analogRead(ldr_pin) * 100.0) / 1024;
    return light;
}