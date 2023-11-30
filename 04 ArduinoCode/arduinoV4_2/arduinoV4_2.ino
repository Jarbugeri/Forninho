
// Title      : Forninho REFLOW
// Project    : 
//--------------------------------------------------------------------------------------------------------
// File       : serialthermocoupleV3_1.ino
// Author     : José Augusto Arbugeri (josearbugeri@gmail.com)
// Company    : Instituto de eletrônica de Potência, Universidade Federal de Santa Catarina 
//---------------------------------------------------------------------------------------------------------
// Este programa realiaza o controle de temperatura de um forno eletrico.
//                    
//---------------------------------------------------------------------------------------------------------
// Copyright (c) Instituto de eletrônica de Potência, Universidade Federal de Santa Catarina 
//---------------------------------------------------------------------------------------------------------
// Revisions  :
// Date        Version  Author   Description
// 28/06/2021    1.0     José     Created: comunicação entre APP e arduino 
// 01/07/2021    2.0     José     Adicionado máquina de estados, Perfils e timer
// 04/07/2021    2.1     José     Adicionado mais variaveis ao vetor de comunicação (Temp, Ref, ON/off , etc.)
// 01/10/2021    2.2     José     Adicionado PID e PWM (1 Hz), finalizando o programa.
// 09/10/2021    2.3     José     Adicionado botões,led , buzzer.
// 04/12/2021    3.1     José     Versão Final, ajuste de controladores e teste de rampas maxima
//----------------------------------------------------------------------------------------------------------

//Includes
#include "max6675.h"

//#define ForninhoZe 1
#ifdef ForninhoZe
  // Define os pinos para a: IHM
  #define PUSH_START  A2
  #define PUSH_STOP   A3
  #define LED_ON_OFF  A6
  #define PWM         9
  #define BUZZER      4
  #define BUZZER_EN   5
  #define set_buzzer()   OCR0A = 50;
  #define clear_buzzer() OCR0A = 0;
  // Define os pinos para o: SPI  
  #define thermoDO   12
  #define thermoDI   11 //Not used
  #define thermoCS   10
  #define thermoCLK  13     

#else
  // Define os pinos para a: IHM
  #define PUSH_START  10
  #define PUSH_STOP   11
  #define LED_ON_OFF  12
  #define PWM         9
  #define BUZZER      2
  #define set_buzzer()   digitalWrite(BUZZER, HIGH);
  #define clear_buzzer() digitalWrite(BUZZER, LOW );
  // Define os pinos para o: SPI  
  #define thermoDO   4
  #define thermoCS   5
  #define thermoCLK  6
#endif

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

// Definicões do controlador PID

   float A = 1.559;//0.71365;//1.559 ;
   float B = 0.9852;//0.9865; //0.9852;
   float C = 0.9863;//0.9896; //0.9863;
   float D = 0.6604;//0.7619; //0.6604;

   float a0 = A;
   float a1 = (-A)*(B+C);
   float a2 = A*B*C;
   float b0 = 0;
   float b1 = (-1.0) * (1+D);
   float b2 = D;
   

typedef struct{
  //signals
  float x2;
  float x1;
  float x0;
  float y2;
  float y1;
  float y0;
  //coeficientes
  float a2;
  float a1;
  float a0;
  float b2;
  float b1;
  float b0;
} PID_t;

#define Transport_Delay  2

typedef struct{
  float ref;
  float input_raw;
  float input_filterd;
  float erro;
  float duty;
  //PID with filter
  PID_t PID;
  //Limitador PID
  float kfeed; //sqrt(95.2381f*278.1641f);
  float efeed0;
  float efeed1;
  //Modelo preditor smith
  PID_t Modelo;
  int Buffer_count;
  float Buffer[Transport_Delay];
  float yd;
  
} Controle_t;

#define MILLS_TO_SEC 0.001

//PreHeat oven before beggin
int delay_count = 0;
#define Initial_Delay 14  // Segundo 
  

// Define os pinos para a: IHM

boolean t_butStart = 0,   
        t_butStop  = 0; //Flags para armazenar o estado dos botões

