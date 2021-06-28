/**
 * @file society_pt_crono.cpp
 * @author Gladson Araujo (gladsonam@gmail.com)
 * @brief Sistema Desenvolvido para painel Society
 * básico com pontuação 00-99 e cronometro quatro
 * digitos (minutos e segundos) com opção de modo progressivo e regressivo 
 * @version 0.1
 * @date 2021-03-18
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include <Arduino.h>
#include <LedControl.h>
#include <TimerOne.h>
#include <RCSwitch.h>
#include <Wire.h>
#include <EEPROM.h>
#include <CountUpDownTimer.h>
#include <TimeLib.h>
#include <DS1307.h>




// Definicoes dos pinos e quantidade de modulos no circuito
//                      din(1)/clk(13)/load(12)/n_de_ci's
LedControl lc= LedControl(12,    8,      10,       1);
//Instancia do cronometro
CountUpDownTimer T(DOWN,LOW);
//Instancia do relogio
DS1307 rtc;
//Instancia do controle remoto
RCSwitch mySwitch = RCSwitch();

#define espacoEEPROM 1000
#define botInc 14 //pino 23
#define botDec 15 //pino 24
#define botSelConfig 16 //pino 25
#define botZera 17 //pino 26
#define botCrono 5 //pino 11
#define botSetCrono 6 //pino 12
#define alarm 4 //pino 6
#define pinTerra1 0 //pino 2
#define pinTerra2 1 //pino 3

////////////////////////////////////////////DECLARAÇÃO DE VARIÁVEIS

int contDelayDeTecla = 0; 
int contpontos1 = 0; // GUARDA O VALOR ATUAL DE PONTOS
int contpontos2 = 0; // GUARDA O VALOR ATUAL DE PONTOS
int bufpontos1 = 0;  // GUARDA O VALOR ANTERIOR E INFORMAR QUANDO FOR DECREMENTADO
int bufpontos2 = 0;  // GUARDA O VALOR ANTERIOR E INFORMAR QUANDO FOR DECREMENTADO
int cont = 0;
int brilho = 0; // GUARDA O VALOR DO BRILHO DO DISPLAY
int crono = 0;  // ALTERNA O COMANDO DE START/STOP DO CRONOMETRO
int y = 0;      // VARIAVEL DE IMPRESSÃO NO DISPLAY
int ajuste = 0;    // PERMITE BOTÕES DE INCREMENTO/DECREMENTO ALTERNAR ENTRE PONTOS (0), BRILHO(1) OU RELOGIO(2)
int o = 0;
int loop_pisca = 0; //VARIÁVEL QUE PERMITE LOOP PARA PISCAR PONTOS/CRONOMETRO
int loop_pisca2 = 0; //VARIÁVEL QUE PERMITE LOOP PARA PISCAR RELOGIO
int setsPisca = 0;
int segundo;
int minuto;
int hora;
int dia;
int mes;
int ano;
int buffer_ajuste = 0;  //GUARDA A FUNÇÃO DAS TECLAS INC/DEC
int q = 0;  //CONTADOR DE LOOP P/ GRAVAÇÃO DE TX
byte segCont = 0;
unsigned long setmin = 0;
unsigned long setseg = 10;
unsigned long min = 0;
unsigned long seg = 0;
unsigned long bufmin = 0;
unsigned long bufseg = 0;
unsigned long b = 0;
unsigned long c = 0;
unsigned long d = 0;
unsigned long e = 0;
unsigned long delaytime = 20;
long recebeRX;
long controle1[6];
long controle2[6];
boolean prog = 0; //VARIÁVEL QUE DEFINE MODO DO CRONOMETRO
boolean seltime = 0; //VARIAVEL DE SELEÇÃO DE JOGADOR
boolean a = 0; //VARIÁVEL AUXILIA NAS FUNÇÕES 'PISCA'
boolean m = 0; //VARIAVEL DE CONTROLE DE RETENÇÃO DO TRANSMISSOR
boolean n = 0;
boolean bloq_pisca = 1; //VARIÁVEL QUE BLOQUEIO INCREMENTO DURANTE PISCA PONTOS/GAMES/SETS
boolean bloq_tecla = 1; //VARIAVEL PARA EVITAR LOOP DAS TECLAS
boolean f = 0;
boolean bloq = 0;
boolean relogio = 0;
boolean l = 0;
boolean ajuste_relogio = 0;
////////////////////////////////////////////DECLARAÇÃO DE FUNÇÕES


void exibe_ponto1();
void exibe_ponto2();
void exibe_crono(unsigned long d, unsigned long e);
void grava_tx1();
void grava_tx2();
void trava_rx();
void eeprom_escreve(int endereco, int val);
void eeprom_escreveLong(int endereco, long val);
int  eeprom_le(int endereco);
long eeprom_leLong(int endereco);
void atualizacontpontos();
void atualizabufpontos();
void atualizacrono();
void atualizabufcrono();
void pisca_pontos();
void pisca_pontos1();
void pisca_pontos2();
void pisca_crono();
void pisca_hora();
void pisca_minuto();
void modo_cronoNP();
void modo_cronoP();


ISR(TIMER2_OVF_vect){ 
  TCNT2 = 0B11111010;
  cont++;
  bloq_pisca=0;
  if(cont==800){
    TIMSK2 = 0B00000000;
    mySwitch.enableReceive(0);
    cont=0;
    m=0;
    bloq_pisca=1;
  } 
}

void setup() {

  pinMode(botInc, INPUT_PULLUP);
  pinMode(botDec, INPUT_PULLUP);
  pinMode(botSelConfig, INPUT_PULLUP);
  pinMode(botZera, INPUT_PULLUP);
  pinMode(botCrono, INPUT_PULLUP);
  pinMode(botSetCrono, INPUT_PULLUP);
  pinMode(alarm, OUTPUT);
  pinMode(pinTerra1, OUTPUT);
  pinMode(pinTerra2, OUTPUT);
  digitalWrite(pinTerra1, 0);
  digitalWrite(pinTerra2, 0);

  // Inicializa o 7219
  lc.shutdown(0,false);
  //lc.shutdown(1,false);
  
  // Ajuste do brilho do display
  lc.setIntensity(0,brilho);
  
  // Apaga o display
  lc.clearDisplay(0);
  //lc.clearDisplay(1);
  
  // Limita o numero de digitos
  lc.setScanLimit(0,8);
  //lc.setScanLimit(1,4);

  //Definição do cronometro
  T.SetTimer(0,0, setseg);  //config p/ regressivo
  //T.SetStopTime(0, 0, setseg); //config p/ progressivo
  T.StartTimer();

  // RECEPÇÃO DO RX PELA INTERRUPÇÃO 0(PINO 2)
  pinMode(2, INPUT);
  mySwitch.enableReceive(0);  

  // configuração do relógio
  //rtc.set(0, 25, 14, 21, 06, 2021); //08:00:00 21.06.2021 //segundo, minuto, hora, dia, mês, ano
  
  //stop/pause RTC
  // rtc.stop();

  //start RTC
  rtc.start();

  /////CONFIGURAÇÃO DO TIMER2
  
  //estouro de pilha    =  (ciclo de maq) X (prescaller)  X (valor inicial da pilha)
  //        16ms      =       62,5ns    X     1024      X          250
  
  TCCR2A = 0B00000000;  //MODO DO TIMER : NORMAL
  TCCR2B = 0B00000111;  //VALOR DE PRESCALLER PARA 1/1024 (ciclo de maquina*1024)  
  TCNT2 =  0B11111010;  //VALOR INICIAL DA PILHA DE CONTAGEM DO TIMER 2 EM 0 (de 256)    
  TIMSK2 = 0B00000000;  //INTERRUPÇÃO DESABILITADA POR ESTOURO DE PILHA
  
  /////Limpeza da EEPROM Executada apenas para limpar a memoria na primeira gravação do CI
  /*for (int nL = 0; nL < espacoEEPROM; nL++) { 
    EEPROM.write(nL, 0);
  } */ //Comentar até aqui.
  
  for(int i=0; i<=5; i++){
    controle1[i] = eeprom_leLong(i*4);
  }
  for(int i=6; i<=11; i++){
    controle2[i] = eeprom_leLong(i*4);
  }

  contpontos1 = eeprom_le(48);
  contpontos2 = eeprom_le(49);  
  seltime = eeprom_le(50);
  //min = eeprom_le(51);
  //seg = eeprom_le(52);
  exibe_ponto1(); 
  exibe_ponto2();
  exibe_crono(setmin, setseg);
}

