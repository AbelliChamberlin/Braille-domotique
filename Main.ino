#include <IRremote.h>
#include <Stepper.h>
#include <SoftwareSerial.h>
#include <dht_nonblocking.h>
#include <DHT.h>
#define DHT_SENSOR_TYPE DHT11


//Telecommande
const int RECV_PIN = 6;   
    
#define SIG_UP 16748655 // Up
#define SIG_DOWN 16769055 // Down
#define SIG_STOP 16738455 // 0
#define SIG_LIGHTON  16724175 // 1
#define SIG_LIGHTOFF  16716015 // 4
#define SIG_LIGHTVRF  16728765 // 7
#define SIG_DOOROPN  16743045 // 3
#define SIG_DOORCLS  16734885 // 6
#define SIG_DOORVRF  16732845 // 9
#define SIG_TEMP  16753245  // OFF
#define bool measure_environment( float *temperature, float *humidity ); //mesure

// Volets
int Moteur_enA = 10;  // variable de type "int", nommée "enA" et attachée à la Broche 12, qui permet de gérer la vitesse du moteur.
int Moteur_in1 = 9;   // variable de type "int", nommée "in1" et attachée à la Broche 9, qui permet de gérer le sens de rotation.
int Moteur_in2 = 8;

// Lumiere
const int led1=7;
const int led2=16;

// Porte
const int capteur=18; // a changer

//module de vérification de la lampe
const int PHOTO_PIN=0; // broche analogique A0


//TEMPERATURE
//static const int DHT_SENSOR_PIN = 1;
#define DHT_SENSOR_PIN A1
const byte DHT_SUCCESS = 0; 
float temperature;

