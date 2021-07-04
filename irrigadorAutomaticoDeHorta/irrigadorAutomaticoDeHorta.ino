/* 
Este é código do projeto realizado para a materia INE-5670 (Desenvolvimento de Sistemas Móveis
e Embarcados)do semestre 2021.1 dos acadêmicos: Paulo Hemm e Vanessa Cunha, o projeto neste 
código consiste em realizar o desenvolvimento de um sistema IoT sendo no caso a automatização
de irrigação de uma mini horta de apartamento.

Definições solicitadas pela biblioteca Blynk */
#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "[TEMPLATE ID]"
#define BLYNK_DEVICE_NAME "[DEVICE NAME]"

/*================================Bibliotecas========================================
   Biblioteca nativa responsavel pela comunicação serial da placa com o computador.*/
#include <SPI.h>

/* Biblioteca nativa responsavel pela conexão da placa com a internet via wi-fi.*/ 
#include <ESP8266WiFi.h>

/* Biblioteca responsável pela conexão do arduino com o aplicativo, bem como o envio 
de dados para aplicação.*/ 
#include <BlynkSimpleEsp8266.h>
/*================================Sensores=======================================|
Pinagem do sensor ultrassônico, echo sendo modo input e trigger modo output*/
#define echoPin D13 // Pino Echo
#define trigPin D12 // Pino Trigger

/*Pinagem do sensor de umidade e criação da variavel para armazenamento do valor*/
#define SensorUmidade D7
double valorSensorUmidade;

/*Pinagem do sensor de Luminosidade e criação da variavel para armazenamento do valor*/
#define SensorLuminosidade A0
double valorSensorLuminosidade; 

/*Pinagem do que acionara o relé da motobomba responsavel por irrigar a horta*/
#define BombaAgua D8


/*============================Estados da FSM=====================================|
 O projeto foi definindo usando a ideia de FSM (Finite State Machine) o que possibilita o 
 looping de execução do código realizando os testes necessario para ver se há necessidade 
 de irrigação da horta.
 Para o momento atual do projeto foi definido 7 estados definidos abaixo que serão comentados
 abaixo no void loop()*/

#define S_Inicial                D0
#define S_EsperaAnoitecer        D1
#define S_VerificaUmidadeSolo    D2
#define S_EsperaAmanhecer        D3
#define S_VerificaVolumeAgua     D4
#define S_ErroVolumeAgua         D5
#define S_AcionaBombaAgua        D6


/*================================Definições=====================================|*/

BlynkTimer timer;

/* Token da API do Blynk para envio de dados dos sensores*/
char auth[] = "[TOKEN API]";

/*Credenciais da rede wi-fi para se conectar na internet*/
char ssid[] = "[NOME DA REDE]";
char pass[] = "[SENHA DA REDE]";

/*variaveis criadas para a execução dos testes*/
double duracao, altura, volume_medido, volume_disponivel;
double raio_do_compartimento = 11;
double volume_max = 8;

/*função criada para medir e enviar o volume de água que esta armazenado no compartimento, a medição */
void sendVolumeSensor()
{
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  /*Inicia a medição gerando um trigger e aguardando o retorno*/
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duracao = pulseIn(echoPin, HIGH);
  
  /*Calcula a altura que o sensor se encontra do nivel de agua (em cm) baseado na velocidade do som.*/
  altura = duracao/58.2;

  /*Calcula a volume do reservatorio com base na formula V=pi*r^2*altura e divide por 1000 para obter o resultado em litros.*/
  volume_medido = 3.14159265359*altura*11*11;
  volume_medido = volume_medido/1000;

  /*O volume medido é o volume de ar presente dentro do reservatorio, então se faz necessario a subtração do volume maximo
  que o reservatorio pode ter, pelo volume medido de ar, retornando o volume de agua disponivel*/
  volume_disponivel = volume_max - volume_medido;
  Serial.println(volume_disponivel);
  
  /*Atraso de 1 segundo antes da próxima leitura*/
  delay(1000);

  /*Envio da leitura do sensor para o aplicativo*/
  if (isnan(volume_disponivel)) {
    Serial.println("Falha ao ler o sensor!");
    return;
  }
  Blynk.virtualWrite(V1, volume_disponivel);

}