// Maquina de estados

#define WAITING       0
#define PREHEATOVEN   1
#define RAMPTOSOAK    2
#define PREHEAT       3
#define RAMPTOPEAK    4
#define REFLOW        5
#define COOLING       6
#define RESETPROFILE  7
int     STATE    =    0;

// Variaveis de tempo

long  StartTime; // Tempo em milisegundos
float Time;      // Tempo em segundos

// Tempo de atualização do PWM

float Delta               = 0.25 ; 
float RefTemp             = 0.0;

// Filtro media movel

#define MA_LENGHT  4
typedef struct {
  int   LENGHT;
  float INV_LENGHT;
  int   index;
  int   count;
  float acc;
  float buffer[MA_LENGHT];
}moving_average_t;

int START   = 0;  // START = 1 (Begin),               START = 0 (Waiting)
int RESET   = 0;  // RESET = 1 (Go to STATE WAITING), RESET = 0 (Do nothing)

typedef struct{
  boolean enable;
  int beep_times;
  int beep_count;
  int beep_duration;
  int beep_times_count;
} Buzz_t;
  
typedef struct{
  float RampUpRate;
  float PreheatSoakInitTemp;
  float PreheatSoakEndTemp;
  float PreheatSoakTime;
  float PreheatSoakRate;
  float RampToReflow;
  float ReflowTemp;
  float ReflowTime;
  float CoolingRate;
} Perfil_t;

typedef struct{
  float timer;
  float reflow_Temp_raw;
  float reflow_Temp_filtered;
  float reflow_ref;
  float reflow_duty;
  uint32_t reflow_status;
} reflow_t;

#define PWM_COUNTER 15625
#define set_pwm_duty(duty)  OCR1A  = (uint16_t)(duty * PWM_COUNTER);

/* structs defines */
Perfil_t   ChosenProfile = {0};
reflow_t   reflow        = {0};
Controle_t Controle      = {0};
Buzz_t     Buzzer        = {0};
moving_average_t     ma_filter = {MA_LENGHT, (float) 1.0 / MA_LENGHT};

/***** Definicões das funções ****/
/* Serial communication functions*/
void serialEvent();
/* State machine functions*/
inline void next_state(bool condition, int next_state);
inline void reset_state(bool condition);  
void StateMachine();
/* Timer config functions*/
void Timer1Config();
/* GPIO config functions*/
void GPIO();
/* Control functions*/
void PID_filtro_smith(Controle_t * Controle);  //PID com filtro e preditor smith
void setBuffer(Controle_t * Controle, float InitValue);
/* PID functions*/
void setPID(PID_t * pid, float input, float output);
void runPID(PID_t * pid, float erro);
/* Buttons functions*/
void readButtons();
/* Buzzer functions*/
void Buzzerpattern();
/* moving average functions*/
float moving_average(moving_average_t * ma_filter, float input);
void set_moving_average(moving_average_t * ma_filter, float InitValue);

