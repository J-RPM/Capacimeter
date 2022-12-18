/*
                    ***  Medidor de capacidad y ESR con 2 entradas (ATMega328P) ***
                                   1pF -> 1uF  / 50nF -> 10mF 
                               >>>  Mide ESR a partir de 10uF  <<<

           Se utiliza la librería Capacitor.h de Jonathan Nethercott, en la entrada de capacidades BAJAS 
                   "Capacitor.h" habilitada para medir capacidades BAJAS, entre 1pF y 1uF
                  https://wordpress.codewrite.co.uk/pic/2014/01/25/capacitance-meter-mk-ii/

        IMPORTANTE: Arrancar con las puntas abiertas, porque se calibran el '0' de las 2 entradas
                                   *** Capacitor_JRPM.ino ***
_____________________________________________________________________________________________________
                                       Escrito por: J_RPM
                                        http://j-rpm.com
                                     Diciembre de 2022 (v1.2)
________________________________________________________________________________________________________
*/
#include <Capacitor.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

String Version = "(v1.2)";
#define resistencia_H  10035.00F    //R alta: 10K para cargar/descargar el condensador
#define resistencia_L  275.00F      //R baja: 240 para cargar/descargar el condensador

#define CapIN_H       A1     // Medida de capacidades ALTAS + ESR (>50nF)
#define CapOUT        A2     // Punto comúm de medida
#define CapIN_L       A3     // Medida capacidad BAJAS (<1uF)
#define cargaPin      12     // Carga lenta (resistencia_H)
#define descargaPin   13     // Carga & Descarga & Impulso ESR (resistencia_L)

// Definición de los dos pines de conexión (A3, A2), para capacidades BAJAS
Capacitor pFcap(A3,A2);

// Medidas Offset para el calibrado 
float Off_pF_Hr = 0;                   
float Off_pF_H = 0;                   
float Off_pF_Low = 0;                   
float Off_GND = 0; 

unsigned long iniTime;
unsigned long endTime;
float medida; 
float valor;               
String unidad;
String tipo;

// Capacidades BAJAS < 1uF
int Repe = 30;        

// ESR
const int MAX_ADC_VALUE = 1023;
unsigned int sampleESR;
unsigned int milliVolts;
unsigned int ADCref = 0;
float esr;

////////////////////////////////////////////////////////////////////////////////////
void setup() {
  // Estructura de las variables de calibrado y sus valores por defecto
  // ... para ATMEGA328P con la librería: Capacitor.h 
  //void Capacitor::Calibrate(float strayCap, float pullupRes)
  //#define STRAY_CAP (26.30);
  //#define R_PULLUP (34.80);
  pFcap.Calibrate(41.95,36.00);  // J_RPM: 41.95,36.00 >>> Se puede comentar esta línea, si los valores C,R son 26.30 y 34.80
  
  pinMode(CapOUT, OUTPUT);
  pinMode(CapIN_L, OUTPUT);
  Serial.begin(9600);

  // Inicializa el LCD y pone el mensaje de inicio
  Serial.print(F("### Capacitor J_RPM "));  
  Serial.print(Version);  
  Serial.println(F(" ###"));  
  
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Capacitor J_RPM "));
  lcd.setCursor(5, 1);
  lcd.print(Version);
  delay(2000);

  lcd.setCursor(1, 1);
  lcd.print(F("...calibrando   "));
  calibrado();
  delay(1000);

}

 /////////////////////////////// CAPACIDAD //////////////////////////////////  
