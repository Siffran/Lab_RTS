#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>

int brotherJohnIndex[32] = {0,2,4,0,0,2,4,0,4,5,7,4,5,7,7,9,7,5,4,0,7,9,7,5,4,0,0,-5,0,0,-5,0};
int periodBrotherJohn[25] = {2025,1911,1804,1703,1607,1517,1432,1351,1276,1204,1136,1073,1012,956,902,851,804,758,716,676,638,602,568,536,506};
int beatLength[32] = {2,2,2,2,2,2,2,2,2,2,1,2,2,1,4,4,4,4,2,2,4,4,4,4,2,2,2,2,1,2,2,1};

int *SPEAKER = ((int *) 0x4000741C);

typedef struct {
    Object super;
    int i;
    int count;
    char c;
    int myNum;
    int id;
    int numOfSlaves;
    int active;
    char buffer[20];
} App;

App app = { initObject(), 0, 0, 'X', 0, 0, 2, 0};

typedef struct {
    Object super;
    int volume;
    int prevVolume;
    int deadline;
    int running;
}Tone;

Tone tone = {initObject(), 0, 0, 0, 0};

typedef struct {
    Object super;
    int running;
    int index;
    int key;
    int tempo;
    Msg tag;
    char buffer[20];
}ToneCord;

ToneCord toneCord = {initObject(), 0, 0, 0, 120}; // Tone coordinator

typedef struct {
    Object super;
    int background_loop_range;
    int deadline;
    char buffer[20];
}Dirty;

Dirty dirty = {initObject(), 1000, 0};

//Declare functions
void reader(App*, int);
void receiver(App*, int);

void toneGen(Tone*, int);
void toneGenStart(Tone*, int);
void toneGenStop(Tone*, int);

void tonePlay(ToneCord*, int);
void toneStart(ToneCord*, int);
void toneStop(ToneCord*, int);

void increaseVol(Tone*, int);
void decreaseVol(Tone*, int);
void mute(Tone*, int);

void stopRestart(ToneCord*, int);
void startOrStop(ToneCord*, int);

void setKey(ToneCord* self, int key);

void setTempo(ToneCord* self, int tempo);

void setDeadline(Tone*, int arg);

void deadlineSwitchTone(Tone*, int);
void deadlineSwitchLoad(Dirty*, int);

void loadGen(Dirty*, int);

void increaseDirt(Dirty*, int);
void decreaseDirt(Dirty*, int);

void printBrotherJohnPeriods(int);

//--------------------------------//

Serial sci0 = initSerial(SCI_PORT0, &app, reader); //Talked about this on the lecture

Can can0 = initCan(CAN_PORT0, &app, receiver);

//--------------------------------//

void toneGen(Tone *self, int arg){
    
    if(self->running){ //if running !=0 do, otherwise terminate
        if(*SPEAKER != 0){
            *SPEAKER = 0;
        }else{
            *SPEAKER = self->volume;
        }
        SEND( USEC(arg), USEC(self->deadline), self, toneGen, arg);
    }
}

void toneGenStart(Tone *self, int unused){
    
    //set running to 1 in Tone Object
    self->running = 1;
    
}


void toneGenStop(Tone *self, int unused){
    
    //set running to 0 in Tone Object
    self-> running = 0;
    
}

void tonePlay(ToneCord *self, int unused){
    
    //if running do below, otherwise terminate
    
        //use argument and key to find period value
        //use tempo to set deadline for Tone Object
    
        //Call toneGen using BEFORE with argument: period
        //Kill toneGen 50-100 ms before the next tone is to be played. (set Tone Object running to 0) How to not kill instantly?) Do we need a timer in this object?
        //How do we make sure the new tone is not begun instatly? use AFTER!?
    
    int tempo;
    int period;

    if(self->running){
        
        period = periodBrotherJohn[brotherJohnIndex[self->index]+self->key+10];
        tempo = (beatLength[self->index] * self->tempo)/2;
        tempo = 60000/tempo;
         
        //set deadline for Tone object using tempo, do we need this?
        SYNC( &tone, setDeadline, period);
         
        ASYNC( &tone, toneGen, period); //call toneGen
         
        AFTER( MSEC(tempo-75), &tone, toneGenStop, 0);
        SYNC(&tone, toneGenStart, 0);
         
        self->index++;
        self->index = self->index % 32;
         
        self->tag = AFTER( MSEC(tempo), self, tonePlay, 0 );
    }
}