void loop() {
  
  if (relogio){
    rtc.get(&segundo, &minuto, &hora, &dia, &mes, &ano);
    exibe_crono(hora, minuto);
  }  
  if(crono==1){
    T.Timer();                    // INICIA CRONOMETRO 
    if (T.TimeHasChanged()){
      d=T.ShowMinutes();
      e=T.ShowSeconds();  
      if (prog){
        d=setmin-d;
        e=setseg-e;
      }
      exibe_crono(d, e);      
      if((T.ShowMinutes())==b && (T.ShowSeconds())==c) {
        T.StopTimer();
        T.SetTimer(0, 0, setseg); //REGRESSIVO
        T.StartTimer();
        crono=0;
        bloq_pisca=0;
        Timer1.attachInterrupt(pisca_crono);
        Timer1.initialize(400000);        
      }
    }
  }

  if(bloq_tecla && bloq_pisca){      
    
    if((!m)&&(mySwitch.available())){
      recebeRX = (mySwitch.getReceivedValue());
      mySwitch.resetAvailable();
      mySwitch.disableReceive();
      m=1;      
      if(recebeRX == controle1[0] || recebeRX == controle2[0])goto incrementa;
      if(recebeRX == controle1[1] || recebeRX == controle2[1])goto decrementa;
      if(recebeRX == controle1[2] || recebeRX == controle2[2])goto seleciona;
      if(recebeRX == controle1[3] || recebeRX == controle2[3])goto zera;
      if(recebeRX == controle1[4] || recebeRX == controle2[4])goto cronometro;
      if(recebeRX == controle1[5] || recebeRX == controle2[5])goto setCrono;
    } 
    if(m){
      TIMSK2 = 0B00000001;
    }
    ////////////////////////////////////////////////////
    //INCREMENTA PONTO INCREMENTA PONTO INCREMENTA PONTO   
    ////////////////////////////////////////////////////                    
    if(!digitalRead(botInc)){
      delay(20);
      if(!digitalRead(botInc)){
        incrementa:
        recebeRX = 0;
        bloq_tecla=0;
        switch (ajuste){
        case 0:
          if(!seltime){                             //SE SELEÇÃO DE JOGADOR FOR TIME 1
            if(contpontos1<5){                      //SE TIME 1 TIVER MENOS DE xx PONTOS        
              atualizabufpontos();                  //ARMAZENA NO BUFFER OS PONTOS DOS JOGADORES
              contpontos1++;                        //INCREMENTA O PONTO DO TIME 1 
              exibe_ponto1();                       //ESCREVE PONTO DO TIME 1 NO DISPLAY
              eeprom_escreve(48, contpontos1);
            }else{
              bloq_pisca = 0;
              Timer1.attachInterrupt(pisca_pontos1);
              Timer1.initialize(400000);
            }
          }else{                                    //SE SELEÇÃO DE JOGADOR FOR TIME 2
            if(contpontos2<5){                     //SE TIME 2 TIVER MENOS DE xx PONTOS        
              atualizabufpontos();                  //ARMAZENA NO BUFFER OS PONTOS DOS JOGADORES
              contpontos2++;                        //INCREMENTA O PONTO DO TIME 2 
              exibe_ponto2();                       //ESCREVE PONTO DO TIME 2 NO DISPLAY
              eeprom_escreve(49, contpontos2);
            }else{
              bloq_pisca = 0;
              Timer1.attachInterrupt(pisca_pontos2);
              Timer1.initialize(400000);
            }
          } 
          break;

        case 1:
          brilho++;          
          if(brilho>13){
            brilho=13;
            bloq_pisca = 0;
            Timer1.attachInterrupt(pisca_pontos);
            Timer1.initialize(400000);
          }else lc.setIntensity(0,brilho);          
          break;

        case 2:
          hora ++;
          if(hora >23) hora = 0;
          rtc.set(0, minuto, hora, 1, 1, 2000);
          break;

        case 3:
          minuto ++;
          if(minuto > 59) minuto = 0;
          rtc.set(0, minuto, hora, 1, 1, 2000);
          break;

        default:
          break;
        }
        
        
      }
    }

    ////////////////////////////////////////////////////
    //DECREMENTA PONTO DECREMENTA PONTO DECREMENTA PONTO   
    //////////////////////////////////////////////////// 
    if(!digitalRead(botDec)){
      delay(20);
      if(!digitalRead(botDec)){
        decrementa:
        recebeRX = 0;
        bloq_tecla=0;
        switch (ajuste)
        {
        case 0:
          if(!seltime){                               
            if(contpontos1>0){                  
              contpontos1--;
              exibe_ponto1();                   //ESCREVE PONTO DO TIME 1 NO DISPLAY
              exibe_ponto2();                   //ESCREVE PONTO DO TIME 2 NO DISPLAY
              eeprom_escreve(48, contpontos1);  //SALVA NA EEPROM O VALOR ATUAL
            }else{
              bloq_pisca = 0;
              Timer1.attachInterrupt(pisca_pontos1);
              Timer1.initialize(400000);
            }
          }else{                                
            if(contpontos2>0){                  
              contpontos2--;
              exibe_ponto1();                   //ESCREVE PONTO DO TIME 1 NO DISPLAY
              exibe_ponto2();                   //ESCREVE PONTO DO TIME 2 NO DISPLAY
              eeprom_escreve(49, contpontos2);  //SALVA NA EEPROM O VALOR ATUAL
            }else{
              bloq_pisca = 0;
              Timer1.attachInterrupt(pisca_pontos2);
              Timer1.initialize(400000);
            }
          }
          break;
        
        case 1:
          brilho--;                 
          if(brilho<0){
            brilho=0;
            bloq_pisca = 0;
            Timer1.attachInterrupt(pisca_pontos);
            Timer1.initialize(400000);
          }else lc.setIntensity(0,brilho);
          break;
        
        case 2:
          if(hora == 0) hora = 24;
          hora --;
          rtc.set(0, minuto, hora, 1, 1, 2000);
          break;

        case 3:
          if(minuto == 0) minuto = 60;
          minuto --;
          rtc.set(0, minuto, hora, 1, 1, 2000);
          break;

        default:
          break;
        }
                
      }
    }

    ////////////////////////////////////////////////////
    //SELEÇÃO DE JOGADOR OU CONFIGURAÇÃO DO CONTROLE   
    //////////////////////////////////////////////////// 
    if(!digitalRead(botSelConfig)){
      delay(20);    
      while(!digitalRead(botSelConfig)&&contDelayDeTecla<150){
        delay(50);
        contDelayDeTecla++;
      }
      if(contDelayDeTecla==150){
        bloq_tecla=0;
        if(q==0){
          grava_tx1();
        }else if(q==6)grava_tx2();      
      
      }else{        
        seleciona:
        recebeRX = 0;
        seltime =!seltime;
        if(!seltime){
          //DESENVOLVER FUNÇÃO DE ACIONAMENTO DO LED DE SELEÇÃO DO TIME 1
          eeprom_escreve(50, seltime);         
        }else{
          //DESENVOLVER FUNÇÃO DE ACIONAMENTO DO LED DE SELEÇÃO DO TIME 2
          eeprom_escreve(50, seltime);
        }
      }
      contDelayDeTecla = 0;
    }
    ////////////////////////////////////////////////////
    //ZERA PONTO ZERA PONTO ZERA PONTO ZERA PONTO 
    //////////////////////////////////////////////////// 
    if(!digitalRead(botZera)){
      delay(20);    
      while(!digitalRead(botZera)&&contDelayDeTecla<30){
        delay(50);
        contDelayDeTecla++;
      }
      if(contDelayDeTecla==30){
        bloq_tecla=0;
        if (ajuste == 0){
          ajuste = 1;
          lc.setChar(0,4,'b',false);
          lc.setChar(0,5,' ',false);
          lc.setChar(0,6,' ',false);
          lc.setChar(0,7,' ',false);
        } else {
          ajuste = 0;
          if (crono == 0){
            exibe_crono(setmin, setseg);
          }else if (crono == 2)exibe_crono(d, e);
        }
        }else if (ajuste == 0){
          zera:                
          recebeRX = 0;
          bloq_tecla=0;       
          atualizabufpontos();
          contpontos1 = 0;
          contpontos2 = 0;
          exibe_ponto1();                   //ESCREVE PONTO DO TIME 1 NO DISPLAY
          exibe_ponto2();                   //ESCREVE PONTO DO TIME 2 NO DISPLAY
          eeprom_escreve(48, contpontos1);
          eeprom_escreve(49, contpontos2);
        }
      contDelayDeTecla = 0;
    }
    ////////////////////////////////////////////////////
    //INICIA/PARA/ZERA CRONOMETRO OU SELECIONA RELOGIO (TEMPORIZADO)
    //////////////////////////////////////////////////// 
    if(!digitalRead(botCrono)){
      delay(20);    
      while(!digitalRead(botCrono) && contDelayDeTecla < 30){
        delay(50);
        contDelayDeTecla ++;
      }      
        if(contDelayDeTecla == 30){    //TEMPORIZADO
          bloq_tecla = 0;
          if(!ajuste_relogio){
            if(crono == 2){
            T.StopTimer();
            T.SetTimer(0, 0, setseg); 
            T.StartTimer();
            exibe_crono(0, setseg);
            crono = 0;          
            }else if(crono == 0){
              relogio = !relogio;
              digitalWrite(alarm,relogio); 
              if(!relogio){
                d = setmin;
                e = setseg;
                exibe_crono(d, e);
              }
            }
          }            
        }else if(!relogio && !ajuste_relogio){  //INSTANTANEO      
          cronometro:
          recebeRX = 0;
          bloq_tecla = 0;
          switch (crono){          
          case 0:          //INICIA CRONOMETRO, CRONO = 1
            crono = 1;
            break;

          case 1:          // PARA CRONOMETRO, CRONO  = 2 
            T.PauseTimer();
            crono = 2;
            break;

          case 2:          // RETOMA CRONOMETRO, CRONO = 1 PRA PERMITIR PAUSAR
            T.ResumeTimer();
            crono = 1;
            break;

          default:
            break;
          }
        }
           
      contDelayDeTecla = 0;
    }
    
    ////////////////////////////////////////////////////
    //SETAR TEMPO E MODO DO CRONOMETRO OU PERMITE AJUSTE DE RELOGIO
    //////////////////////////////////////////////////// 
    
    if(!digitalRead(botSetCrono)){
      delay(20);    
      while(!digitalRead(botSetCrono)&&contDelayDeTecla<30){
        delay(50);
        contDelayDeTecla++;
      }
      if(contDelayDeTecla==30){ //TEMPORIZADO
        bloq_tecla = 0;        
        if(relogio==1){
          rtc.stop();
          relogio = 0;
          ajuste_relogio = 1;
          buffer_ajuste = ajuste;
          Timer1.initialize(300000); 
          Timer1.attachInterrupt(pisca_hora);
        }else if(!ajuste_relogio){
          if(prog){
            prog = 0;
            bloq_pisca = 0;
            Timer1.attachInterrupt(modo_cronoNP);
            Timer1.initialize(400000);            
          }else{
            prog = 1;
            bloq_pisca = 0;
            Timer1.attachInterrupt(modo_cronoP);
            Timer1.initialize(400000);          
          }
        }else{          
          loop_pisca2 = 1;
          rtc.start();
          relogio = 1;
          ajuste_relogio = 0;
        }
      }else{                 //INSTANTÂNEO
        bloq_tecla = 0;
        if(!relogio){
          if(!ajuste_relogio){
            setCrono:
            recebeRX = 0;
            setseg = setseg +5;
            if(setseg == 30)setseg = 5;
            exibe_crono(0, setseg);
            T.SetTimer(0, 0, setseg); 
          }else loop_pisca2 = 1;
        }              
      }  
      contDelayDeTecla = 0;
    }    
    
  }

   ////////////////////////////////////////////////////////////////////
   //IMPEDIR LOOP DE COMANDOS IMPEDIR LOOP DE COMANDOS  
   ////////////////////////////////////////////////////////////////////
   while((!bloq_tecla)&&(!digitalRead(botInc)||!digitalRead(botDec)||!digitalRead(botSelConfig)||
   !digitalRead(botZera)||!digitalRead(botCrono)||!digitalRead(botSetCrono)));
   bloq_tecla=1;
         

}
/////////////////////////////////////////BLOCO DE FUNÇÕES


