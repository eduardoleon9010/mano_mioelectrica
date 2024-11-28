#include <Servo.h> // Incluye la librería Servo para controlar los servos

// Configuración de parámetros
const boolean isRight = 1; // Define si el dispositivo es para la mano derecha (1) o izquierda (0)
const int outThumbMax = 140; // Ángulo máximo para el pulgar, abierto para la mano derecha, cerrado para la izquierda
const int outIndexMax = 130; // Ángulo máximo para el índice, abierto para la mano derecha, cerrado para la izquierda
const int outOtherMax = 145; // Ángulo máximo para otros tres dedos, abierto para la mano derecha, cerrado para la izquierda
const int outThumbMin = 70;  // Ángulo mínimo para el pulgar, cerrado para la mano derecha, abierto para la izquierda
const int outIndexMin = 27;  // Ángulo mínimo para el índice, cerrado para la mano derecha, abierto para la izquierda
const int outOtherMin = 105; // Ángulo mínimo para otros dedos, cerrado para la mano derecha, abierto para la izquierda
const int speedMax = 10;     // Velocidad máxima
const int speedMin = 0;      // Velocidad mínima
const int speedReverse = -3; // Velocidad al invertir el movimiento
const int thSpeedReverse = 15; // Umbral para invertir velocidad (0-100)
const int thSpeedZero = 30;    // Umbral para velocidad cero (0-100)
const boolean onSerial = 1;   // Activar (1) o desactivar (0) la salida en el Monitor Serial

// Configuración de pines
int pinCalib; // Pin para iniciar calibración
int pinExtra;
int pinThumb; // Pin para abrir/cerrar el pulgar
int pinOther; // Pin para bloquear/desbloquear otros tres dedos
int pinSensor; // Entrada del sensor
int pinServoIndex; // Pin para el servo del índice
int pinServoOther; // Pin para el servo de otros dedos
int pinServoThumb; // Pin para el servo del pulgar

// Hardware
Servo servoIndex; // Servo del dedo índice
Servo servoOther; // Servo de otros tres dedos
Servo servoThumb; // Servo del pulgar

// Variables de software
boolean isThumbOpen = 1; // Estado del pulgar (abierto: 1, cerrado: 0)
boolean isOtherLock = 0; // Estado de bloqueo para otros dedos
boolean isReversed = 0;  // Estado de inversión de movimiento
int swCount0, swCount1, swCount2, swCount3 = 0; // Contadores para detección de pulsos en botones
int sensorValue = 0; // Valor leído desde el sensor EMG
int sensorMax = 400; // Valor máximo de sensor para la calibración
int sensorMin = 100; // Valor mínimo de sensor para la calibración
int speed = 0;       // Velocidad de movimiento
int position = 0;    // Posición actual (0-100)
const int positionMax = 100; // Límite superior de la posición
const int positionMin = 0;   // Límite inferior de la posición
int prePosition = 0;  // Almacena la posición previa
int outThumb, outIndex, outOther = 90; // Posiciones de los servos
int outThumbOpen, outThumbClose, outIndexOpen, outIndexClose, outOtherOpen, outOtherClose; // Rango de posiciones para cada servo

void setup() {
  Serial.begin(9600); // Inicia la comunicación serial
  if(isRight){ // Configura los pines para la mano derecha
    pinCalib =  A6;
    pinExtra =  A5; 
    pinThumb =  A4;
    pinOther =  A3;
    pinSensor = A0;
    pinServoIndex = 3;
    pinServoOther = 5;
    pinServoThumb = 6;
    outThumbOpen=outThumbMax; outThumbClose=outThumbMin; // Define posiciones abiertas/cerradas para el pulgar
    outIndexOpen=outIndexMax; outIndexClose=outIndexMin; // Define posiciones abiertas/cerradas para el índice
    outOtherOpen=outOtherMax; outOtherClose=outOtherMin; // Define posiciones abiertas/cerradas para otros dedos
  }
  else{ // Configura los pines para la mano izquierda
    pinCalib =  11;
    pinExtra =  10; 
    pinThumb =  8;
    pinOther =  7;
    pinSensor = A0;
    pinServoIndex = 3;
    pinServoOther = 5;
    pinServoThumb = 6;
    outThumbOpen=outThumbMin; outThumbClose=outThumbMax;
    outIndexOpen=outIndexMin; outIndexClose=outIndexMax;
    outOtherOpen=outOtherMin; outOtherClose=outOtherMax;
  }

  servoIndex.attach(pinServoIndex); // Asocia el servo del índice al pin configurado
  servoOther.attach(pinServoOther); // Asocia el servo de otros dedos
  servoThumb.attach(pinServoThumb); // Asocia el servo del pulgar

  // Configura los pines como entradas con resistencia pull-up
  pinMode(pinCalib, INPUT_PULLUP);
  pinMode(pinExtra, INPUT_PULLUP);
  pinMode(pinThumb, INPUT_PULLUP);
  pinMode(pinOther, INPUT_PULLUP);
}

