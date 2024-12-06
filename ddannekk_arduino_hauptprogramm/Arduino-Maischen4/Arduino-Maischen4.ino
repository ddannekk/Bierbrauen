// Einbinden der nötigen Bibliotheken
#include <LiquidCrystal_I2C.h>
#include <PID_v1.h>
#include <RCSwitch.h>
#include <OneWire.h>
#include <DallasTemperature.h>



/////////////////////////////////////////////////////////////////
// Variablendeklarationen für das Thermometer.
#define ONE_WIRE_BUS 13

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
/////////////////////////////////////////////////////////////////



/////////////////////////////////////////////////////////////////
// Deklaration der Datenstruktur des LCD-Displays für eine 20x4 Display
// LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// Deklaration der Datenstruktur des LCD-Displays für eine 16x2 Display
// Bitte als Kommentar setzen und die Deklaration für ein 20x4 Display benutzen,
// falls Du ein solches in Deinem Kasten hast!!!
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

/////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////////////////
// Deklaration der Variablen und Konstanten des Senders 
RCSwitch sender = RCSwitch();
// OFF als Konstante wird mit dem Signal gleichgesetzt, 
// mit dem die Funksteckdose ausgeschaltet wird. 
// Der angegebene Wert wurde mit der Funkfernsteuerung und dem entsprechenden 
// Arduino-Programm zum Empfang dieses Werts für die benutzte Funksteckdose festgestellt.
#define OFF 1941622
// ON ist die entsprechende Konstante für das Anschalten der Funksteckdose. 
#define ON 1941623
/////////////////////////////////////////////////////////////////////////////////////////





/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Deklaration der Datenstrukturen für den PID-Algorithmus

// SollTemperatur ist die Solltemperatur im Maischkessel.
double SollTemperatur;
// AktuelleTemperatur die die Varible für die aktuell gemessene Temperatur.
double AktuelleTemperatur;
// PIDErgebnis ist der Wert, den der PID-Algorithmus ausgibt.
double PIDErgebnis;

// Deklaration der Datenstruktur einer schnellen Steuerung der Temperaturzunahme durch den PID-Algorithmus
PID myPIDSCHNELL(&AktuelleTemperatur, &PIDErgebnis, &SollTemperatur, 1000, 1, 0, DIRECT);
// Deklaration der Datenstruktur einer langsamen Steuerung der Temperaturzunahme durch den PID-Algorithmus
PID myPIDLANGSAM(&AktuelleTemperatur, &PIDErgebnis, &SollTemperatur, 400, 1, 0, DIRECT);

// Festlegung des Zeitfensters auf 5000ms = 5s, in dem das Verhältnis von An- und Abstellen der Funksteckdose und damit der Heizplatte
// durch den PID-Algorithmus gesteuert wird.
int Fenstergroesse = 5000; 

// AktFenstergroesse beschreibt, wieviel Zeit des Zeitfensters gerade abgelaufen ist (diese Zeit wird mit PIDErgebnis verglichen)
unsigned long AktFenstergroesse; 

// RastStart ist eine Variable, in der Beginn der jeweiligen Rast gespeichert wird, damit
// damit das erste Zeitfenster genau berechnet werden kann. 
unsigned long RastStart; 
float AktuelleRastZeit;

// FensterStartZeit speichert die Zeit des Starts des gerade aktuellen Zeitfensters (von 5000 ms) AktFenstergroesse.
// Diese Variable muss immer wieder neu gesetzt werden (weil am Anfang des Programmlaufs eine fortlaufende Zeitmessung initiiert wird,
// auf der der Start des aktuellen Zeitfensters von 5 Sekunden immer wieder neu festgelegt werden muss.
unsigned long FensterStartZeit;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Deklaration der Variablen für die Steuerung der Rasten
// Rast ist 0 für die Eiweiß-, 1 für die Zucker-, und 2 für die Stärkerast
int Rast;