/////////////////////////////////////////FUNÇÃO DE EXIBIÇÃO DE PONTO 1
void exibe_ponto1(){
  y=contpontos1;
  lc.setDigit(0,0,(y/10),false);
  lc.setDigit(0,1,(y%10),false);
  delay(delaytime);
}  
  
/////////////////////////////////////////FUNÇÃO DE EXIBIÇÃO DE PONTO 2

void exibe_ponto2(){
  y=contpontos2;
  lc.setDigit(0,2,(y/10),false);
  lc.setDigit(0,3,(y%10),false);
  delay(delaytime);
}

/////////////////////////////////////////FUNÇÃO DE EXIBIÇÃO DO CRONOMETRO

void exibe_crono(unsigned long d, unsigned long e){
  lc.setDigit(0,4,(d/10),false);
  lc.setDigit(0,5,(d%10),false);
  lc.setDigit(0,6,(e/10),false);
  lc.setDigit(0,7,(e%10),false);
  
}

/////////////////////////////////////////////FUNCAO PISCA PONTOS 
void pisca_pontos(){
  if(!a){
    lc.setChar(0,0,' ',false);
    lc.setChar(0,1,' ',false);
    lc.setChar(0,2,' ',false);
    lc.setChar(0,3,' ',false);
  }else{
    exibe_ponto1();                   
    exibe_ponto2();                   
  }
  a=!a;
  loop_pisca++;
  if(loop_pisca==6){
    Timer1.stop();
    a=0;
    loop_pisca=0;
    bloq_pisca=1;    
  } 
}   

