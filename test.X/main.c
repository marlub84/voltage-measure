/*
 * File:   main.c
 * Author: mario
 *
 * Created on 27. ?ervence 2017, 18:48
 */


#include <xc.h>
#include "mcc_generated_files/mcc.h"

//#include "config.h"



#define CALIB                   18000
#define CALM_LEVEL              627      // Value on 2.5 V on sonda
#define DA_LEVEL                231     // D/A Value on 2.5 V
#define DA_BUFFER               2
#define CALIB_L_LEVEL(value)        (CLAM_LEVEL - value)
#define CALIB_H_LEVEL(value)        (CLAM_LEVEL + value)
#define CALIB_COUNT             10
       

void analog_send(adc_channel_t);
void ProbeDelay(uint8_t);
void NormalWork(uint16_t *, uint8_t *, uint16_t *);
void AutoSet(adc_channel_t, uint16_t *, uint16_t *);
void DataSend(uint16_t);
void Calibrate(adc_channel_t);
void SendToSPI(uint8_t, uint8_t*);
void MinMax(adc_channel_t, uint8_t , uint16_t *, uint16_t *, uint16_t *);
void TimerTest();

// Global variable

uint8_t offset = 30;                  // rozmezi klidoveho stavu
uint16_t auto_offset;                   // for count clam state
uint8_t analog_count = 100;
uint8_t readDA = 233;
uint16_t calm_state[2];
bool filtr = false;

void main(void) {
    
    SYSTEM_Initialize();
    // MCU setting
    
    led_power_SetHigh();
    
    uint8_t probe_delay = 10;           // Time in ms
    uint16_t weld_detect = 63;         // different between clam state and weld level
    uint16_t level_detect[2];
    // End setting
    
    uint16_t adc_result;
    uint8_t sendH;
    uint8_t sendL;
    uint8_t readL;
    uint8_t readH;    
    uint8_t command;
    //uint16_t calm_state[2];
    uint8_t spiBuffer[2];
    
    
    calm_state[0] = 0;
    calm_state[1] = 0;
    
    
    command = 0;
    
    adc_channel_t analog;
    
    while(1){
        if (PIR1bits.RC1IF == 1){
            command = EUSART1_Read();
        }
        
        
        uint8_t i; 
        
        switch(command){
            case 1:
                // read 'analog_count' times of sonda input
                analog = sonda_out;  
                for(i = 0; i <= analog_count; i++){
                    analog_send(analog);
                    ProbeDelay(probe_delay);
                }
                command = 0;
                break;
            case 2:
                // read 'analog_count' times of OP 
                analog = op_out;
                for(i = 0; i <= analog_count; i++){
                    analog_send(analog);
                    ProbeDelay(probe_delay);
                }
                command = 0;
                break;
            case 3:
                // Change DAC value
                DA_out_SetLow();
                readDA = EUSART1_Read();
                SendToSPI(readDA, &spiBuffer);
                DA_out_SetHigh();
                command = 0;
                break;
            case 4:
                /* Send analog measure from OP
                 * if receive some data stop sending data
                 * 
                 */
                analog = op_out;
                
                for (;;){
                    if (PIR1bits.RC1IF == 1){
                        break;
                    }
                    analog_send(analog);
                    ProbeDelay(probe_delay);
                }
                command = 0;
                break;
            case 5:
                /* Send analog measure from sonda
                 * if receive some data stop measure
                 * 
                 */
                analog = sonda_out;
                
                for (;;){
                    if (PIR1bits.RC1IF == 1){
                        break;
                    }
                    analog_send(analog);
                    ProbeDelay(probe_delay);
                }
                command = 0;
                break;
            case 6:
                /* 
                 * Normal work
                 * 
                 *  */
                analog = op_out;               
                NormalWork(&weld_detect, &probe_delay, level_detect);
                command = 0;
                break;
            case 7:
                // Set the state of clam and send data back
                // Auto offset
                
                analog = op_out;
                AutoSet(analog, &weld_detect, level_detect);
                DataSend(level_detect[0]);
                DataSend(level_detect[1]);
                command = 0;
                break;
            case 8:
                // Set the state of clam and send data back
                
                analog = op_out;
                Calibrate(analog);
                
                command = 0;
                break;
            case 9:
                /* MCU setting 
                 * Write to MCU
                 *  */
                
                probe_delay = EUSART1_Read();           // 1 byte
                offset = EUSART1_Read();                // 1 byte
                analog_count = EUSART1_Read();          // 1 byte
                readL = EUSART1_Read();                 // 2 byte
                readH = EUSART1_Read(); 
                weld_detect = (readH << 8) + readL;
                
                readL = EUSART1_Read();
                if (readL == 'Y'){
                    filtr = true;
                }else filtr = false;
                
                //weld_ignition = EUSART1_Read();         // 2 byte
                //readDA = EUSART1_Read();                // 1 byte
                command = 0;
                break;
            case 10:
                /* MCU setting 
                 * Read from MCU
                 *  */
                EUSART1_Write(probe_delay);
                EUSART1_Write(offset);
                EUSART1_Write(analog_count);
                DataSend(weld_detect);
                EUSART1_Write(readDA);
                if (filtr){
                    EUSART1_Write("Y");
                }else EUSART1_Write("N");
                
                command = 0;
                break;
            case 11:
                // Set the state of clam and send data back
                TimerTest();
                
                command = 0;
                break;
        }
           
    }
    
    return;
}

