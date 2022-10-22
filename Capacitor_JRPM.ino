/*
                           ***  Medidor de capacidad entre 0,2pF y 100uF con ARDUINO ***
                      Utilizando las librerías Capacitor.h y CapacitorLite.h de Jonathan Nethercott
                      CapacitorLite.h >>> Cuando el condensador bajo prueba mide < 655pF (0,2pF ... 655pF)
                       Capacitor.h >>> Cuando el condensador bajo prueba mide > 655pF  (656pF ... 100uF)
                      
  La librería "CapacitorLite.h" utiliza menos almacenamiento de programa y es más rápida que la versión "Capacitor.h"
                        "CapacitorLite.h" solo puede medir entre 0,2pF y 655pF
      Measure() entrega valores x100, así habrá que dividir el resultado entre 100, para prersentar pF con 2 decimales 
                  https://wordpress.codewrite.co.uk/pic/2014/01/21/cap-meter-with-arduino-uno/

                  "Capacitor.h" permite medir capacidades comprendidas entre 1pF y 100uF
                  https://wordpress.codewrite.co.uk/pic/2014/01/25/capacitance-meter-mk-ii/
    
_____________________________________________________________________________________________________
                                       Escrito por: J_RPM
                                        http://j-rpm.com
                                     Octubre de 2022 (v1.1)
________________________________________________________________________________________________________
*/
#include <CapacitorLite.h>
#include <Capacitor.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Definición de los dos pines de conexión (D12, A2), el mismo para las dos librerías
// Si el condensador bajo prueba es electrolítico el Pin + debe conectarse a D12
CapacitorLite capL(12,A2);
Capacitor capH(12,A2);

#define  muestras 16       // Número de muestras consecutivas, para promediar la medida
float medida;
String unidad;
String Version = "(v1.1)";

void setup() {
  Serial.begin(9600);
  // Estructura de las variables de calibrado y sus valores por defecto
  // ... para ATMEGA328P con la librería: Capacitor.h 
  //void Capacitor::Calibrate(float strayCap, float pullupRes)
  //#define STRAY_CAP (26.30);
  //#define R_PULLUP (34.80);
  capH.Calibrate(44.80,36.80);  // J_RPM: 44.80,36.8 >>> Se puede comentar esta línea, si los valores C,R son 26.30 y 34.80
  
  // Estructura de las variables de calibrado y sus valores por defecto
  // ... para ATMEGA328P con la librería: CapacitorLite.h 
  //void CapacitorLite::Calibrate(unsigned int strayCap)
  capL.Calibrate(4480);     // J_RPM: 4480 >>> Se puede comentar esta línea, si el valor C es 2630

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
}

void loop() {
// Habilitar para mostrar los valores ADC por el puerto serie
//capH.ShowDebug(true);
//capL.ShowDebug(true);
 
  // Lee la capacidad en pF X 100 (0,2pF...655pF)
  medida = capL.Measure();
  
  //Comprueba si es una medida válida, inferior a 655pF
  if (medida < 65500 ) {
      mediaL();           // Promedia la medida: CapacitorLite.h
      unidad =" pF.";
      medida = medida/100;
      lcd.setCursor(0, 0);
      lcd.print(F("CapacitorLite.h "));
      Serial.println(F("### CapacitorLite.h ###"));  
      Serial.println(medida);  
  }else {
    // Lee la capacidad en pF (656pF...100uF)
    mediaH();           // Promedia la medida: Capacitor.h
    lcd.setCursor(0, 0);
    lcd.print(F(">>> Capacitor.h "));
    Serial.println(F("### Capacitor.h ###"));  
    if (medida >= 101000000) {
       medida = -1;
    }else if (medida >= 1000000) {
       medida = medida/1000000;
       unidad =" uF.";
    }else if (medida >= 1000) {
       medida = medida/1000;
       unidad =" nF.";
    }else {
      unidad =" pF.";
    }
  }
  // Presenta los resultados de la medida y muestra la actividad en el display
  lcd.setCursor(0, 1);
  lcd.print("*");
  lcd.setCursor(5, 1);
  if (medida >= 0) {
    Serial.print(medida);  
    Serial.println(unidad);  
    lcd.print(medida);
    lcd.print(unidad);
  }else {
    Serial.println(F(">100 uF."));  
    lcd.print(F(">100 uF."));
  }
  lcd.print(F("   "));
  
  //Mantiene la marca de actividad en el LCD durante 300ms
  delay(300);
  lcd.setCursor(0, 1);
  lcd.print(F(" "));

  // Retardo entre dos medidas consecutivas (refresco de los valores en el display)
  delay(300);
}
///////////////////////////////////////////////////////
void mediaL(){
  float valorMedio = 0;                       // Reinicia valor medio
  for (byte y = 0; y < muestras; y++){        // Muestreos 
    medida = capL.Measure();
    valorMedio = valorMedio + medida;
    delay(20);
  }
  medida = valorMedio / muestras;             // Guarda el valor promedio de las muestras
  
  // Corrige las medidas: -25pF 
  if (medida > 2500) {
    medida = medida-2500;
  }else {
    medida = 0;
  }
}
///////////////////////////////////////////////////////
void mediaH(){
  float valorMedio = 0;                       // Reinicia valor medio
  for (byte y = 0; y < muestras; y++){        // Muestreos 
    medida = capH.Measure();
    valorMedio = valorMedio + medida;
    delay(20);
  }
  medida = valorMedio / muestras;             // Guarda el valor promedio de las muestras
}
///////////////////////////////////////////////////////

// Fin del programa //
