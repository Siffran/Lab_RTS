#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>

int brotherJohnIndex[32] = {0,2,4,0,0,2,4,0,4,5,7,4,5,7,7,9,7,5,4,0,7,9,7,5,4,0,0,-5,0,0,-5,0};
int period[25] = {2025,1911,1804,1703,1607,1517,1432,1351,1276,1204,1136,1073,1012,956,902,851,804,758,716,676,638,602,568,536,506};

//#define SPEAKER             0x4000741C;
//#define WRITE_SPEAKER(val)  ((*(volatile uint8_t *)SPEAKER) = (val));

int *SPEAKER = ((int *) 0x4000741C);

/* Notes for Part 1
 * 
 * Define address to Digital to Analog converter
 * 
 * 0 to D/A - silence
 * 5-20 to D/A - sound (volume)
 * 
 * Not enough to use ASYNC when generating tone 
 * Need to use AFTER aswell to wake us up when done
 * 
 * Look in the tinytimber tutorial!
 * 
 */
 
typedef struct {
    Object super;
    int i;
    int count;
    char c;
    int key;
    int myNum;
    char buffer[20];
} App;

App app = { initObject(), 0, 0, 'X', 0, 0};

typedef struct {
    Object super;
    int volume;
    int prevVolume;
    int deadline;
}Tone;

Tone toneGen = {initObject(), 0, 0, 0};

typedef struct {
    Object super;
    int background_loop_range;
    int deadline;
    char buffer[20];
}Dirty;

Dirty dirty = {initObject(), 1000, 0};

//Declare functions
void printBrotherJohnPeriods(int);
void toneStop(Tone*, int);
void toneStart(Tone*, int);
void loadStart(Dirty*,int);
void loadStop(Dirty*,int);
void mute(Tone*);
void increaseVol(Tone*);
void decreaseVol(Tone*);
void increaseDirt(Dirty*);
void decreaseDirt(Dirty*);
void reader(App*, int);
void receiver(App*, int);
void deadlineSwitchTone(Tone*);
void deadlineSwitchLoad(Dirty*);

Serial sci0 = initSerial(SCI_PORT0, &app, reader); //Talked about this on the lecture

Can can0 = initCan(CAN_PORT0, &app, receiver);

/*
void toneStop(Tone *self, int arg){
    *SPEAKER = 0;
    AFTER(USEC(arg), self, toneStart, arg);
}*/

void toneStart(Tone *self, int arg){
    
    for(int i = 0; i < 1000; i++){
        
        if(*SPEAKER != 0){
            *SPEAKER = 0;
        }else{
            *SPEAKER = self->volume;
        }
    }
    
    
    //SEND( USEC(arg), USEC(self->deadline), self, toneStart, arg); //why not this!?
}

/*
void loadStop(Dirty *self, int arg){
    //After
    AFTER(USEC(650), self, loadStart, 0); 
}*/
    
void loadStart(Dirty *self, int arg){
    
    if(arg == 0){
        for(int i = 0; i < self->background_loop_range; i++){}
        arg = 1;
    }else{
        arg = 0;
    }
    
    //SEND( USEC(650), USEC(self->background_loop_range), self, loadStart, arg);

    //AFTER(USEC(650), self, loadStop, 0);
    
}

void receiver(App *self, int unused) { //interrupt from CANbus
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "Can msg received: ");
    SCI_WRITE(&sci0, msg.buff);
}

void reader(App *self, int c) { //interrupt when key is pressed
    
    SCI_WRITE(&sci0, "Rcv: \'");
    SCI_WRITECHAR(&sci0, c);
    SCI_WRITE(&sci0, "\'\n");
    
    if(c == 'F'){//reset running sum
        self->myNum = 0;
        self->i=0;
        
        snprintf(self->buffer,20,"%d",self->myNum);     //Save as string in buffer
        SCI_WRITE(&sci0, "The running sum is ");
        SCI_WRITE(&sci0, self->buffer);
        SCI_WRITE(&sci0, "\n");
        
    }else if(c == 'k'){
        self->buffer[self->i] = '\0';
        self->i = 0;
        self->key = atoi(self->buffer);
        
        if(self->key > 5 || self->key < -5){
            SCI_WRITE(&sci0, "input key value is not valid, enter value between -5 and 5\n");
        }else{
            printBrotherJohnPeriods(self->key);
        }
        
    }else if(c == 'e'){//end of buffer        
        self->buffer[self->i] = '\0';                   //insert null at end of buffer
        self->i = 0;                                    //set i = 0
        
        SCI_WRITE(&sci0, "The entered number is ");
        SCI_WRITE(&sci0, self->buffer);
        SCI_WRITE(&sci0, "\n");
        
        self->myNum = self->myNum + atoi(self->buffer); //convert buffer to int and add
        
        snprintf(self->buffer,20,"%d",self->myNum);     //Save as string in buffer
        SCI_WRITE(&sci0, "The running sum is ");
        SCI_WRITE(&sci0, self->buffer);
        SCI_WRITE(&sci0, "\n");
        
    }else if(c == 'p') { //increase volume by 1
        SYNC(&toneGen, increaseVol,0);
    }else if(c == 'o') { //decrease volume by 1
        SYNC(&toneGen, decreaseVol,0);
    }else if (c == 'm') { //mute
        SYNC(&toneGen, mute,0);
    }else if (c == 'i'){
        SYNC(&dirty, increaseDirt,0);
    }else if (c == 'u'){
        SYNC(&dirty, decreaseDirt,0);
    }else if ( c == 'd'){
        SYNC(&toneGen, deadlineSwitchTone, 0);
        SYNC(&dirty, deadlineSwitchLoad, 0);
    }else if( c != '\n' ){//next buffer value
        self->buffer[self->i] = c;                      //add c to end of buffer
        self->i = self->i+1;
    }
}
void deadlineSwitchTone(Tone* self){
    if(self->deadline == 0){
        self->deadline = 100;
        SCI_WRITE(&sci0, "deadline on\n");
    }else{
        self->deadline = 0;
        SCI_WRITE(&sci0, "deadline off\n");
    }
}