void analog_send(adc_channel_t analog){
    /* 
     * 
     *  
     */
    
    uint16_t adc_result;
    uint8_t sendH;
    uint8_t sendL;
    adc_result = 0;
    adc_result = ADC_GetConversion(analog);
    __delay_us(25);
    DataSend(adc_result);
    
}

void ProbeDelay(uint8_t delay){
    uint8_t j;
    for (j = 0;j <= delay; j++){
        __delay_ms(1);
    }
}

void NormalWork(uint16_t *weld_detect, uint8_t *delay, uint16_t *level_detect){
    
    adc_channel_t analog = op_out;
    uint16_t result;
    uint16_t count_measure = 0;
    //uint8_t table[DA_BUFFER];
    bool weld = false;
    
    AutoSet(analog, weld_detect, level_detect);
    
    for(;;){
        // if receive some data 
        if (PIR1bits.RC1IF == 1){
            break;
        }
        
                
        // read analog input
        ProbeDelay(*delay);
        result = ADC_GetConversion(analog);
        DataSend(result);
        
        if (count_measure >= CALIB){
            // check if robot don't weld
            if (in_1_PORT){
                //Calibrate(analog);
                AutoSet(analog, weld_detect, level_detect);
                count_measure = 0;
            }
        }
        
        if ((result > calm_state[1]) && (in_1_PORT == LOW)){
            
            // Wait and check if robot have arc ignition
            ProbeDelay(*delay);
            result = ADC_GetConversion(analog);
            DataSend(result);
            if (result >= (level_detect[1])){
                // robot start weld

                led_weld_SetHigh();
                out_1_SetHigh();                    // send signal
                weld = true;
                while(weld){
                    if (!filtr){
                        //Filtr 20ms don`t control result
                        uint8_t count = 0;
                        while (count > 2){
                            ProbeDelay(*delay);
                            count++;
                        }
                        // End filtr
                        filtr = true;
                    }else ProbeDelay(*delay);
                    
                    result = ADC_GetConversion(analog);
                    DataSend(result);
                    if (result < (level_detect[1])){ 
                        weld = false;
                    }
                }
                led_weld_SetLow();
                out_1_SetLow();
                // End weld
            }         
        }else if ((result < calm_state[0]) && (in_1_PORT == HIGH)){
            // Wait and check if robot have arc ignition
            ProbeDelay(*delay);
            result = ADC_GetConversion(analog);
            DataSend(result);
            if (result <= (level_detect[1])){
                // robot start weld

                led_weld_SetHigh();
                out_1_SetHigh();                    // send signal
                weld = true;
                while(weld){
                    if (!filtr){
                        //Filtr 20ms don`t control result
                        uint8_t count = 0;
                        while (count > 2){
                            ProbeDelay(*delay);
                            count++;
                        }
                        // End filtr
                        filtr = true;
                    }else ProbeDelay(*delay);
                    
                    result = ADC_GetConversion(analog);
                    DataSend(result);
                    if (result > (level_detect[1])){ 
                        weld = false;
                    }
                }
                led_weld_SetLow();
                out_1_SetLow();
                // End weld
            }
        }
        count_measure++;
    }
}


void AutoSet(adc_channel_t analog, uint16_t *weld, uint16_t *detect){
    /*
     * Function to auto set the level 
     * of no welds
     *  */
    uint16_t min, max, tmp;
    uint16_t prumer = 0;
    analog = op_out;
    MinMax(analog, CALIB_COUNT, &min, &max, &prumer);
    
    if ((calm_state[0] == 0) || (calm_state[1] == 0)) {
        Calibrate(analog);
    }
    else if ((prumer <= calm_state[0]) || (prumer >= calm_state[1])){

        Calibrate(analog);
        analog = op_out;
    }
    
    MinMax(analog, CALIB_COUNT, &min, &max, &prumer);
    calm_state[0] = prumer - offset;
    detect[0] = calm_state[0] - *weld;    
    calm_state[1] = prumer + offset;
    detect[1] = calm_state[1] + *weld;
    
}

