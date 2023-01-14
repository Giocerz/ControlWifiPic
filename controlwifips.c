///////////////////////////////////////////////////////////////////////////
////                  WIFI UNIVERSAL IR CONTROLLER                     ////
////               Developed by Giovanni CaicedO, 2022                 ////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
////             Free use code for educational protections             ////
////             Use in commercial activities is not allowed           ////
///////////////////////////////////////////////////////////////////////////

#include <16F88.h>

#FUSES NOWDT                    //No Watch Dog Timer
#FUSES PUT                      //Power Up Timer
#FUSES NOBROWNOUT               //No brownout reset
#FUSES NOLVP                    //No low voltage prgming, B3(PIC16) or B5(PIC18) used for I/O
#FUSES CCPB3

#use delay(crystal=20000000)
#use rs232(baud=9600,parity=N,xmit=PIN_B1,rcv=PIN_B2,bits=8,stream=ESP,FORCE_SW,errors) //FORCE_SW
#include <stdio.h>
#include <string.h>

#define IR_Sensor PIN_A0
#define RELE_1    PIN_A1
#define LED_STATE PIN_A2
#define BTN_RST   PIN_B0

int8 cntRx=0, cntTx=0, timerIr;
int8 contTimer=0,contConsulta=0,consultaFlag=0, contReset=0;
int8 wifiFlag=0;
int8 redStatus=0;
int16 contServer=0;
int16 tiemposTrama = 0;

char IPD[]="+IPD,";
char STATUS[]="STATUS";
char OK[]="OK";
char FAIL[]="FAIL";
char vrx[85];
char vIr[80];
char vname[20];
char vpass[20];
char dname[]="Pro_Sistemas";
char dpass[]="ProS2022";

int8 get_entrecoma(int8 posini, char *v1[]);
void nuevaRed(short op);
char get_cmd();
int8 vCompareNB(char *v1[], char *v2[], int8 nBytes);
void waitClearBufferUarQ2Wt(int16 time);
void activarServidor();
void crearRed(char name[], char pass[]);
void conectarRed(char name[], char pass[]);
void pwmSoft(short en);
void ir_send(char codigo[]);
void ir_read(void);
void transmitir(void);
void grabar(void);
void replyWifi(void);
void procesoPrincipal(void);

#INT_TIMER1
void  TIMER1_isr(void) 
{
   contConsulta++;
   contTimer++;
   contServer++;
   if(contTimer==9 && redStatus==0){
      output_toggle(LED_STATE);
      contTimer=0;
   }else if(contTimer==39 && redStatus==1){
      output_toggle(LED_STATE);
      contTimer=0;
   }
   if(contConsulta==134){
      consultaFlag=1;
      contConsulta=0;
   } 
   if(contServer==443){
      activarServidor();
      contServer=0;
   }
   //RESETEO SERVIDOR
   if(input(BTN_RST)){
      contReset++;
   }else{
      contReset=0;
   }
   if(contReset==95){
      for(int cn=0;cn<6;cn++){delay_ms(50);output_toggle(LED_STATE);}
      output_high(LED_STATE);
      crearRed(dname,dpass);
      activarServidor();
      for(cn=0;cn<6;cn++){delay_ms(50);output_toggle(LED_STATE);}
      output_low(LED_STATE);
      contReset=0;
   }
   
}


void main()
{                                                                                                                                                                      
   setup_ccp1(CCP_PWM);
   set_pwm1_duty(0); 
   setup_timer_2(T2_DIV_BY_4,32,1);      
   setup_timer_1(T1_INTERNAL|T1_DIV_BY_4);      //52,4 ms overflow
   enable_interrupts(INT_TIMER1);
   enable_interrupts(GLOBAL);
   while(TRUE)
   {  
      if(consultaFlag==1 && wifiFlag == 0){
         puts("AT+CIPSTATUS");
         consultaFlag=0;
      }
      if(kbhit(ESP)>0){
         if(wifiFlag==0){
            procesoPrincipal();
         }else{
            replyWifi();
         }
      }
   }
}

