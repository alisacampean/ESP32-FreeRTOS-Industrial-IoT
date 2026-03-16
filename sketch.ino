#include <Wire.h> // protocolul I2C (limbajul pt comunicare cu ecranul OLED )
#include <Adafruit_GFX.h> // biblioteca de grafica (litere,linii)
#include <Adafruit_SSD1306.h> // driverul pt ecranul OLED
#include <DHT.h> // senzor temperatura
#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

// date internet
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// date cloud (MQTT)
WiFiClient espClient;
PubSubClient mqttClient(espClient);
const char* mqtt_server = "broker.hivemq.com";         // serverul public
const char* topic_telemetrie = "alisa/master/senzori"; // canalul tau UNIC de comunicare

// Definirea pinilor
#define DHTPIN 15
#define DHTTYPE DHT22
#define SERVO_PIN 18
#define LED_OK 19
#define LED_ALARM 5

// Initializare obiecte
DHT dht(DHTPIN, DHTTYPE); // obiect cu numele "dht" - senzorul de temperatura
Servo myServo;    // obiect de tip Servo 

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
// &Wire -> trimite automat semnale catre pinii GPIO 21 (SDA) și GPIO 22 (SCL)
// deci nu mai trebuie declarati cu #define
// -1 -> nu avem nevoie de pin de reset
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Variabile globale
float temperaturaCurenta = 0.0;
float umiditateCurenta = 0.0;
bool stareAlarma = false;

// Declarare task-uri FreeRTOS
void TaskCitireSenzor(void *pvParameters);
void TaskDisplayOLED(void *pvParameters);
void TaskControlSistem(void *pvParameters);
void TaskConexiuneWiFi(void *pvParameters);
void TaskMQTT(void *pvParameters);

void setup() {
  Serial.begin(115200);
  
  mqttClient.setServer(mqtt_server, 1883); // 1883 - portul standard MQTT

  // Setare pini LED
  pinMode(LED_OK, OUTPUT);
  pinMode(LED_ALARM, OUTPUT);
  
  // Pornire componente
  dht.begin();
  myServo.attach(SERVO_PIN);
  myServo.write(0); // punem valva la 0 grade 

  // Pornire ecran OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Eroare la pornirea ecranului OLED!"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(WHITE);

  // TASK-URI
  
  xTaskCreate(
    TaskCitireSenzor,   // numele funcției
    "CitireDHT",        // numele task-ului (pentru debug)
    2048,               // nemorie alocată (în bytes)
    NULL,               // parametri (nu avem)
    1,                  // prioritate (1 = mica, 3 = mare)
    NULL                // handle (nu avem nevoie acum)
  );

  xTaskCreate(TaskDisplayOLED, "Display", 2048, NULL, 1, NULL);
  xTaskCreate(TaskControlSistem, "Control", 2048, NULL, 2, NULL); // prioritate mare pt control
  xTaskCreate(TaskConexiuneWiFi, "WiFiTask", 4096 ,NULL ,1 ,NULL);
  xTaskCreate(TaskMQTT, "TaskMQTT", 4096, NULL, 1, NULL);
}

void loop() {
  // in FreeRTOS, loop-ul principa este GOL
}

// FUNCTIILE 

void TaskCitireSenzor(void *pvParameters) {
  // VARIANTA 1 - INTRARE SINUSOIDALA
  for (;;) { 
    // timpul curent al sistemului in secunde
    float timp_secunde = millis() / 1000.0;

    float frecventa = 0.1;     // 0.1 Hz - un ciclu complet la 10 secunde
    float amplitudine = 10.0;
    float offset = 30.0;       // oscileaza in jurul valorii de 30 grade

    float temp_simulata = offset + amplitudine * sin(2 * PI * frecventa * timp_secunde);

    // punem valorile la variabilele globale (ignoram DHT22-ul)
    temperaturaCurenta = temp_simulata;
    umiditateCurenta = 50.0; // umiditatea nu ne intereseaza

    // trimite in consola
    Serial.print("Timp: ");
    Serial.print(timp_secunde);
    Serial.print("s | Temp simulata: ");
    Serial.println(temperaturaCurenta);

    // task-ul doarme doar 100 ms ca sa genereze o unda lina
    vTaskDelay(100 / portTICK_PERIOD_MS); 
  }
/* // VARIANTA 2 - CITESTE DIRECT DE LA SENZOR (PUN EU MANUAL TEMPERATURA)
  for (;;) { 
    // cerem senzorului datele
    float temp = dht.readTemperature();
    float umid = dht.readHumidity();
 
    // isnan() - "Is Not A Number" (verifica daca a dat eroare)
    if (!isnan(temp) && !isnan(umid)) {
        // punem valorile la variabilele globale
        temperaturaCurenta = temp;
        umiditateCurenta = umid;
    } else {
        Serial.println("Eroare la citirea senzorului DHT!");
    }

    // task-ul doarme 2 secunde (DHT22 este un senzor lent, are nevoie de timp)
    vTaskDelay(2000 / portTICK_PERIOD_MS); 
  }
  */
}