void loop() {
  // Se hace un test del condensador
  descargaCap();
  pinMode(descargaPin, OUTPUT); 
  digitalWrite(descargaPin, HIGH);  
  delay(1);
  unsigned int muestra1 = analogRead(CapIN_H);
  delay(100);
  unsigned int muestra2 = analogRead(CapIN_H);
  unsigned int cambio = muestra2 - muestra1;
  // TEST ??
  if (muestra2 < 1000 && cambio < 30 ) {
    tipo = "[Test]";
    Serial.print(F("Muestra 1: ")); 
    Serial.println(muestra1); 
    Serial.print(F("Muestra 2: ")); 
    Serial.println(muestra2); 
 
   // <<< Test OK
  }else {
    // Se descarga/carga el condensador bajo prueba, con la resistencia de 240 Ohmios
    descargaCap();
    cargaCap_Fast();
    medida = ((float)endTime / resistencia_L) - (Off_pF_Hr / 1000000);
    
    // Si la capacidad es inferior a 80 uF, se repite otra vez la medida cargando con la resistencia de 10K
    if (medida < 80){
      tipo = " <80uF";
      descargaCap();
      cargaCap_Slow();
      medida = ((float)endTime / resistencia_H) - (Off_pF_H / 1000000);
    }else {
      tipo = " >80uF";
    }
  
    if (medida > 1){
      valor = medida;
      unidad = " uF";
    }else if (medida > 0.05){ 
      tipo = " >50nF";
      valor = medida * 1000; 
      unidad = " nF";
    }else {
      tipo = "  <1uF";
      midePF();
      valor = valor - Off_pF_Low;
     
      // No muestra valores inferiores a 1 pF 
      if (valor < 1) { 
        valor = 0;
        unidad = " pF";
      }else if (valor > 1000000) {
        valor = -999;
        unidad = "> 1uF";
      }else if (valor > 1000) {
        valor = valor / 1000;
        unidad = " nF";
      }else{
        // Ajuste fino (pF)
        if (valor < 35){
          valor = valor - 9;
        }else if (valor < 50){
          valor = valor - 13;
        }else if (valor < 65){
          valor = valor - 17;
        }else {
          valor = valor - 22;
        }
        unidad = " pF";
      }
    }
   }
  // <<< Fin del Test y Medida >>> PRESENTA DATOS

  // Mide ESR a partir de 10uF  
  lcd.setCursor(0, 0);
  if (medida > 10 || tipo == "[Test]"){
    mideESR();
     if (tipo == "[Test]"){
        Serial.print(F("OHM: "));
        lcd.print(F("OHM: "));
     }else {
        Serial.print(F("ESR: "));
        lcd.print(F("ESR: "));
     }

    if(esr < 300){
      Serial.print(esr,1);
      lcd.print(esr,1); 
      lcd.print(F("  ")); 
    }else {
      Serial.println(F(">300"));
      lcd.print(F(">300")); 
    }
    Serial.println(F(" Ohm"));
  } else {
    lcd.print(F("Capacidad "));
  }
  lcd.setCursor(10, 0);
  lcd.print(tipo);

  
  // Se comprueba que haya pasado el TEST
  if (tipo == "[Test]"){
    lcd.setCursor(0, 1);
    lcd.print(F("#ERROR capacidad"));
    Serial.println(F("#ERROR capacidad")); 
  }else {
    // Test OK >>> se muestra la medida en el display
    if (valor < 0 && valor != -999) {valor = 0;}
    
    // Presenta los resultados de la medida y muestra la actividad en el display
    Serial.print(F("Capacidad"));
    Serial.print(tipo);
    Serial.print(F(" = "));  

    lcd.setCursor(0, 1);
    lcd.print("     ");
    if (valor != -999) {
      if (valor < 1000 &&  unidad != " pF" ) {
        lcd.print(valor,2);
        Serial.print(valor,2);  
      }else {
        lcd.print(" ");
        lcd.print(valor,0);
        Serial.print(valor,0);  
      }
    }
    lcd.print(unidad);
    lcd.print("      ");
    Serial.println(unidad);  
  }
    
  //Mantiene la marca de actividad en el LCD durante 300ms
  delay(300);
  lcd.setCursor(0, 1);
  lcd.print(F("?"));
  delay(300);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
void calibrado() {
  descargaCap();
  cargaCap_Slow();
  Off_pF_H = ((float)endTime / resistencia_H)*1000000;  
  Serial.print(F("Offset <80uF (pF): ")); 
  Serial.println(Off_pF_H);

  descargaCap();
  cargaCap_Fast();
  Off_pF_Hr = ((float)endTime / resistencia_L)*1000000;  
  Serial.print(F("Offset >80uF (pF): ")); 
  Serial.println(Off_pF_Hr);

  midePF();
  Off_pF_Low = valor; 

  Serial.print(F("Offset <1uF (pF): ")); 
  Serial.println(Off_pF_Low);

  medidaADC();
}
//////////////////////////////////////////////////////////////////////////////////////////////////
void descargaCap() {
  analogReference(DEFAULT);           
  pinMode(CapIN_H,INPUT);
 
  pinMode(cargaPin,OUTPUT);              //Pin de carga SALIDA
  digitalWrite(cargaPin, LOW);           //Descarga el condensador 
  pinMode(descargaPin, OUTPUT);          //Pin de descarga SALIDA  
  digitalWrite(descargaPin, LOW);        //Descarga el condensador     
  while(analogRead(CapIN_H) > 0){}       //Espera a que se descargue del todo   
  pinMode(descargaPin, INPUT);           //Pin de descarga en alta impedancia      
  pinMode(cargaPin,INPUT);               //Pin de carga en alta impedancia
}
//////////////////////////////////////////////////////////////////////////////////////////////////
void cargaCap_Slow() {
  pinMode(cargaPin, OUTPUT); 
  digitalWrite(cargaPin, HIGH);  
  iniTime = micros();
  
  //Espera hasta el 63% de 1024
  while(analogRead(CapIN_H) < 645){}    
  
  endTime = micros() - iniTime;
}
//////////////////////////////////////////////////////////////////////////////////////////////////
void cargaCap_Fast() {
  pinMode(descargaPin, OUTPUT); 
  digitalWrite(descargaPin, HIGH);  
  iniTime = micros();
  
  //Espera hasta el 63% de 1024
  while(analogRead(CapIN_H) < 645){}    
  
  endTime = micros() - iniTime;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
// Utilizamos el ADC para medir la referencia de 1.1V, incorporada en la mayoría de modelos de Arduino
// Luego se calcula la tensión a la que realmente está alimentado Arduino.
// 1.1 x 1023 x 1000 = 1125300
///////////////////////////////////////////////////////////////////////////////////////////////////////
int refADC(){
   long result;
   // Lee 1.1V de referencia interna 
   ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
   delay(2); 
   ADCSRA |= _BV(ADSC);
   while (bit_is_set(ADCSRA,ADSC));
   result = ADCL;
   result |= ADCH<<8;
   result = 1125300L / result; // Calcula la tensión AVcc en mV
   return result;
}
/////////////////////////////////////////////////////////////////////////
void medidaADC() {
  ADCref = refADC();
  Serial.print(F("ADCref (mV): "));
  Serial.println(ADCref);
}
/////////////////////////////////////////////////////////////////////////
void mideESR() {
  descargaCap();
  digitalWrite(CapOUT, LOW);
  pinMode(descargaPin, OUTPUT); 
  
  digitalWrite(descargaPin, LOW);
  delayMicroseconds(100);
  digitalWrite(descargaPin, HIGH);
  delayMicroseconds(5);
  sampleESR =  analogRead(CapIN_H); 

  // Mide el Offset del punto de referencia de la medida con respecto a GND 
  Off_GND =  analogRead(CapOUT); 
  Serial.print(F("Off_GND (Ohm): "));
  Serial.println(Off_GND);

  // Corrige el Offset (resistencia a masa de A2)
  sampleESR = sampleESR - Off_GND;

  Serial.print(F("LEE: "));
  Serial.println(sampleESR);
  descargaCap();

  milliVolts = (sampleESR * (float)ADCref) / MAX_ADC_VALUE;
  Serial.print(F("mV: "));
  Serial.println(milliVolts);

  // Calcula la resistencia de A2 a GND y la suma a la resistencia de carga
  int R_GND = resistencia_L / MAX_ADC_VALUE * Off_GND;
  Serial.print(F("R_GND (Ohm): "));
  Serial.println(R_GND);
  
  esr = (resistencia_L + R_GND) / (((float)ADCref / milliVolts) - 1); 
  
  // Calibrado ESR (Ohmios)
  esr = esr - 0.9;
  if (esr < 0) {esr = 0;}
}
///////////////////////////////////////////////////////
void midePF(){
  descargaCap();
  float valorMedio = 0;                   // Reinicia valor medio
  for(int i = 0; i < Repe; i++){
    valor = pFcap.Measure();
    valorMedio = valorMedio + valor;
    descargaCap();
  }
  valor = valorMedio / Repe;              // Guarda el valor promedio de las muestras
}
/////////////////////////////////////////////////////////////////////////
// FIN