/////////////////////////////////////////////FUNCAO PISCA PONTOS 1
void pisca_pontos1(){
  if(!a){
    lc.setChar(0,0,' ',false);  
    lc.setChar(0,1,' ',false);
  }else{
    exibe_ponto1();                   
  }
  a=!a;  
  loop_pisca++;
  if(loop_pisca==6){
    Timer1.stop();
    a=0;
    loop_pisca=0;
    bloq_pisca=1;    
  } 
}     

/////////////////////////////////////////////FUNCAO PISCA PONTOS 2
void pisca_pontos2(){
  if(!a){
    lc.setChar(0,2,' ',false);  
    lc.setChar(0,3,' ',false);
  }else{
    exibe_ponto2();                   
  }
  a=!a;
  loop_pisca++;
  if(loop_pisca==6){
    Timer1.stop();
    a=0;
    loop_pisca=0;
    bloq_pisca=1;    
  } 
}

/////////////////////////////////////////////FUNCAO PISCA CRONO 
void pisca_crono(){
  if(!a){
    digitalWrite(alarm, HIGH);
    lc.setChar(0,4,' ',false);
    lc.setChar(0,5,' ',false);
    lc.setChar(0,6,' ',false);
    lc.setChar(0,7,' ',false);
  }else{    
    if (prog && f){
      d=setmin-d;
      e=setseg-e;
      f=0;
    }
    exibe_crono(d, e);
  }
  a=!a;
  loop_pisca++;
  if(loop_pisca==8){
    digitalWrite(alarm, LOW);
    exibe_crono(setmin, setseg);
    Timer1.stop();      
    a=0;
    loop_pisca=0;
    bloq_pisca=1;  
    f=1;  
  } 
}   