float  SollTemperaturEiweissRast = 52;
float  SollTemperaturStaerkeRast = 62;
float  SollTemperaturZuckerRast  = 72;

// SollRastZeit ist die Variable, in die die jeweilige Rastzeit der einzelnen Rasten eingespeichert wird. Diese Rastzeiten
// werden hier nachfolgend für das Kepler-Bräu festgelegt.
int    SollRastZeit;
int    SollZeitEiweissRast = 900;
int    SollZeitStaerkeRast = 1200;
int    SollZeitZuckerRast  = 1200;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void SCHNELLESPIDVerfahren()
{
      // Berechnung der Steuerungswerte des PID-Algorithmus  
      myPIDSCHNELL.Compute();

      // Jetzt wird geprüft, ob wieder ein neues Zeitfenster gesetzt werden muss.
      // Wenn das der Fall ist, wird die FensterStartZeit um 5 s (Fenstergroesse) hochgesetzt.
      AktFenstergroesse = millis() - FensterStartZeit;
      if (AktFenstergroesse > Fenstergroesse)
        FensterStartZeit += Fenstergroesse;

      AktFenstergroesse = millis() - FensterStartZeit;
      // Wenn der Wert von AktFenstergroesse schon größer ist als PIDErgebnis,
      // wird die Funksteckdose ausgestellt. D.h., die Kochplatte heizt nicht!
      if(AktFenstergroesse > PIDErgebnis)
        sender.send(OFF,24);

      // Im anderen Fall, falls also die die abgelaufene Zeit des aktuellen Zeifensters noch kleiner als 
      // PIDErgebnis ist, dann wird die Funksteckdose angestellt, die Kochplatte heizt!       
      else 
       sender.send(ON,24);
}

void LANGSAMESPIDVerfahren()
 {
  myPIDLANGSAM.Compute();
 
  // Jetzt wird geprüft, ob wieder ein neues Zeitfenster gesetzt werden muss.
  // Wenn das der Fall ist, wird die FensterStartZeit um 5 s (WindowSize) hochgesetzt.
  AktFenstergroesse = millis() - FensterStartZeit;
  if (AktFenstergroesse > Fenstergroesse)
    {
     FensterStartZeit += Fenstergroesse;
    }
  AktFenstergroesse = millis() - FensterStartZeit;
 
  // Wenn der Wert von PIDErgebnis schon größer ist als die aktuell abgelaufene Zeit des Zeitfensters,
  // wird die Funksteckdose ausgestellt. D.h., die Kochplatte heizt nicht!
  if(AktFenstergroesse < PIDErgebnis)
    { 
     sender.send(OFF,24);
    }
  // Im anderen Fall, falls also die die abgelaufene Zeit des aktuellen Zeifensters noch kleiner als 
  // PIDErgebnis ist, dann wird die Funksteckdose angestellt, die Kochplatte heizt!       
  else 
    sender.send(ON,24);
 }

void AufheizRampe()

       
void RastTemperaturregelung()




void TestMaischEnde()
{
    // Falls die letzte Rast gerade abgearbeitet wurde): Endloslosschleife mit Anzeige 
    // des Programmende auf dem LCD-Display.
    if (Rast == 2)
      {
       // Endloschleife mit Schlussanzeige auf dem LCD-Display 
       while (1 == 1)
        {
         lcd.clear();
         lcd.setCursor(0,0); lcd.print("Programmende!");
         lcd.setCursor(0,1); lcd.print("Alle Rasten durchlaufen.");
         lcd.setCursor(0,2); lcd.print("Bitte alles abschalten!");
         delay(1000);
         Serial.println("Maischende erreicht: Bitte alles abschalten");
        }
      } 
}

// Methode Temperaturmessung 
double Temperaturmessung()
 {
   sensors.requestTemperatures();
   AktuelleTemperatur = sensors.getTempCByIndex(0);
   return AktuelleTemperatur;
 }