void setup() {
  
  //<! Referencia: https://www.nxp.com/docs/en/supporting-information/Reflow_Soldering_Profile.pdf
  //<! Init defaut profile
  
  ChosenProfile.RampUpRate          =    1.0;        // Max 3º/sec
  ChosenProfile.PreheatSoakInitTemp =  100.00;        // 100-183 ºC
  ChosenProfile.PreheatSoakEndTemp  =  150.00;        // 100-183 ºC
  ChosenProfile.PreheatSoakTime     =   90.00;        // 60 to 120
  ChosenProfile.PreheatSoakRate     = (ChosenProfile.PreheatSoakEndTemp - ChosenProfile.PreheatSoakInitTemp)/ChosenProfile.PreheatSoakTime;
  ChosenProfile.RampToReflow        =    1.0;        // Max 3º/sec
  ChosenProfile.ReflowTemp          =  230.00;        // 150 
  ChosenProfile.ReflowTime          =   30.00;        // 45 to 70
  ChosenProfile.CoolingRate         =    1.80;         // MAX 6º/sec

  Buzzer.enable           = true;
  Buzzer.beep_times       = 5;  
  Buzzer.beep_count       = 0;
  Buzzer.beep_times_count = 0;
  Buzzer.beep_duration    = 1;  

  Controle.PID.a0 = a0;
  Controle.PID.a1 = a1;
  Controle.PID.a2 = a2;
  Controle.PID.b0 = b0;
  Controle.PID.b1 = b1;
  Controle.PID.b2 = b2;

  Controle.kfeed = 2.0;

  Controle.Modelo.a0 = 0.06381;
  Controle.Modelo.a1 = 0.12760;
  Controle.Modelo.a2 = 0.06381;
  Controle.Modelo.b0 = 0;
  Controle.Modelo.b1 = -1.9430;
  Controle.Modelo.b2 = 0.94380;

  ma_filter.LENGHT = MA_LENGHT;
  ma_filter.INV_LENGHT = (float) 1.0 / MA_LENGHT;

  // Inicializa as portas utilizadas 
  GPIO();

  // Configura o timer usado para PWM e interrupção
  Timer1Config();

  // Inicializa a comunicação serial
  Serial.begin(115200);

  // Reseta variaveis
  setPID(&Controle.PID          , 0.0 , 0.0);
  setPID(&Controle.Modelo       , 0.0 , 0.0);
  setBuffer(&Controle           , 0.0);
  set_moving_average(&ma_filter , 0.0);
   
  START = 0;
  RESET = 0;
  set_pwm_duty(0.0);

}

void loop() {

  readButtons();
 
}

void serialEvent() {
  while (Serial.available()) {
  
    char CharSerialRX = (char) Serial.read();

    if(CharSerialRX == (char) 'D'){
        size_t struct_size = sizeof reflow;
        char reflow_buffer[struct_size];
        
        memcpy(reflow_buffer, &reflow, struct_size);
        
        for (int i = 0; i < struct_size; i++){    
          Serial.print(reflow_buffer[i]);
        }
    }

    if(CharSerialRX == (char) 'o'){
      Serial.print("OK");
      set_buzzer();
      delay(100);
      clear_buzzer(); 
    }
    
    if(CharSerialRX == (char) '1'){
        START = 1;
    }
    if(CharSerialRX == (char) 's'){
        RESET = 1;
        START = 0;
    }

    if(CharSerialRX == (char) 'R'){
      
      size_t struct_size = sizeof ChosenProfile;
      char profile_buffer[struct_size];
      
      memcpy(profile_buffer, &ChosenProfile, struct_size);
      
      for (int i = 0; i < struct_size; i++){    
        Serial.print(profile_buffer[i]);
      }
    }

    
  }
}

inline void reset_state(bool condition){
  if(condition){
    Buzzer.enable = true;
    STATE = WAITING;
    RESET = 0;
    START = 0; 
  }    
}

inline void next_state(bool condition, int next_state){
  if(condition){
    STATE     = next_state;
    StartTime = ((float) millis())*MILLS_TO_SEC;   
  }    
}

void StateMachine(){
  switch (STATE){
    case WAITING:
      reset_state(RESET == 1);
         
      RefTemp = Controle.input_raw;
      
      next_state(START == 1, STATE + 1);
    break;

    case PREHEATOVEN:
      reset_state(RESET == 1);

      RefTemp = Controle.input_raw;   
      setPID(&Controle.PID, 0.0, 0.5);
      setBuffer(&Controle , 1.0); 
      
      next_state((Time - StartTime) >= Initial_Delay , STATE + 1);
    break;
    
    case RAMPTOSOAK:
      reset_state(RESET == 1);
      
      RefTemp      = RefTemp + Delta*ChosenProfile.RampUpRate;
      
      next_state(RefTemp >= ChosenProfile.PreheatSoakInitTemp, STATE + 1);
    break;
    
    case PREHEAT:
      reset_state(RESET == 1);

      RefTemp      = RefTemp + Delta*ChosenProfile.PreheatSoakRate;
      
      next_state(( Time - StartTime ) >= ChosenProfile.PreheatSoakTime, STATE + 1);
    break;
    
    case RAMPTOPEAK:
      reset_state(RESET == 1);
      
      RefTemp      = RefTemp + Delta*ChosenProfile.RampToReflow;
      
      next_state(RefTemp >= ChosenProfile.ReflowTemp, STATE + 1);
    break;

    case REFLOW:
      reset_state(RESET == 1);

      RefTemp = ChosenProfile.ReflowTemp;
      
      next_state(( Time - StartTime ) >= ChosenProfile.ReflowTime, STATE + 1);
    break;
    
    case COOLING:
      reset_state(RESET == 1);

      RefTemp      = RefTemp - Delta*ChosenProfile.CoolingRate;
      Buzzerpattern();
    
      next_state(RefTemp <= 30, STATE + 1);
    break;

    case RESETPROFILE:
      reset_state(RESET == 1);
      
      START = 0;
      RESET = 1;

      next_state(true, WAITING);      
    break;     
  }
}