void loadGen(Dirty *self, int arg){
    
    if(arg == 0){
        for(int i = 0; i < self->background_loop_range; i++){}
        arg = 1;
    }else{
        arg = 0;
    }
    
    SEND( USEC(650), USEC(self->background_loop_range), self, loadGen, arg);

    //AFTER(USEC(650), self, loadStop, 0);
}

void receiver(App *self, int unused) { //interrupt from CANbus
    CANMsg msg;
    
    CAN_RECEIVE(&can0, &msg);
    SCI_WRITE(&sci0, "Can msg received: ");
    SCI_WRITE(&sci0, msg.buff);
    SCI_WRITE(&sci0, "\n");
    
    /*
     * 1 = tempo
     * 2 = key
     * 3 = start/stop
     * 4 = restart
     * */
    
    if(msg.nodeId == self->id){
        
        switch(msg.msgId){
            case 1:
               SYNC(&toneCord, setTempo, msg.buff[0]);
            break;
            
            case 2:
                SYNC(&toneCord, setKey, msg.buff[0]);
            break;
            
            case 3:
                SYNC(&toneCord, startOrStop, 0);
            break;
            
            case 4:
                SYNC(&toneCord, stopRestart, 0);
            break;
        }
    }
    
    /*
    if(msg.msgId == self->id){
        
        switch (msg.buff[msg.length-1]){
            case 's':
                SYNC(&toneCord, startOrStop, 0);
            break;

            case 't':
                SYNC(&toneCord, setTempo, msg.buff[0]);
            break;
            
            case 'x':
                SYNC(&toneCord, stopRestart, 0);
            break;
        
            case 'k':
                SYNC(&toneCord, setKey, msg.buff[0]);
            break;
            
            default:
                SCI_WRITE(&sci0, "CAN message not recognized \n");
        
        }
    }*/
}

void reader(App *self, int c) { //interrupt when key is pressed
    
    CANMsg msg;
    
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
        int key = atoi(self->buffer);
        key += 5;
            
        if(self->numOfSlaves){
                
            for(int i = 1; i <= self->numOfSlaves; i++){
                    
                msg.msgId = 2;
                msg.nodeId = i;
                msg.length = 1;
                msg.buff[0] = key;
                CAN_SEND(&can0, &msg);
            }
            SYNC(&toneCord, setKey, key);
        }
    }else if(c == 'e'){ //         
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
        SYNC(&tone, increaseVol,0);
    }else if(c == 'o') { //decrease volume by 1
        SYNC(&tone, decreaseVol,0);
    }else if(c == 'm') { //mute
        SYNC(&tone, mute,0);
    }else if(c == 'i'){
        
    }else if(c == 'u'){
        SYNC(&dirty, decreaseDirt,0);
    }else if( c == 'd'){ //toggle deadlines on/off
        SYNC(&tone, deadlineSwitchTone, 0);
        SYNC(&dirty, deadlineSwitchLoad, 0);
    }else if(c == 't'){
        self->buffer[self->i] = '\0';
        self->i = 0;
        int tempo = atoi(self->buffer);
        
        if(self->numOfSlaves){
        
            for(int i = 1; i <= self->numOfSlaves; i++){
                msg.msgId = 1;
                msg.nodeId = i;
                msg.length = 1;
                msg.buff[0] = tempo;
                //msg.buff[1] = 't';
                CAN_SEND(&can0, &msg);
            }
            SYNC(&toneCord, setTempo, tempo);
        }
    }else if(c == 's'){
        if(self->numOfSlaves){
            
            for(int i = 1; i <= self->numOfSlaves; i++){
                
                msg.msgId = 3;
                msg.nodeId = i;
                msg.length = 0;
                //msg.buff[0] = 's';
                CAN_SEND(&can0, &msg);
            }
            SYNC(&toneCord, startOrStop, 0);
        }
        
    }else if(c == 'x'){
        
        if(self->numOfSlaves){
            
            for(int i = 1; i <= self->numOfSlaves; i++){
                msg.msgId = 4;
                msg.nodeId = i;
                msg.length = 0;
                //msg.buff[0] = 'x';
                CAN_SEND(&can0, &msg);
            }
            SYNC(&toneCord, stopRestart, 0);
        }
        
    }else if( c != '\n'){ //add c to buffer
        self->buffer[self->i] = c;
        self->i = self->i+1;
    }
}