// Methode LCDAnzeige()
void LCDAnzeige()
 {     
  //Befehle zur Anzeige der aktuellen Werte auf dem LCD-Display
  //Für die Ausgabe reicht das kleine 16x2 Display !
  lcd.clear();
  lcd.setCursor(0,0); lcd.print("t:");
  lcd.setCursor(2,0);lcd.print(AktuelleTemperatur);
  lcd.setCursor(8,0); lcd.print("St:");
  lcd.setCursor(11,0);lcd.print(SollTemperatur);

  if (Rast == 0)
   {   
    lcd.setCursor(0,1);lcd.print("Eiweissr.");
   }
  if (Rast == 1)
   {
    lcd.setCursor(0,1);lcd.print("Staerker.");
   }
  if (Rast == 2)
   {
    lcd.setCursor(0,1);lcd.print("Zuckerr.");      
   } 
  lcd.setCursor(11,1);lcd.print(AktuelleRastZeit); 
  } 
 
float Zeitmessung()
 {
  unsigned long MilliSekunden;
  
  MilliSekunden = millis() - RastStart;
  AktuelleRastZeit = MilliSekunden / 1000.0;
  return AktuelleRastZeit;
 }


 void RastAnfangsZeitEinstellung()
 {
  FensterStartZeit = millis();
  RastStart = millis();
 }

void setup()
{

  /////////////////////////////////////////////////////////////////////////////////////////  
  //Initialisierung der beiden PID-Algorithmen
  // Der Start des aktuellen Zeitfensters für den PID-Algorithmus wird auf die gerade gemessene Zeit (0s) gestellt). 
  FensterStartZeit = millis();
  // Dem PID-Algorithmus (schnell und langsam) wird die Größe des Zeitfensters    
  // mitgeteilt. Das ist wichtig, weil damit der Algorithmus als Ergebnis einen Bruchteil des Zeitfenster von 5s 
  // zurückgibt, der direkt zur Steuerung der 
  // Heizplatte verwendet werden kann.
  myPIDSCHNELL.SetOutputLimits(0, Fenstergroesse);
  myPIDLANGSAM.SetOutputLimits(0, Fenstergroesse);
  
  myPIDSCHNELL.SetMode(AUTOMATIC);
  myPIDLANGSAM.SetMode(AUTOMATIC);
  /////////////////////////////////////////////////////////////////////////////////////////
  


  /////////////////////////////////////////////////////////////////////////////////////////
  //Initialisierung des 433 MHz-Senders
  // DatenPin des 433 MHz-Senders an Pin 2
  sender.enableTransmit(2);  
  // Initialisieren des 433 MHz-Senders
  sender.setProtocol(1);
  sender.setPulseLength(305);

  //Initialisierung des DS18b20
  sensors.begin();
  sensors.requestTemperatures();
  /////////////////////////////////////////////////////////////////////////////////////////


  /////////////////////////////////////////////////////////////////////////////////////////
  // Anzeige des Maischstarts auf dem LCD-Display
  lcd.backlight();
  lcd.begin(16,2);
  lcd.setCursor(0,1);
  lcd.print(("   Start!"));
  // Überprüfung auf zu hohe Anfangstemperatur der Maische
  sensors.requestTemperatures();
  AktuelleTemperatur = sensors.getTempCByIndex(0);
  if (AktuelleTemperatur > SollTemperaturEiweissRast)
   {
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Temperatur zu hoch!");
    lcd.setCursor(0,1); lcd.print("Frische Maische !!!"); 
    lcd.setCursor(0,2); lcd.print("Programmabbruch !!!"); 
    while (1==1)
     {}; 
    }
  /////////////////////////////////////////////////////////////////////////////////////////

    
}

 

void loop()
{
 for (Rast=0;Rast<3;Rast=Rast+1)
  {
   AufheizRampe();
   RastTemperaturregelung();
   TestMaischEnde();
  }
}