/////////////////////////////////////////////FUNCAO PISCA HORA 
void pisca_hora(){
  ajuste = 2; //CONFIG A VARIAVEL P/ INC/DEC DE HORA
  if(!a){
    digitalWrite(alarm, LOW);
    lc.setChar(0,4,' ',false);
    lc.setChar(0,5,' ',false);
  }else{    
    exibe_crono(hora, minuto);
  }
  a =! a;
  if(loop_pisca2 == 1){
    loop_pisca2 = 0;
    digitalWrite(alarm, HIGH);
    exibe_crono(hora,minuto);
    Timer1.stop();      
    a = 0;
    if(ajuste_relogio){
      Timer1.initialize(300000); 
      Timer1.attachInterrupt(pisca_minuto);
    }else ajuste = buffer_ajuste;    
  } 
}

/////////////////////////////////////////////FUNCAO PISCA MINUTO 
void pisca_minuto(){
  ajuste = 3; //CONFIG A VARIAVEL P/ INC/DEC DE MINUTO
  if(!a){
    digitalWrite(alarm, LOW);
    lc.setChar(0,6,' ',false);
    lc.setChar(0,7,' ',false);
  }else{    
    exibe_crono(hora, minuto);
  }
  a =! a;
  if(loop_pisca2 == 1){
    loop_pisca2 = 0;
    digitalWrite(alarm, HIGH);
    exibe_crono(hora,minuto);
    Timer1.stop();      
    a = 0;
    if(ajuste_relogio){
      Timer1.initialize(300000); 
      Timer1.attachInterrupt(pisca_hora);
    }else ajuste = buffer_ajuste;
  } 
}

