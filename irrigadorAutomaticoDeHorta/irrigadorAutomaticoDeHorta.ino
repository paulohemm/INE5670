//adicionar cabeçalho

//==============================Bibliotecas======================================|
#include <HCSR04.h>

//================================Sensores=======================================|
int triggerUltrassonico = D13;
int echoUltrassonico = D12;
UltraSonicDistanceSensor distanceSensor(triggerUltrassonico, echoUltrassonico);

#define SensorUmidade  A0
int valorSensorUmidade;

#define SensorLuminosidade  9
int valorSensorLuminosidade; 

int BombaAgua = D7;


//============================Estados da FSM=====================================|
#define S_Inicial                0
#define S_EsperaAnoitecer        1
#define S_VerificaUmidadeSolo    2
#define S_EsperaAmanhecer        3
#define S_VerificaVolumeAgua     4
#define S_ErroVolumeAgua         5
#define S_AcionaBombaAgua        6

//==================================Setup========================================|
void setup()
{
  Serial.begin(9600);
  pinMode(BombaAgua, OUTPUT);
  pinMode(SensorUmidade, INPUT);
  pinMode(SensorLuminosidade, INPUT);
}

//========================Execução em looping do código==========================|
void loop()
{
  static int state = S_Inicial;
  
  switch(state)
  {

    case S_Inicial:
      {
        state = S_EsperaAnoitecer;
        break;
      }

    case S_EsperaAnoitecer:
      {
        valorSensorLuminosidade = analogRead(SensorLuminosidade);
        Serial.println(SensorLuminosidade);
        if (SensorLuminosidade < 750 ) 
        {              
          state = S_VerificaUmidadeSolo;
        }
        else
        {
          delay(600000); 
          state = S_EsperaAnoitecer;
        }
        break;
      }
   
    case S_VerificaUmidadeSolo:
      {
        valorSensorUmidade = analogRead(SensorUmidade);
        Serial.println(valorSensorUmidade);
        if (valorSensorUmidade > 400 ) 
        {   
          state = S_VerificaVolumeAgua;
        }
        else
        { 
          state = S_EsperaAmanhecer;
        }
        break;
      }

    case S_VerificaVolumeAgua:
      {
        double distance = distanceSensor.measureDistanceCm();
        Serial.println(distance);
        if((distance > 20)) 
        {    
          state = S_AcionaBombaAgua;
        }
        else
        {
          //msg volume agua baixo
          state = S_ErroVolumeAgua;
        }
        break;
      }
           
    case S_AcionaBombaAgua:
      {
        digitalWrite(BombaAgua, HIGH);
        delay(1000);
        digitalWrite(BombaAgua, LOW);
        state = S_EsperaAmanhecer;
        break;
      }

    case S_ErroVolumeAgua:
      {
        double distance = distanceSensor.measureDistanceCm();
        Serial.println(distance);
        if((distance > 20))
        {              
          state = S_EsperaAnoitecer;
        }
        else
        {
          delay(600000);
          state = S_ErroVolumeAgua;
        }
        break;
      }
       
    case S_EsperaAmanhecer:
      {
        valorSensorLuminosidade = analogRead(SensorLuminosidade);
        Serial.println(SensorLuminosidade);
        if (SensorLuminosidade > 750 )
        {              
          state = S_EsperaAnoitecer;
        }
        else
        {
          delay(600000);
          state = S_EsperaAmanhecer;
        }
        break;
      }
       
       
  }    
}