void Timer1Config(){
  
  // clock frequency = 16MHz 
  
  pinMode(PWM,OUTPUT);
  
  cli();
  
  //Timer 1 (interrupt cada 0.25s)
  TCCR1A = 0; // Reset 
  TCCR1B = 0; // Reset
  
  delay(1);
  
  TCCR1A |= _BV(COM1A1) | _BV(WGM11);      
  TCCR1B |= _BV(WGM13) | _BV(WGM12) | _BV(CS12);
  TIMSK1 |= _BV(OCF1B);
  ICR1   = (uint16_t) PWM_COUNTER; // 0.25 Hz
  OCR1A  = 0;
  OCR1B  = 0;   
  sei();
}

ISR(TIMER1_COMPB_vect){

  // Zera o flag interrupcao
  //Tratador
 
  Time = ( (float) millis() )*MILLS_TO_SEC;
  
  StateMachine();
  Controle.ref = RefTemp;

  //Disable all interrupts 
  cli();
  Controle.input_raw = (float) thermocouple.readCelsius();
  sei();

  Controle.input_filterd = moving_average(&ma_filter, Controle.input_raw);

  if (START == 1){
    if (STATE <= PREHEATOVEN){
      Controle.duty = 1.0;
    }else{
      PID_filtro_smith(&Controle); 
    }      
  }else{
    Controle.duty = 0.0;
    setPID(&Controle.PID, 0.0, 0.0);
    setBuffer(&Controle ,0.0);
  }

  //!<Atualiza moduladora

  set_pwm_duty(Controle.duty);
  
  //!<Atualiza dados
  reflow.timer                = Time;  
  reflow.reflow_Temp_raw      = Controle.input_raw;
  reflow.reflow_Temp_filtered = Controle.input_filterd;
  reflow.reflow_ref           = Controle.ref;
  reflow.reflow_duty          = Controle.duty;
  reflow.reflow_status        = STATE << 2 | RESET << 1| START;

  digitalWrite(LED_ON_OFF,START); 

  
}

void PID_filtro_smith(Controle_t * Controle){


  // Computa o erro
  //Controle->erroerro = Controle->ref - ( (Controle->input - Controle->yd)+ Controle->Modelo.y0 ) ;
  Controle->erro = Controle->ref - Controle->input_filterd;
  
  runPID(&Controle->PID, Controle->erro);
  
  // Anti windup
  //// Saturador 

  Controle->efeed1 = Controle->efeed0;

  float y0_sat;
  
  if (Controle->PID.y0 >= 1.0){
    y0_sat = 1.0;
    Controle->efeed0 = Controle->kfeed * (Controle->PID.y0 - Controle->PID.y0); // Kfeed = sqrt(Ki*Kd)  
  }else if(Controle->PID.y0 <= 0.0){
    y0_sat = 0.0f;
    Controle->efeed0 = Controle->kfeed * (Controle->PID.y0 - Controle->PID.y0); // Kfeed = sqrt(Ki*Kd)    
  }else{
    y0_sat = Controle->PID.y0;
    Controle->efeed0 = 0.0;
  }

  //!<Preditor smith

  runPID(&Controle->Modelo, Controle->PID.y0);

  //!<Memoria
  Controle->Buffer[Controle->Buffer_count] = Controle->Modelo.y0;
  Controle->Buffer_count ++;

  if(Controle->Buffer_count == Transport_Delay){
    Controle->Buffer_count = 0;
  }

  Controle->yd = Controle->Buffer[Controle->Buffer_count];

  //!<Output
  Controle->duty = y0_sat;

}