/////////////////////////////////////////////FUNCAO MODO CRONOP
void modo_cronoP(){
  if(!a){
    lc.setChar(0,4,' ',false);
    lc.setChar(0,5,' ',false);
    lc.setChar(0,6,' ',false);
    lc.setChar(0,7,' ',false);
  }else{
    lc.setChar(0,4,'P',false);                  
  }
  a=!a;
  loop_pisca++;
  if(loop_pisca==8){
    exibe_crono(setmin, setseg);
    T.SetTimer(0, setmin, setseg);
    Timer1.stop();      
    a=0;
    loop_pisca=0;
    bloq_pisca=1;    
  } 
}   

/////////////////////////////////////////////FUNCAO MODO CRONONP
void modo_cronoNP(){
  if(!a){
    lc.setChar(0,4,' ',false);
    lc.setChar(0,5,' ',false);
    lc.setChar(0,6,' ',false);
    lc.setChar(0,7,' ',false);
  }else{
    lc.setChar(0,4,'n',false);
    lc.setChar(0,5,'P',false);                  
  }
  a=!a;
  loop_pisca++;
  if(loop_pisca==8){
    exibe_crono(setmin, setseg);
    T.SetTimer(0, setmin, setseg);
    Timer1.stop();      
    a=0;
    loop_pisca=0;
    bloq_pisca=1;    
  } 
} 