void stopRestart(ToneCord *self, int unused){
    self->index = 0;
}

void startOrStop(ToneCord *self, int unused){ //The play function
    if(self->running == 0){
            self->running = 1;
            ASYNC(&toneCord, tonePlay, 0);
            SCI_WRITE(&sci0, "Song Started");
            
    }else{
            self->running = 0;
            ABORT(self->tag);
            SYNC(&tone, toneGenStop, 0);
            SCI_WRITE(&sci0, "Song Stoped");
            
    }
}

void mute(Tone *self, int unused){
    if (self->volume != 0) {
        self->prevVolume = self->volume;
        self->volume = 0; 
    }else { 
        self->volume = self->prevVolume; 
    }
}

void increaseVol(Tone *self, int unused){
    self->volume += 1; 
    if (self->volume >= 20) {
        self->volume = 20; 
    }  
}

void decreaseVol(Tone *self, int unused){
    self->volume -= 1; 
    if (self->volume <= 0) {
        self->volume = 0;
    }
}

void setKey(ToneCord *self, int key){
    key -= 5;
    if( key <= 5 && key >= -5){
        self->key = key;
        SCI_WRITE(&sci0, "New key entered \n");
    }else{
        SCI_WRITE(&sci0, "New key not valid \n");
    }
}

void setTempo(ToneCord *self, int tempo){
    if( tempo <= 240 && tempo >= 60){
        self->tempo = tempo;
        SCI_WRITE(&sci0, "New tempo entered \n");
    }else{
        SCI_WRITE(&sci0, "New tempo not valid \n");
    }
}

void setDeadline(Tone* self, int arg){
    self->deadline = arg;
}
void deadlineSwitchTone(Tone* self, int unused){ //Switch deadline on and off for tone generator
    if(self->deadline == 0){
        self->deadline = 100;
        SCI_WRITE(&sci0, "deadline on\n");
    }else{
        self->deadline = 0;
        SCI_WRITE(&sci0, "deadline off\n");
    }
}

void deadlineSwitchLoad(Dirty* self, int unused){ //Switch deadline on and off for background loop
    if(self->deadline == 0){
        self->deadline = 1300;
        SCI_WRITE(&sci0, "deadline on\n");
    }else{
        self->deadline = 0;
        SCI_WRITE(&sci0, "deadline off\n");
    }
}

/* NOT IMPORTANT FUNCTIONS */

void increaseDirt(Dirty *self, int unused){
    self->background_loop_range += 500; 
    if (self->background_loop_range >= 21000) {
        self->background_loop_range = 21000; 
    }
    snprintf(self->buffer, 20, "%d", self->background_loop_range);
    SCI_WRITE(&sci0, self->buffer);
}

void decreaseDirt(Dirty *self, int unused){
    self->background_loop_range -= 500; 
    if (self->background_loop_range <= 0) {
        self->background_loop_range = 0;
    }
    snprintf(self->buffer, 20, "%d", self->background_loop_range);
    SCI_WRITE(&sci0, self->buffer);
}

void printBrotherJohnPeriods(int key){ //Prints the period times for Brother John based on key.
    
    char temp[20];
    
    for(int i; i < 32; i++){
        sprintf(temp,"%d", periodBrotherJohn[brotherJohnIndex[i]+key+10]);
        SCI_WRITE(&sci0, temp);  //print periods of brother john based on key
        SCI_WRITE(&sci0, ", ");
    }
    SCI_WRITE(&sci0, '\n');
}

void wcetCount(){  //Measure WCET of some function. returns avereage and worst time.
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
        //loadGen(&dirty, 0);
        toneGen(&tone, 0);
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

/* NOT IMPORTANT FUNCTIONS END */

void startApp(App *self, int arg) {
    CANMsg msg;

    CAN_INIT(&can0);
    SCI_INIT(&sci0);
    SCI_WRITE(&sci0, "Hello, hello...\n");
    
    
    /* TESTTESTTES
    char buffer[10];
    uchar a = 1;
    uchar b = 2;
    
    int c = a + b;
    
    snprintf(buffer,10,"%d", c);
    SCI_WRITE(&sci0, buffer);
    */
    
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