void loop() {
  //==Esperando calibración==
  if(onSerial) Serial.println("======Esperando Calibración======");
  while (1) { // Ciclo de espera de calibración
    servoIndex.write(outIndexOpen); // Posiciona los servos en posición abierta
    servoOther.write(outOtherOpen);
    servoThumb.write(outThumbOpen);
    if(onSerial) serialMonitor(); // Muestra información en el Monitor Serial
    delay(10);
    if (digitalRead(pinCalib) == LOW) { // Inicia calibración al presionar el botón
      calibration();
      break;
    }
  } 
 //==control==
  position = positionMin;
  prePosition = positionMin;
  while (1) {
    if (digitalRead(pinCalib) == LOW) swCount0 += 1; // Detecta pulsación en botón de calibración
    else swCount0 = 0;
    if (swCount0 == 10) { // Activa calibración tras 10 lecturas bajas continuas
      swCount0 = 0;
      calibration();
    }
    if (digitalRead(pinExtra) == LOW) swCount1 += 1; // Detecta pulsación en botón extra
    else swCount1 = 0;
    if (swCount1 == 10) { // Invierte movimiento si botón extra se mantiene presionado
      swCount1 = 0;
      isReversed = !isReversed;
      while (digitalRead(pinExtra) == LOW) delay(1); // Espera hasta que el botón se suelte
    }
    if (digitalRead(pinThumb) == LOW) swCount2 += 1; // Detecta pulsación en botón del pulgar
    else swCount2 = 0;
    if (swCount2 == 10) { // Alterna el estado del pulgar entre abierto/cerrado
      swCount2 = 0;
      isThumbOpen = !isThumbOpen;
      while (digitalRead(pinThumb) == LOW) delay(1);
    }
    if (digitalRead(pinOther) == LOW) swCount3 += 1; // Detecta pulsación en botón de bloqueo para otros dedos
    else swCount3 = 0;
    if (swCount3 == 10) { // Alterna bloqueo/desbloqueo de otros dedos
      swCount3 = 0;
      isOtherLock = !isOtherLock;
      while (digitalRead(pinOther) == LOW) delay(1);
    }
    
    sensorValue = readSensor(); // Lee el valor del sensor
    delay(25);
    if(sensorValue<sensorMin) sensorValue=sensorMin;
    else if(sensorValue>sensorMax) sensorValue=sensorMax;
    sensorToPosition(); // Convierte la lectura de sensor a una posición de movimiento

    // Mapeo de posición para el índice
    outIndex = map(position, positionMin, positionMax, outIndexOpen, outIndexClose);
    servoIndex.write(outIndex); // Actualiza la posición del servo del índice
    if (!isOtherLock){ // Control de otros dedos si no están bloqueados
      outOther = map(position, positionMin, positionMax, outOtherOpen, outOtherClose);
      servoOther.write(outOther);
    }

    outThumb = map(position, positionMin, positionMax, outThumbOpen, outThumbClose); // Mueve el pulgar
    servoThumb.write(outThumb);

    if(onSerial) serialMonitor(); // Muestra valores en el Monitor Serial
  } 
}

/*
 * Funciones adicionales
 */
int readSensor() {
  // Promedia 10 lecturas del sensor para una medición estable
  int i = 0, sval = 0;
  for (i = 0; i < 10; i++) {
    sval += analogRead(pinSensor);
  }
  sval = sval/10;
  return sval;
}

void sensorToPosition(){
  // Ajusta la velocidad y posición según el valor del sensor y la configuración de inversión
  int tmpVal = 0;
  if (isReversed) {
    tmpVal = map(sensorValue, sensorMin, sensorMax, 0, 100);
  } else {
    tmpVal = map(sensorValue, sensorMin, sensorMax, 100, 0);
  }
  if(tmpVal<thSpeedReverse) speed=speedReverse;
  else if(tmpVal<thSpeedZero) speed=speedMin;
  else speed=map(tmpVal,40,100,speedMin,speedMax);
  position = prePosition + speed;
  if (position < positionMin) position = positionMin;
  if (position > positionMax) position = positionMax;
  prePosition = position;
}

void calibration() {
  // Ejecuta el proceso de calibración, estableciendo límites del sensor y posiciones iniciales de los servos
  outIndex=outIndexOpen;
  servoIndex.write(outIndexOpen);
  servoOther.write(outOtherClose);
  servoThumb.write(outThumbOpen);
  position=positionMin; 
  prePosition=positionMin;
  
  delay(200);
  if(onSerial) Serial.println("======inicio de calibración======");

  sensorMax = readSensor();
  sensorMin = sensorMax - 50;
  unsigned long time = millis();
  while ( millis() < time + 4000 ) {
    sensorValue = readSensor();
    delay(25);
    if ( sensorValue < sensorMin ) sensorMin = sensorValue;
    else if ( sensorValue > sensorMax )sensorMax = sensorValue;
    
    sensorToPosition();
    outIndex = map(position, positionMin, positionMax, outIndexOpen, outIndexClose);    
    servoIndex.write(outIndex);
    
    if(onSerial) serialMonitor();
  }
  if(onSerial)  Serial.println("======fin de calibración======");
}

void serialMonitor(){
  // Muestra en el Monitor Serial los datos actuales de posición, sensor y estado de dedos
  Serial.print("Min:"); Serial.print(sensorMin);
  Serial.print(",Max:"); Serial.print(sensorMax);
  Serial.print(",sensor:"); Serial.print(sensorValue);
  Serial.print(",speed:"); Serial.print(speed);
  Serial.print(",position:"); Serial.print(position);
  Serial.print(",outIndex:"); Serial.print(outIndex);
  Serial.print(",isThumbOpen:"); Serial.print(isThumbOpen);
  Serial.print(",isOtherLock:"); Serial.println(isOtherLock);
}