DHT dht(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

// Module MP3
#define RX 12
#define TX 11


// Commandes Player MP3
static int8_t Send_buf[8] = {0} ;//The MP3 player undestands orders in a 8 int string 
//0X7E FF 06 command 00 00 00 EF;(if command =01 next song order)  
#define NEXT_SONG 0X01  
#define PREV_SONG 0X02  
#define CMD_PLAY_W_INDEX 0X03 //DATA IS REQUIRED (number of song) 
#define VOLUME_UP_ONE 0X04 
#define VOLUME_DOWN_ONE 0X05 
#define CMD_SET_VOLUME 0X06//DATA IS REQUIRED (number of volume from 0 up to 30(0x1E)) 
#define SET_DAC 0X17 
#define CMD_PLAY_WITHVOLUME 0X22 //data is needed  0x7E 06 22 00 xx yy EF;(xx volume)(yy number of song) 
#define CMD_SEL_DEV 0X09 //SELECT STORAGE DEVICE, DATA IS REQUIRED 
#define DEV_TF 0X02 //HELLO,IM THE DATA REQUIRED 
#define SLEEP_MODE_START 0X0A 
#define SLEEP_MODE_WAKEUP 0X0B 
#define CMD_RESET 0X0C//CHIP RESET 
#define CMD_PLAY 0X0D //RESUME PLAYBACK 
#define CMD_PAUSE 0X0E //PLAYBACK IS PAUSED 
#define CMD_PLAY_WITHFOLDER 0X0F//DATA IS NEEDED, 0x7E 06 0F 00 01 02 EF;(play the song with the directory \01\002xxxxxx.mp3 
#define STOP_PLAY 0X16 
#define PLAY_FOLDER 0X17// data is needed 0x7E 06 17 00 01 XX EF;(play the 01 folder)(value xx we dont care) 
#define SET_CYCLEPLAY 0X19//data is needed 00 start; 01 close 
#define SET_DAC 0X17//data is needed 00 start DAC OUTPUT;01 DAC no output 

long IR;

int val_capteur=1;

const int nombrePas = 650;//512; //1/4 de tour
const int vitesse=35; 

int val_photoresistance; //on lira la valeur du branchement analogique

IRrecv irrecv(RECV_PIN);
decode_results results;

Stepper monMoteur(nombrePas, 2, 3, 4, 5);

SoftwareSerial mySerial(RX,TX);

DHT_nonblocking dht_sensor( DHT_SENSOR_PIN, DHT_SENSOR_TYPE );

void FermerPorte(){
  digitalWrite(capteur,LOW); //la porte est verrouillée
  monMoteur.setSpeed(vitesse);
  monMoteur.step(nombrePas); //rotation horaire
  }

void OuvrirPorte(){
  digitalWrite(capteur, HIGH);
  monMoteur.setSpeed(vitesse); //modifier pour changer la vitesse
  monMoteur.step(-nombrePas); //rotation anti-horaire
}

void AllumerLed(){
    digitalWrite(led1,LOW);
    digitalWrite(led2,LOW);
   
}

void EteindreLed(){
    digitalWrite(led1,HIGH);
    digitalWrite(led2,HIGH);
    
}

const byte COMMAND_BYTE_START = 0x7E;
const byte COMMAND_BYTE_VERSION = 0xFF;
const byte COMMAND_BYTE_STOP = 0xEF;


void sendMP3Cmd(int8_t command, int16_t dat) { 
  delay(20);
  Send_buf[0] = 0x7e; //starting byte 
  Send_buf[1] = 0xff; //version 
  Send_buf[2] = 0x06; //the number of bytes of the command without starting byte and ending byte 
  Send_buf[3] = command; // 
  Send_buf[4] = 0x00;//0x00 = no feedback, 0x01 = feedback 
  Send_buf[5] = (int8_t)(dat >> 8);//datah 
  Send_buf[6] = (int8_t)(dat); //datal 
  Send_buf[7] = 0xef; //ending byte 
  for(uint8_t i=0; i<8; i++){ 
    mySerial.write(Send_buf[i]) ; 
  } 
  delay(10);
} 

void playTrack(int16_t musicSoundID) {
  sendMP3Cmd(CMD_PLAY_WITHVOLUME, musicSoundID);
}

void setup() {
  Serial.begin(9600);
  irrecv.enableIRIn();
  irrecv.blink13(true);
  pinMode(Moteur_enA, OUTPUT);         // indique que la broche de la variable "enA" donc ici la PIN 10, est une sortie.
  pinMode(Moteur_in1, OUTPUT);         // indique que la broche de la variable "in1" donc ici la PIN 9, est une sortie.
  pinMode(Moteur_in2, OUTPUT);   

  pinMode(led1,OUTPUT);
  pinMode(led2,OUTPUT); 

  
  pinMode(capteur, INPUT); 

  mySerial.begin(9600);
  delay(500);//attendre l'initialisation MP3
  sendMP3Cmd(CMD_SEL_DEV, DEV_TF);//select the TF card   
  delay(200);
  sendMP3Cmd(CMD_PLAY_WITHVOLUME, 0x0F01);

  dht.begin();
}

void loop() {
    float temperature;
    
    if (irrecv.decode(&results)){      
        Serial.println(results.value);
        irrecv.resume();
        int i = 0;
        switch(results.value) {
          
          case SIG_UP :
            while(results.value == SIG_UP and results.value != SIG_STOP and i<300){ //changer valeur de i pour plus ou mois de temps
                Serial.println("UP DURING 5sec");
                i++;
                // Serial.println(i);
                digitalWrite(Moteur_in1,HIGH);   // envoie l'électricité dans la broche in1 (donc la PIN 9)ce qui fait donc tourner le moteur dans le sens de rotation de in1.
                digitalWrite(Moteur_in2,LOW);       // en mettant la broche in2 à l'état bas, la carte n'injecte pas d'électricité, le moteur peut donc continuer à tourner dans le sens de in1.
                analogWrite(Moteur_enA,200);
                irrecv.decode(&results);
                
                
                // défini la vitesse du moteur, ici 200 sur un maximum de 255 (échélle comprise entre 0 et 255, car cela correspond à 8 bits soit 1 octet).
                // en mettant la broche in2 à l'état bas, la carte n'injecte pas d'électricité, le moteur est donc à l'arrêt.
            }
            digitalWrite(Moteur_in1,LOW);       // en mettant la broche in1 à l'état bas, la carte n'injecte pas d'électricité, le moteur est donc à l'arrêt.
            digitalWrite(Moteur_in2,LOW);
          
          break;
          
          case SIG_STOP :        
            if (results.value  == SIG_STOP){
                Serial.println("STOP");
                digitalWrite(Moteur_in1,LOW);       // en mettant la broche in1 à l'état bas, la carte n'injecte pas d'électricité, le moteur est donc à l'arrêt.
                digitalWrite(Moteur_in2,LOW);
            }
          break;

          case SIG_DOWN :
            while(results.value == SIG_DOWN and results.value != SIG_STOP and i<300){
                Serial.println("DOWN DURING 5sec");  
                i++;
                // Serial.println(i);   
                digitalWrite(Moteur_in1,LOW);       // met la broche in1 à l'état bas, la carte n'injecte pas d'électricité, le moteur ne tourne pas dans le sens de in1.
                digitalWrite(Moteur_in2,HIGH);       // en mettant la broche in2 à l'état bas, la carte n'injecte pas d'électricité, le moteur peut donc continuer à tourner dans le sens de in1.
                analogWrite(Moteur_enA,200);
                irrecv.decode(&results);
                // Serial.println(results.value);
            }
            digitalWrite(Moteur_in1,LOW);       // en mettant la broche in1 à l'état bas, la carte n'injecte pas d'électricité, le moteur est donc à l'arrêt.
            digitalWrite(Moteur_in2,LOW);
          break;
          
          case SIG_LIGHTON :  // LUMIERE
              Serial.println("allumer led");
              AllumerLed();
          break;

          case SIG_LIGHTOFF :
              Serial.println("éteindre led");
              EteindreLed();
          break;

          case SIG_LIGHTVRF :
            
              val_photoresistance=analogRead(PHOTO_PIN);
              Serial.print("Analog reading = ");
              Serial.print(val_photoresistance); 
              
              if (val_photoresistance<500){
                
                sendMP3Cmd(CMD_PLAY_WITHFOLDER, 0X0F00702);
                Serial.println("la lumière est éteinte");
                delay(1000);
                }
              if (val_photoresistance>=500){
                ;
                sendMP3Cmd(CMD_PLAY_WITHFOLDER, 0X0F00701);
                Serial.println("la lumière est allumée");
                delay(1000);
              }            
            
          break;
          
          case SIG_DOOROPN :
            val_capteur=digitalRead(capteur); //lecture état du capteur
            if (val_capteur==LOW){
                  OuvrirPorte();
                  Serial.println( "ouverture porte");
                }

          break;
          case SIG_DOORCLS :
             val_capteur=digitalRead(capteur); //lecture état du capteur
             if (val_capteur==HIGH){
                    FermerPorte();
                    Serial.println( "fermeture porte");
                  }
          break;
          case SIG_DOORVRF :
              val_capteur=digitalRead(capteur); //lecture de l'état du capteur
              if (val_capteur==LOW){           //si notre interrupteur est enfoncé
                  sendMP3Cmd(CMD_PLAY_WITHFOLDER, 0X0F00704);;
                }
              if (val_capteur==HIGH){
                  sendMP3Cmd(CMD_PLAY_WITHFOLDER, 0X0F00703);;
                }
          break;
          case SIG_TEMP :
              temperature = dht.readTemperature();

              if (isnan(temperature)){ //on vérifie qu'on obtient un nombre
                  Serial.println("ERROR");
                  return;
              }
                  
              Serial.print("La température est: ");
              Serial.print(temperature);
              Serial.println("°C");

             
             
              if (temperature==20) {
              sendMP3Cmd(CMD_PLAY_WITHFOLDER, 0X0F00705);;
              }
              if (temperature==21) {
              sendMP3Cmd(CMD_PLAY_WITHFOLDER, 0X0F00706);;
              }
              if (temperature==22) {
              sendMP3Cmd(CMD_PLAY_WITHFOLDER, 0X0F00707);;
              
              }
              if (temperature==23) {
              sendMP3Cmd(CMD_PLAY_WITHFOLDER, 0X0F00708);;
              
              }
              if (temperature==24) {
              sendMP3Cmd(CMD_PLAY_WITHFOLDER, 0X0F00709);
              }
              

              break;
        }
   }  // CASE
   }   // IF IR
