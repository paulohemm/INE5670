/* 
Este é código do projeto realizado para a materia INE-5670 (Desenvolvimento de Sistemas Móveis
e Embarcados)do semestre 2021.1 dos acadêmicos: Paulo Hemm e Vanessa Cunha, o projeto neste 
código consiste em realizar o desenvolvimento de um sistema IoT sendo no caso a automatização
de irrigação de uma mini horta de apartamento.

Definições solicitadas pela biblioteca Blynk */
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPLnwHtRIZH"
#define BLYNK_DEVICE_NAME "Teste A"

/*================================Bibliotecas========================================
/*Biblioteca nativa responsavel pela comunicação serial da placa com o computador.*/
#include <SPI.h>

/*Carrega a biblioteca do sensor ultrassonico*/
#include <Ultrasonic.h>

/* Biblioteca nativa responsavel pela conexão da placa com a internet via wi-fi e hora atual*/ 
#include <NTPClient.h>//Biblioteca do NTP.
#include <ESP8266WiFi.h>//Biblioteca do WiFi.
#include <WiFiUDP.h>//Biblioteca do UDP.

/* Biblioteca responsável pela conexão do arduino com o aplicativo, bem como o envio 
de dados para aplicação.*/ 
#include <BlynkSimpleEsp8266.h>

/*================================Sensores=======================================|
//Pinagem do sensor ultrassônico, echo sendo modo input e trigger modo output*/
#define pino_echo D13 // Pino Echo
#define pino_trigger D12 // Pino Trigger

/*Inicializa o sensor nos pinos definidos acima*/
Ultrasonic ultrasonic(pino_trigger, pino_echo);

/*Pinagem do sensor de umidade e criação da variavel para armazenamento do valor*/
#define SensorUmidade A0
double valorSensorUmidade;

/*Pinagem do que acionara o relé da motobomba responsavel por irrigar a horta*/
#define BombaAgua D8

/*============================Estados da FSM=====================================|
 O projeto foi definindo usando a ideia de FSM (Finite State Machine) o que possibilita o 
 looping de execução do código realizando os testes necessario para ver se há necessidade 
 de irrigação da horta.
 Para o momento atual do projeto foi definido 7 estados definidos abaixo que serão comentados
 abaixo no void loop()*/

#define S_Inicial                D0
#define S_EsperaHorario          D1
#define S_VerificaUmidadeSolo    D2
#define S_VerificaVolumeAgua     D3
#define S_ErroVolumeAgua         D4
#define S_AcionaBombaAgua        D5

/*================================Definições=====================================|*/

BlynkTimer timer;

WiFiUDP udp;//Cria um objeto "UDP".
NTPClient ntp(udp, "a.st1.ntp.br", -3 * 3600, 60000);//Cria um objeto "NTP" com as configurações.

/* Token da API do Blynk para envio de dados dos sensores*/
char auth[] = "rhJBD8CZENyKOyjuODgqKM1ZyxdIUL9B";

/*Credenciais da rede wi-fi para se conectar na internet*/
char ssid[] = "no puedo estoy sem senhita";
char pass[] = "quemerusbenarede";

/*variaveis criadas para a execução dos testes*/
float distancia_cm, volume_disponivel;
long tempo_distancia;
String hora;

/*função criada para verificar a hora atual*/
void verificaHora()
{
  hora = ntp.getFormattedTime();//Armazena na váriavel HORA, o horario atual.
}

/*função criada para medir o volume de água que esta armazenado no compartimento,  O volume disponivel foi 
 * obtido atraves da equação da melhor reta, fazendo testes aumentando o nível de água em 500 ml e realizando
 * medição da distancia, após os calculos realizados chegamsos a equação v = -0.2823*d + 5.9258 o que retornou
   um valor muito proximo do esperado*/    
void verificaVolume()
{
  //Le as informacoes do sensor, em cm 
  tempo_distancia = ultrasonic.timing();
  distancia_cm = ultrasonic.convert(tempo_distancia, Ultrasonic::CM);

  //formula para descobrir o volume atual
  volume_disponivel = -0.2823*distancia_cm + 5.9258;
  if (volume_disponivel < 0)
    volume_disponivel = 0;

  /*Envio da leitura do sensor para o aplicativo*/
  if (isnan(volume_disponivel)) {
    Serial.println("Falha ao ler o sensor!");
    return;
  }
  Blynk.virtualWrite(V1, volume_disponivel);

}

/*função criada para medir a umidade do solo, utilizando o sensor HL-69*/
 
void verificaUmidade()
{
  /*Inicia a medição aguardando o retorno*/
  pinMode(SensorUmidade, INPUT);
  valorSensorUmidade = analogRead(SensorUmidade);
  
  /*Envio da leitura do sensor para o aplicativo*/
  if (isnan(valorSensorUmidade)) {
    Serial.println("Falha ao ler o sensor!");
    return;
  }
  Blynk.virtualWrite(V3, valorSensorUmidade);

}

//==================================Setup========================================|