/////////////////////////////////////////////FUNÇÃO ATUALIZA CONTPONTOS
void atualizacontpontos(){
  contpontos1 = bufpontos1;
  contpontos2 = bufpontos2;
}

/////////////////////////////////////////////FUNÇÃO ATUALIZA BUFPONTOS
void atualizabufpontos(){
  bufpontos1 = contpontos1;
  bufpontos2 = contpontos2;
}
/*
/////////////////////////////////////////////FUNÇÃO ATUALIZA CRONOMETRO
void atualizacrono(){
  min = bufmin;
  seg = bufseg;
}

/////////////////////////////////////////////FUNÇÃO ATUALIZA BUFFER CRONOMETRO
void atualizabufcrono(){
  bufmin = min;
  bufseg = seg;
}
*/
/////////////////////////////////////////FUNÇÃO DE GRAVAÇÃO DO CONTROLE REMOTO 1
void grava_tx1(){
  n=0;  
  for (int nL = 0; nL<=23; nL++) { 
      EEPROM.write(nL, 0);
  }  
  lc.clearDisplay(0);
  contpontos1 = 0;
  exibe_ponto1();
  bloq_pisca=0;
  while(!n){
    for(q=0;q<=5;q++){
      while(mySwitch.available() == 0);      
      if (mySwitch.available()) {
        controle1[q] = mySwitch.getReceivedValue(); 
        eeprom_escreveLong((q*4), controle1[q]); 
        contpontos1 = (q+1);
        exibe_ponto1();     
        mySwitch.resetAvailable();
        mySwitch.disableReceive();
        TIMSK2 = 0B00000001;        
      }          
    } 
    if(q == 6)n=1; 
    contpontos1 = 0;
    contpontos2 = 0;
    exibe_ponto1();         //ESCREVE PONTO DO TIME 1 NO DISPLAY
    exibe_ponto2();         //ESCREVE PONTO DO TIME 2 NO DISPLAY
    if((d || e) > 0 ){
      exibe_crono(d, e); 
    }else{
      exibe_crono(setmin, setseg);
    }   
  }  
}
 