int8 get_entrecoma(int8 posini, char *v1[]){
   int8 jk,k1;
   for(jk=posini;jk<cntRx;jk++){
      if(vrx[jk]==','){
         for(k1=jk+1;k1<cntRx;k1++){
            if(vrx[k1]==','){
               *v1 = 0;
               return k1;
            }
            *v1 = vrx[k1];
            v1=v1+1;
         }
         *v1 =0;
      }
   }
}

void nuevaRed(short op){
   int8 posclave,posip;
   posclave = get_entrecoma(7,&vname);
   posip = get_entrecoma(posclave,&vpass);
   if(op){
      crearRed(vname,vpass);
   }else{
      conectarRed(vname,vpass);
   }  
}

char get_cmd(){
   char cmd1;
   int8 jk;
   cmd1=0;
   for(jk=6;jk<cntRx;jk++){
      if(vrx[jk]==':'){
         cmd1 = vrx[jk+1];
         return cmd1;
      }
   }
   return cmd1;
}

int8 vCompareNB(char *v1[], char *v2[], int8 nBytes){
   int8 j;
   for(j=0;j<nBytes;j++){
      if(*v1!=*v2){return 0;}
      v1=v1+1;
      v2=v2+1;
   }
   return 1;
}

void waitClearBufferUart(int16 time){
   for(int16 i=0;i<time;i++){
      if( kbhit()){getc();}
      delay_us(500);
      if( kbhit()){getc();}
      delay_us(500);
   }
}

void activarServidor(){
   puts("AT+CIPMUX=1");
   waitClearBufferUart(200);
   puts("AT+CIPSERVER=1,2020");
   waitClearBufferUart(200);
   cntRx=0;
}

void crearRed(char name[], char pass[]){
   puts("AT");
   waitClearBufferUart(100);
   puts("AT+CWQAP");
   waitClearBufferUart(200);
   puts("AT+CWMODE=2");
   waitClearBufferUart(200);
   fprintf(ESP,"AT+CWSAP=\"%s\",\"%s\",3,4\r\n",name,pass);
   waitClearBufferUart(200);
   puts("AT+RST");
   waitClearBufferUart(3000);
}

void conectarRed(char name[], char pass[]){
   wifiFlag = 1;
   puts("AT");
   waitClearBufferUart(100);
   puts("AT+CWMODE=1");
   waitClearBufferUart(200);
   fprintf(ESP,"AT+CWJAP=\"%s\",\"%s\"\r\n",name,pass);
   return;
}

void ir_send(char codigo[])         
{             
   int8 tk = 0; 
   tiemposTrama = 0;       
   //BIT DE START
   set_pwm1_duty(16);                                                                                                                             
   tiemposTrama = (int16)codigo[tk]*38; 
   delay_us(tiemposTrama);                                       
   tk++;
   set_pwm1_duty(0); 
   tiemposTrama = (int16)codigo[tk]*38; 
   delay_us(tiemposTrama); 
   tk++;    
   //DATOS
   while(tk<80){
      if(!codigo[tk]){break;}
      set_pwm1_duty(16);
      tiemposTrama = (int16)codigo[tk]*34;
      delay_us(tiemposTrama);
      tk++;
      if(!codigo[tk]){break;}
      set_pwm1_duty(0);
      tiemposTrama = (int16)codigo[tk]*34; 
      delay_us(tiemposTrama); 
      tk++;
   }
   set_pwm1_duty(0);
   
}