void GPIO(){
  
  // Declara os pinos HHM, para os pushbuttom pull-up interno ativo
  
  pinMode(PUSH_START , INPUT_PULLUP);
  pinMode(PUSH_STOP  , INPUT_PULLUP);
  pinMode(LED_ON_OFF , OUTPUT);

  pinMode(BUZZER      , OUTPUT);
  digitalWrite(BUZZER , LOW);

  pinMode(PWM        , OUTPUT);

  // zera as saidas na inicialização

  digitalWrite(LED_ON_OFF, LOW);

  digitalWrite(PWM       , LOW);
}

void readButtons()
{
   if(!digitalRead(PUSH_START))    t_butStart  = true;
   if(!digitalRead(PUSH_STOP))     t_butStop   = true;

   if( digitalRead(PUSH_START) && t_butStart )
   {
       //Zera flag para tratamento da interrupção: start  
       t_butStart = false;

       RESET = 0;
       START = 1;
    
   } //end if START

   if( digitalRead(PUSH_STOP) && t_butStop )
   {
       //Zera flag para tratamento da interrupção: start  
       t_butStop = false;
       
       START = 0;
       RESET = 1;
    
   } //end if START

}

void Buzzerpattern(){
  
  if (Buzzer.enable) {
    if (Buzzer.beep_count == Buzzer.beep_duration){
      set_buzzer()
      Buzzer.beep_count = 0;
      Buzzer.beep_times_count++;
      if (Buzzer.beep_times == Buzzer.beep_times_count){
         Buzzer.enable = false;
         Buzzer.beep_times_count = 0;
         Buzzer.beep_count = 0;
      }
    }else{
      clear_buzzer()
      Buzzer.beep_count++;
    }
  }else{
    clear_buzzer()
  }
}

void setBuffer(Controle_t * Controle, float InitValue){
  int counter = 0;
  while(counter < Transport_Delay){
    Controle->Buffer[counter] = InitValue;
    counter ++;
  }
}

void set_moving_average(moving_average_t * ma_filter, float InitValue){
  int counter = 0;
  while(counter < ma_filter->LENGHT){
    ma_filter->buffer[counter] = InitValue;
    counter ++;  
  }
}

float moving_average(moving_average_t * ma_filter, float input){
  ////Perform average on sensor readings
  ma_filter->acc   = 0.0;
  ma_filter->count = 0;

  ma_filter->buffer[ma_filter->index] = input;

  while(ma_filter->count < ma_filter->LENGHT){
    ma_filter->acc = ma_filter->acc + ma_filter->buffer[ma_filter->count];
    ma_filter->count ++;
  }
  
  ma_filter->index = ma_filter->index + 1;

  if (ma_filter->index >= ma_filter->LENGHT) {ma_filter->index = 0;}

  return ma_filter->acc * ma_filter->INV_LENGHT;
}


void setPID(PID_t * pid, float input, float output){
  pid->x0 = input;
  pid->x1 = input;
  pid->x2 = input;
  pid->y0 = output;
  pid->y1 = output;
  pid->y2 = output;
}

void runPID(PID_t * pid, float erro){

  pid->x2 = pid->x1;
  pid->x1 = pid->x0;
  pid->x0 = erro;
  pid->y2 = pid->y1;
  pid->y1 = pid->y0;                                            
  pid->y0 = pid->x0 * pid->a0 +
            pid->x1 * pid->a1 + 
            pid->x2 * pid->a2 -
            pid->y1 * pid->b1 -
            pid->y2 * pid->b2; 
  
}