/////////////////////////////////////////FUNÇÃO DE GRAVAÇÃO DO CONTROLE REMOTO 2
void grava_tx2(){
  digitalWrite(alarm, HIGH);
  n=0;  
  for (int nL = 24; nL<=47; nL++) { 
      EEPROM.write(nL, 0);
  }  
  lc.clearDisplay(0);
  contpontos2 = 0;
  exibe_ponto2();
  bloq_pisca=0;
  while(!n){
    for(q=6;q<=11;q++){
      while(mySwitch.available() == 0);      
      if (mySwitch.available()) {
        controle2[q] = mySwitch.getReceivedValue(); 
        eeprom_escreveLong((q*4), controle2[q]); 
        contpontos2 = (q-5);
        exibe_ponto2();     
        mySwitch.resetAvailable();
        mySwitch.disableReceive();
        TIMSK2 = 0B00000001;        
      }          
    } 
    if(q == 12){
      n=1;
      q=0;
    } 
    contpontos1 = 0;
    contpontos2 = 0;
    exibe_ponto1();         //ESCREVE PONTO DO TIME 1 NO DISPLAY
    exibe_ponto2();         //ESCREVE PONTO DO TIME 2 NO DISPLAY
    if((d || e) > 0 ){
      exibe_crono(d, e); 
    }else{
      exibe_crono(setmin, setseg);
    }   
  }  
}

/////////////////////////////////////////////FUNCAO GRAVA NA EEPROM 1 BYTE
void eeprom_escreve(int endereco, int val) {
   EEPROM.write(endereco, val);
}

/////////////////////////////////////////////FUNCAO LÊ NA EEPROM 1 BYTE
int eeprom_le(int endereco){
  int dado = EEPROM.read(endereco);   
  return (dado);
} 
   
/////////////////////////////////////////////FUNCAO GRAVA NA EEPROM 4 BYTES
void eeprom_escreveLong(int endereco, long val) {
   byte four = (val & 0xFF);
   byte three = ((val >> 8) & 0xFF);
   byte two = ((val >> 16) & 0xFF);
   byte one = ((val >> 24) & 0xFF);

   EEPROM.write(endereco, four);
   EEPROM.write(endereco + 1, three);
   EEPROM.write(endereco + 2, two);
   EEPROM.write(endereco + 3, one);
}

/////////////////////////////////////////////FUNCAO LÊ NA EEPROM 4 BYTES
long eeprom_leLong(int endereco) {
   long four = EEPROM.read(endereco);
   long three = EEPROM.read(endereco + 1);
   long two = EEPROM.read(endereco + 2);
   long one = EEPROM.read(endereco + 3);

   return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + 
   ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}