void setup()
{
  /*inicialização do monitor serial */
  Serial.begin(9600);

  /*Inicializa o sensor nos pinos definidos acima*/
  pinMode(BombaAgua, OUTPUT);
  pinMode(SensorUmidade, INPUT);

  /*Inicialização da rede*/
  Blynk.begin(auth, ssid, pass); 
  
  /*Função do setup do blynk que chama a leitura dos sensores a cada 10 segundos*/
  timer.setInterval(1000L, verificaVolume);
  timer.setInterval(1000L, verificaUmidade);

  ntp.begin();//Inicia o NTP.
  ntp.forceUpdate();//Força o Update.

}

//========================Execução em looping do código==========================|
void loop()
{ 
  /*inicialização da comunicação via wi-fi com a api do Blynk e o arduino*/
  Blynk.run();
  timer.run();
  
  /*estado inicial para a inicialização da FSM*/
  static int state = S_Inicial;

  switch(state)
  {
    /*Estado inicial usado para setar informações e indica qual é o proximo estado a ser 'visitado' S_EsperaHorario*/
    case S_Inicial:
      {
        Serial.println("Iniciando o programa");
        Blynk.virtualWrite(V5, "Estado Inicial");
        state = S_EsperaHorario;
        break;
      }

    /*Estado onde a placa verifica o horário atual, caso o valor retornado seja igual a 18:30h
     * o estado muda para S_VerificaUmidadeSolo senão permace em looping no estado realizando nova leitura a cada 10 minutos*/
    case S_EsperaHorario:
      {
        Serial.println("=======================================================");
        Serial.print("Horário atual -> ");
        verificaHora();
        Serial.println(hora);
        if (hora >= "18:30:00" and hora <= "18:50:00")
        {              
          state = S_VerificaUmidadeSolo;
          delay(5000); 
        }
        else
        {
          Serial.println("Aguardando o horário.");
          delay(600000);
          state = S_EsperaHorario;
        }
        break;
      }


    /*Estado onde a placa verifica a leitura do sensor de umidade, caso o valor retornado seja inferior
    a 800(leitura analogica) indicando que o solo está molhado (conforme testes) o estado muda para S_EsperaHorario 
    senão o estado muda S_VerificaVolumeAgua já que a leitura considerou que o solo está seco.*/
    case S_VerificaUmidadeSolo:
      {
        verificaUmidade();
        Serial.println("=======================================================");
        Serial.print("Valor do sensor de umidade -> ");
        Serial.println(valorSensorUmidade);  
        if (valorSensorUmidade < 800)
        {
          Serial.println("Não é necessario regar a planta");
          state = S_EsperaHorario;
          delay(5000); 
        }
        else
        {
          Serial.println("hora de regar sua planta!");
          state = S_VerificaVolumeAgua;
          delay(5000); 
        }
        break;
      }

    /*Estado onde a placa verifica o volume de agua no reservatorio, caso o valor retornado seja superior
    a 1,2 litros (considerado o volume morto necessário para a motobomba funcionar) indicado que a água disponível
    o estado muda para S_AcionaBombaAgua senão o estado muda S_ErroVolumeAgua.*/
    case S_VerificaVolumeAgua:
      {
         verificaVolume();
         Serial.println("=======================================================");
         Serial.print("Distancia até o nivel de água em cm: ");
         Serial.println(distancia_cm);
         Blynk.virtualWrite(V5, "Verificando Volume de água");
         Serial.print("Volume disponivel em litros: ");
         Serial.println(volume_disponivel);
         if((volume_disponivel > 1.20)) 
        {    
          state = S_AcionaBombaAgua;
          delay(5000); 
        }
        else
        {
          state = S_ErroVolumeAgua;
          delay(5000); 
        }
        break;
      }

    /*Estado onde a placa liga a motobomba durante 5 segundos realizando a irrigação da horta
    e apos este tempo muda para o estado S_EsperaAmanhecer começando um novo ciclo*/
    case S_AcionaBombaAgua:
      {
        Blynk.virtualWrite(V5, "Realizando Irrigação");
        Serial.println("=======================================================");
        Serial.println("Realizando Irrigação");
        digitalWrite(BombaAgua, HIGH);
        delay(5000);
        digitalWrite(BombaAgua, LOW);
        state = S_EsperaHorario;
        break;
      }

    /*Estado onde a placa fica verificando a cada segundo se o usuario acrescentou agua ao reservatorio
     caso for adicionado água ao reservatorio ele muda o estado para S_EsperaAnoitecer senão aguarda em looping
     no estado atual*/
    case S_ErroVolumeAgua:
      {
        Blynk.virtualWrite(V5, "Erro Volume de Água");
        Serial.println("=======================================================");
        Serial.println("Erro Volume de Água"); 
        if((volume_disponivel > 1.20))
        {            
          state = S_EsperaHorario;
        }
        else
        {
          delay(1000);
          state = S_ErroVolumeAgua;
        }
        break;
      }

       
  }    
}