void sendLuzSensor()
{
  pinMode(SensorLuminosidade, INPUT);
  /*Inicia a medição aguardando o retorno*/
  valorSensorLuminosidade = analogRead(SensorLuminosidade);
  Serial.println(valorSensorLuminosidade);
  /*Atraso de 1 segundo antes da próxima leitura*/
  delay(1000);
  /*Envio da leitura do sensor para o aplicativo*/
  if (isnan(valorSensorLuminosidade)) {
    Serial.println("Falha ao ler o sensor!");
    return;
  }
  Blynk.virtualWrite(V2, valorSensorLuminosidade);

}

void sendUmidadeSensor()
{
  pinMode(SensorUmidade, INPUT);
  /*Inicia a medição aguardando o retorno*/
  valorSensorUmidade = analogRead(SensorUmidade);
  Serial.println(valorSensorUmidade);
  /*Atraso de 1 segundo antes da próxima leitura*/
  delay(1000);
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

  /*configuração de qual pino é leitura e qual é escrita*/
  pinMode(BombaAgua, OUTPUT);
  pinMode(SensorUmidade, INPUT);
  pinMode(SensorLuminosidade, INPUT);

  /*Inicialização da rede*/
  Blynk.begin(auth, ssid, pass); 
  
  /*Função do setup do blynk que chama a leitura dos sensores a cada 10 segundos*/
  timer.setInterval(1000L, sendVolumeSensor);
  timer.setInterval(1000L, sendLuzSensor);
  timer.setInterval(1000L, sendUmidadeSensor);

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
    /*Estado inicial usado para setar informações e indica qual é o proximo estado a ser 'visitado' S_EsperaAnoitecer*/
    case S_Inicial:
      {
        state = S_EsperaAnoitecer;
        break;
      }

    /*Estado onde a placa verifica a leitura do sensor de luminosidade, caso o valor retornado seja inferior
    a 750(leitura analogica) indicando que é noite (conforme testes) o estado muda para S_VerificaUmidadeSolo 
    senão permace em looping no estado realizando nova leitura a cada 10 minutos*/
    case S_EsperaAnoitecer:
      {
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


    /*Estado onde a placa verifica a leitura do sensor de umidade, caso o valor retornado seja superior
    a 400(leitura analogica) indicando que o solo está seco (conforme testes) o estado muda para S_VerificaVolumeAgua 
    senão o estado muda S_EsperaAmanhecer já que a leitura considerou que o solo já está umido não sendo necessaria a irrigação.*/
    case S_VerificaUmidadeSolo:
      {
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

    /*Estado onde a placa verifica o volume de agua no reservatorio, caso o valor retornado seja superior
    a 1,2 litros (considerado o volume morto necessário para a motobomba funcionar) indicado que a água disponível
    o estado muda para S_AcionaBombaAgua senão o estado muda S_ErroVolumeAgua.*/
    case S_VerificaVolumeAgua:
      {
         if((volume_disponivel > 1.20)) 
        {    
          state = S_AcionaBombaAgua;
        }
        else
        {
          state = S_ErroVolumeAgua;
        }
        break;
      }

    /*Estado onde a placa liga a motobomba durante 5 segundos realizando a irrigação da horta
    e apos este tempo muda para o estado S_EsperaAmanhecer começando um novo ciclo*/
    case S_AcionaBombaAgua:
      {
        digitalWrite(BombaAgua, HIGH);
        delay(5000);
        digitalWrite(BombaAgua, LOW);
        state = S_EsperaAmanhecer;
        break;
      }

    /*Estado onde a placa fica verificando a cada segundo se o usuario acrescentou agua ao reservatorio
     caso for adicionado água ao reservatorio ele muda o estado para S_EsperaAnoitecer senão aguarda em looping
     no estado atual*/
    case S_ErroVolumeAgua:
      {
        if((volume_disponivel > 1.20))
        {              
          state = S_EsperaAnoitecer;
        }
        else
        {
          delay(1000);
          state = S_ErroVolumeAgua;
        }
        break;
      }

    /*Estado similar o anoitecer onde a placa verifica a leitura do sensor de luminosidade, caso o valor retornado seja superior
    a 750(leitura analogica) indicando que é dia (conforme testes) o estado muda para S_EsperaAnoitecersenão permace em looping 
    no estado realizando nova leitura a cada 10 minutos, este estado é necessário para não ser realizado mais de uma irrigação por noite*/       
    case S_EsperaAmanhecer:
      {
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