void DataSend(uint16_t data){
    
    uint8_t sendL, sendH;
    sendL = data;
    sendH = (data >> 8);
    EUSART1_Write(sendL);
    EUSART1_Write(sendH);
}

void Calibrate(adc_channel_t analog){
    
    uint16_t result, tmp;
    uint16_t min, max;
    uint16_t prumer;
    uint8_t diff_1 = 0;
    uint8_t diff_2 = 0;
    uint8_t table[DA_BUFFER];
    
    bool calib = true;
 
    analog = sonda_out;
    //take some measure
    MinMax(analog, CALIB_COUNT, &min, &max, &prumer);
    
    //result = (min + max) / 2;
    
    // Analysis result
    if (prumer == CALM_LEVEL){
        readDA = DA_LEVEL;
        SendToSPI(readDA, &table);
        calib = false;

    }else if (prumer < CALM_LEVEL){
        // resulsts is lower 2.5 V

        readDA = DA_LEVEL + 3;
        SendToSPI(readDA, &table);                
        analog = op_out;
        MinMax(analog, CALIB_COUNT, &min, &max, &prumer);
        do{                    
            //tmp = (min + max) / 2;              
            if (prumer > CALM_LEVEL){
                diff_1 =  prumer - CALM_LEVEL;
                readDA -= 1;
                SendToSPI(readDA, &table); 
                MinMax(analog, CALIB_COUNT, &min, &max, &prumer);
                if (prumer < CALM_LEVEL){
                    diff_2 = CALM_LEVEL - prumer;
                    if (diff_1 < diff_2) readDA += 1;
                    else break;
                }
            }
            MinMax(analog, CALIB_COUNT, &min, &max, &prumer);
            
        }while(prumer >= CALM_LEVEL);
        calib = false;

    }else if (prumer > CALM_LEVEL){
        // Send other value to D/A
        readDA = DA_LEVEL - 3;
        SendToSPI(readDA, &table);                
        analog = op_out;
        MinMax(analog, CALIB_COUNT, &min, &max, &prumer);
        
        do{ 
            MinMax(analog, CALIB_COUNT, &min, &max, &prumer);
            //tmp = (min + max) / 2;              
            if (prumer < CALM_LEVEL){
                diff_1 =  CALM_LEVEL - prumer;
                readDA += 1;
                SendToSPI(readDA, &table); 
                MinMax(analog, CALIB_COUNT, &min, &max, &prumer);
                if (prumer > CALM_LEVEL){
                    diff_2 = prumer - CALM_LEVEL;
                    if (diff_1 < diff_2) readDA -= 1;
                    else break;
                }
            }
            
            
        }while(prumer <= CALM_LEVEL);
        calib = false;

    }
        
    
}

void SendToSPI(uint8_t value, uint8_t *table)
{
    // 
    DA_out_SetLow();
    __delay_ms(3);
    uint8_t set = 0x30;
    uint8_t tmpL, tmpH;
    uint8_t total = 0;
    tmpH = value >> 4;
    tmpH += set;
    tmpL = value << 4;
    table[1] = tmpH;
    table[0] = tmpL;
    SPI1_Exchange8bit(table[1]);
    SPI1_Exchange8bit(table[0]);
    DA_out_SetHigh();
}


void MinMax(adc_channel_t analog, uint8_t count, uint16_t *min, uint16_t *max, uint16_t *prumer)
{
    int i;
    *min = 0;
    *max = 0;
    *prumer = 0;
    ProbeDelay(1);
    uint16_t result = 0;
    int tmp = 0;
    for (i = 0; i < count; i++){
        result = ADC_GetConversion(analog);
        if (i == 0){
                *min = result;
                *max = result;
        }
        //DataSend(result);
        ProbeDelay(1);
        tmp += result;
        if (result < *min) { *min = result;}
        if (result > *max) { *max = result;}
    }
    *prumer = tmp / count;
    //DataSend(*prumer);
    
}

void TimerTest()
{
    uint16_t value;
    value = 3035;       // 1s
    TMR0_WriteTimer(value);
    out_1_SetHigh();
    TMR0_StartTimer();
    while(TMR0_ReadTimer()){
        ProbeDelay(50);
        if (TMR0_HasOverflowOccured()) break;
    }
    TMR0_StopTimer();
    out_1_SetLow();
    value = 34285;          // 0.5 s
    TMR0_WriteTimer(value);
    TMR0_StartTimer();
    while(TMR0_ReadTimer()){
        
    }
    TMR0_StopTimer();
    out_1_SetHigh();
    value = 34285;          // 0.5 s
    TMR0_WriteTimer(value);
    TMR0_StartTimer();
    while(TMR0_ReadTimer()){
        
    }
    TMR0_StopTimer();
    out_1_SetLow();
}