void ir_read(void){
   //BIT START
   while(!input(IR_Sensor))
   {
      timerIr++;
      delay_us(38);
      if(timerIr>250){timerIr=0; return;}
   }
   vIr[cntTx] = timerIr;
   cntTx++;
   timerIr = 0;
     
   while(input(IR_Sensor))
   {
      timerIr++;
      delay_us(38);
      if(timerIr>250){timerIr=0; return;}
   }
   vIr[cntTx] = timerIr;
   cntTx++;
   timerIr = 0;
   
   //TRAMA
   while(cntTx<74)
   {
      while(!input(IR_Sensor))
      {
         timerIr++;
         delay_us(34);
         if(timerIr>250){timerIr=0; return;}
      }
      vIr[cntTx] = timerIr;
      cntTx++;
      timerIr = 0;
     
      while(input(IR_Sensor))
      {
         timerIr++;
         delay_us(34);
         if(timerIr>250){timerIr=0; return;}
      }
      vIr[cntTx] = timerIr;
      cntTx++;
      timerIr = 0;
   }
}


void transmitir(void){
   int8 cntTr;
   cntTr = get_entrecoma(7,&vIr);
   ir_send(vIr);
}

void grabar(void){
   int32 contGrabar=0;
   output_high(RELE_1);
   timerIr = 0;
   cntTx = 0;
   while(input(IR_Sensor)){contGrabar++; if(contGrabar==1200000){break;}};
   ir_read();
   if(cntTx>10){
      output_low(RELE_1);
      
      for(int8 i = 0;i<cntTx;i++){
         if(vIr[i] == ',' || vIr[i] == 10 || vIr[i] == 13){
            vIr[i] += 1;
         }
      }
      
      fprintf(ESP,"AT+CIPSEND=%c,%d\r\n",vrx[5],cntTx);
      waitClearBufferUart(200);
      for(int8 j=0; j<cntTx; j++){
         putc(vIr[j]);
      }
   }
   cntTx=0;
   output_low(RELE_1);
}

void replyWifi(void){
   disable_interrupts(GLOBAL);
   char Dato;
   Dato = getc();
   if(Dato==13){
      if(cntRx<2){cntRx=0; return;}
      if(vCompareNB(&OK, &vrx, 2)==1){
         puts("AT+CIPSTA=\"192.168.0.100\"");
         waitClearBufferUart(200);
         activarServidor();
         for(int cn=0;cn<6;cn++){delay_ms(50);output_toggle(LED_STATE);}
         wifiFlag=0;
      }
      if(vCompareNB(&FAIL, &vrx, 4)==1){
         crearRed(dname,dpass);
         activarServidor();
         for(int cn=0;cn<6;cn++){delay_ms(50);output_toggle(LED_STATE);}
         delay_ms(500);
         for(cn=0;cn<6;cn++){delay_ms(50);output_toggle(LED_STATE);}
         wifiFlag=0;
      }
      cntRx=0;
   }else if(Dato!=10){
      vrx[cntRx]=Dato;
      cntRx++;
      if(cntRx>=84){cntRx=0;}
   }
   enable_interrupts(GLOBAL);
}

void procesoPrincipal(void){
   disable_interrupts(GLOBAL);
   char Dato,cmd;
   Dato = getc();
   if(Dato==13){
      if(cntRx<5){cntRx=0; return;}
      if(vCompareNB(&IPD, &vrx, 2)==1){
         cmd = get_cmd();
         if(cmd=='T'){
            transmitir();
         }
         else if(cmd=='S'){
            grabar();
         }
         else if(cmd=='R'){
            nuevaRed(1); 
            activarServidor();
            for(int cn=0;cn<6;cn++){delay_ms(50);output_toggle(LED_STATE);}
         }else if(cmd=='Z'){
            nuevaRed(0);
            return;
         }
      }
      if(vCompareNB(&STATUS, &vrx, 5)==1){
         cmd = get_cmd();
         if(cmd=='3'){
            redStatus=1;
         }else{
            redStatus=0;
         }
      }
      cntRx=0;
   }else if(Dato!=10){
      vrx[cntRx]=Dato;
      cntRx++;
      if(cntRx>=84){cntRx=0;}
   }
   enable_interrupts(GLOBAL);
}