void deadlineSwitchLoad(Dirty* self){
    if(self->deadline == 0){
        self->deadline = 1300;
        SCI_WRITE(&sci0, "deadline on\n");
    }else{
        self->deadline = 0;
        SCI_WRITE(&sci0, "deadline off\n");
    }
}

void mute(Tone *self){
    if (self->volume != 0) {
        self->prevVolume = self->volume;
        self->volume = 0; 
    }else { 
        self->volume = self->prevVolume; 
    }
}

void increaseVol(Tone *self){
    self->volume += 1; 
    if (self->volume >= 20) {
        self->volume = 20; 
    }  
}

void decreaseVol(Tone *self){
    self->volume -= 1; 
    if (self->volume <= 0) {
        self->volume = 0;
    }
}

void increaseDirt(Dirty *self){
    self->background_loop_range += 500; 
    if (self->background_loop_range >= 21000) {
        self->background_loop_range = 21000; 
    }
    snprintf(self->buffer, 20, "%d", self->background_loop_range);
    SCI_WRITE(&sci0, self->buffer);
}

void decreaseDirt(Dirty *self){
    self->background_loop_range -= 500; 
    if (self->background_loop_range <= 0) {
        self->background_loop_range = 0;
    }
    snprintf(self->buffer, 20, "%d", self->background_loop_range);
    SCI_WRITE(&sci0, self->buffer);
}

void printBrotherJohnPeriods(int key){
    
    char temp[20];
    
    for(int i; i < 32; i++){
        sprintf(temp,"%d", period[brotherJohnIndex[i]+key+10]);
        SCI_WRITE(&sci0, temp);  //print periods of brother john based on key
        SCI_WRITE(&sci0, ", ");
    }
    SCI_WRITE(&sci0, '\n');
}

void wcetCount(){
    Time startTime;
    Time diff;
    
    char buffer1[32];
    char buffer2[32];
    int temp = 0;
    int average = 0;
    int maximum = 0;
    int total = 0;
    
    for(int i = 0; i < 500; i++){
        startTime = CURRENT_OFFSET();
        //loadStart(&dirty, 0);
        toneStart(&toneGen, 0);
        diff = CURRENT_OFFSET();
        
        diff = diff - startTime;
        
        temp = USEC_OF(diff); // Migth need to be Long int
        
        if(temp > maximum){
            maximum = temp;
        }
        
        total = total + temp;
        
        average = total/(i+1);
    }
    
    snprintf(buffer1, 32, "%d", average);
    snprintf(buffer2, 32, "%d", maximum);
    SCI_WRITE(&sci0, "The average time was: ");
    SCI_WRITE(&sci0, buffer1);
    SCI_WRITE(&sci0, "\n");
    
    SCI_WRITE(&sci0, "The maximum time was: ");
    SCI_WRITE(&sci0, buffer2);
    SCI_WRITE(&sci0, "\n");
    
}

void startApp(App *self, int arg) {
    CANMsg msg;

    CAN_INIT(&can0);
    SCI_INIT(&sci0);
    SCI_WRITE(&sci0, "Hello, hello...\n");
    
    //ASYNC(&toneGen, toneStart, 500);
    //ASYNC(&dirty, loadStart, 0);
    
    wcetCount();
    
    
    msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 6;
    msg.buff[0] = 'H';
    msg.buff[1] = 'e';
    msg.buff[2] = 'l';
    msg.buff[3] = 'l';
    msg.buff[4] = 'o';
    msg.buff[5] = 0;
    CAN_SEND(&can0, &msg);
}

int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0); //Inform the TinyTimber kernel that the method is a handler
	INSTALL(&can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}