void TaskDisplayOLED(void *pvParameters) {
  for (;;) {      // for infinit ca sa nu "moara" task-ul(ca si la while(1) )
    // stergem ecranul vechi
    display.clearDisplay(); 
    
    // coltul stanga-sus (x=0, y=0)
    display.setCursor(0, 0);
    display.setTextSize(1);

    // indicator wifi colt dreapta-sus
    display.setCursor(85, 0);
    
    if (WiFi.status() == WL_CONNECTED) {
        display.print("WiFi:ON");
    } else {
        display.print("WiFi:--");
    }
    
    // desenam interfata
    display.println("Sistem IoT Master");
    display.println("-----------------");

    // las putin spatiu liber (momentan vor fi 0.0)
    display.setCursor(0, 20);
    display.print("Temp: ");
    display.print(temperaturaCurenta);
    display.println(" C");

    display.print("Umid: ");
    display.print(umiditateCurenta);
    display.println(" %");

    // statusul sistemului
    display.setCursor(0, 50);
    if (stareAlarma == true) {
        display.print("STATUS: ALARMA!");
    } else {
        display.print("STATUS: OK");
    }

    // trimite tot ce am desenat catre ecranul fizic
    display.display(); 

    // task-ul doarme 500 ms, intre timp procesorul face alt task
    vTaskDelay(500 / portTICK_PERIOD_MS); 
  }
}

void TaskControlSistem(void *pvParameters) {
  
  // 
  float setpoint = 30.0; // marimea de referinta r(t) - pragul critic

  // Unghi = Kp * Eroare
  // 36 C - 30 C = 6 C    - eroarea maxima permisa
  // => 6(eroare) * 15(Kp) = 90(unghi)
  float Kp = 15.0;       // constanta de proportionalitate

  for (;;) {
    // calculam eroarea: e(t) = y(t) - r(t) 
    float eroare = temperaturaCurenta - setpoint; 

    if (eroare > 0) { // temp > 30 de grade (avem nevoie de racire)
        
        // legea de reglare PROPORTIONALA: u(t) = Kp * e(t)
        float comanda_unghi = Kp * eroare; 
        
        // nu se poate deschide mai mult de 90 grade
        if (comanda_unghi > 90.0) {
            comanda_unghi = 90.0;
        }

        // trimitem comanda catre elem. executie (motoras)
        myServo.write((int)comanda_unghi);

        // actualizam starea
        stareAlarma = true;
        digitalWrite(LED_ALARM, HIGH);
        digitalWrite(LED_OK, LOW);
        
    } else {
        
        myServo.write(0); // inchidem valva complet
        
        stareAlarma = false;
        digitalWrite(LED_ALARM, LOW);
        digitalWrite(LED_OK, HIGH);
    }

    // bucla de control ruleaza la fiecare 100ms (frecventa de esantionare 10Hz)
    vTaskDelay(100 / portTICK_PERIOD_MS); 
  }
}

void TaskConexiuneWiFi(void *pvParameters) {
  // 1. Începem procesul de conectare
  Serial.print("Se conecteaza la WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // 2. Așteptăm până primim confirmarea (statusul devine WL_CONNECTED)
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS); // Așteptăm jumătate de secundă
    Serial.print("."); // Printăm puncte ca să vedem că lucrează
  }

  // 3. Succes! Printăm adresa IP primită de la router
  Serial.println("\n--- CONECTAT LA INTERNET! ---");
  Serial.print("Adresa IP locala: ");
  Serial.println(WiFi.localIP());

  for (;;) {
    // 4. Mentenanța conexiunii (rulată la nesfârșit)
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Conexiune pierduta! Se incearca reconectarea...");
        WiFi.disconnect();
        WiFi.reconnect();
    }
    
    // Verificăm statusul rețelei o dată la 10 secunde
    vTaskDelay(10000 / portTICK_PERIOD_MS); 
  }
}

void TaskMQTT(void *pvParameters) {
  unsigned long timpulUltimeiTrimiteri = 0; // cronometru pt mesaje

  for (;;) {
    // verif daca e conectat la WiFi
    if (WiFi.status() == WL_CONNECTED) {
        
        // daca a picat MQTT-ul, reconectam
        if (!mqttClient.connected()) {
            Serial.println("Se cauta Broker-ul MQTT HiveMQ...");
            String clientId = "ESP32-Alisa-" + String(random(0xffff), HEX);
            
            if (mqttClient.connect(clientId.c_str())) {
                Serial.println(">>> CONECTAT LA CLOUD (MQTT) <<<");
            } else {
                vTaskDelay(3000 / portTICK_PERIOD_MS); // incercam peste 3s
            }
        } else {
            // SUNTEM CONECTAȚI
            mqttClient.loop(); // ACEASTĂ FUNCȚIE TREBUIE SĂ RULEZE DES!

            // Verificăm dacă au trecut 3 secunde (3000 ms) de la ultimul mesaj
            if (millis() - timpulUltimeiTrimiteri > 3000) {
                timpulUltimeiTrimiteri = millis(); // Resetam cronometrul
                
                String mesaj = "Temp: " + String(temperaturaCurenta) + " C | Status: ";
                if (stareAlarma) mesaj += "ALARMA!";
                else mesaj += "OK";
                
                // trimitem mesajul pe internet
                mqttClient.publish(topic_telemetrie, mesaj.c_str());
                
                Serial.print("Trimis in cloud -> ");
                Serial.println(mesaj);
            }
        }
    }
    
    // task-ul doarme foarte puțin (50ms) ca sa poata procesa datele de retea
    vTaskDelay(50 / portTICK_PERIOD_MS); 
  }
}