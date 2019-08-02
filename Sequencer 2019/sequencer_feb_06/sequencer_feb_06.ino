//The EKKOFLOK sequencer, November 2018
//Version 1, rev 2, geany
//-------------------------------------


//display
#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif


//U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

//_____

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0])) //macro to calculate the size of an array //int n = sizeof myArray / sizeof *myArray;
//#define NELEMS(x) (sizeof(x) / *sizeof(x))
//int a[17];
//int n = NELEMS(a);

//Comment out the following #define statement if not using breadboard
#define BREADBOARD

#ifdef BREADBOARD //if using breadboard
	const int tempo_pin = A3; //for the breadboard version
	const int ledPin[] =  {A0, A1, A2, 10}; //breadboard 
	const int length_switch = 12; //breadboard
	const int  buttonPin[11] = {2, 3, 4, 5, 6, 7, 8, 9, 2, 11, 12};
#endif

#ifndef BREADBOARD  //if not using breadboard	
	const int tempo_pin = A0;
	const int ledPin[] = {A2, A3, A4, A5}; //the pins that the LEDs are attached to
	const int length_switch = 10; //perf version
	const int  buttonPin[11] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; 
#endif

// the constants won't change:
//const int  buttonPin[11] = {2, 3, 4, 5, 6, 7, 8, 9, 2, 11, 12}; //{2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};   // the pins that the step switches are attached to
//const int clockPin = 11;
//const int ledPin[] = {A2, A3, 6, 7}; //the pins that the LEDs are attached to
//const int ledPin[] = {A2, A2, A2, A2}; //the pins that the LEDs are attached to //temp debug

const int track_selector_switch = 11; 

const int clock_indicator = 13;

const int pressed = LOW; // active low
const int number_of_step_switches = 8; 
const int number_of_step_switches_i_0 = number_of_step_switches - 1;

bool use_internal_clock = true; //set to false if using external clock

//one shot
//int length[] = {1000, 2000};
int length = 20;
unsigned long DELAY_TIME = 4; // 4ms
unsigned long delayStart = 0; // the time the delay started
bool delayRunning = false; // true if still waiting for delay to finish
bool time_to_trig = false;
bool one_shot_state = LOW;
//--------

// Variables will change:
//int count = 0;   //The main counter, increments on each new clock tick
const int number_of_tracks = 4;
int count[number_of_tracks] = {0};
int previousCount = 0;
int number_of_steps[] = {8,8,8,8};
int buttonState[11]     = {1,1,1,1,1,1,1,1,1,1,1};      // current state of the button
int lastButtonState[11] = {1,1,1,1,1,1,1,1,1,1,1};     // previous state of the button
int seq[number_of_step_switches][8] = {0};
int pointer[] = {0};
int track_selector_state = digitalRead(track_selector_switch);
int length_selector_state = digitalRead(length_switch);

//Internal timer
bool clock_state = LOW;
bool old_clock_state = LOW;
unsigned long previousMillis = 0;        // will store last time LED was updated
int interval = 200;           // interval at which to blink (milliseconds)
unsigned long onTime = 5;
unsigned long offTime = 10;
//-----------

//Variables related to the step switches
long time = 0;         // the last time the output pin was toggled
long debounce = 20;   // the debounce time, increase if the output flickers
int debounce_var = 200;

long time2 = 0;
int selected_track = 0;

long time3 = 0;
//--------

byte bin = 0;

void setup() {
  for(int i=0;i<NELEMS(buttonPin);i++){pinMode(buttonPin[i], INPUT_PULLUP);}   // initialize the button pins as a input:
  pinMode(buttonPin[11], INPUT_PULLUP);
    
  //Serial.begin(9600); // initialize serial communication: //remember to disable - makes the code horribly slow!!!
  //Serial.begin(31250); //midi baud rate
  
  for(int i=0; i<NELEMS(ledPin);i++){pinMode(ledPin[i], OUTPUT);} 
  pinMode(clock_indicator, OUTPUT);
  pinMode(tempo_pin, INPUT);
  
  u8x8.begin();
  u8x8.setPowerSave(0);
  
 
  
}

void loop() {
 int tempo = map(analogRead(tempo_pin), 0, 1023, 1023, 5);

 offTime = tempo;

 internalClock();
 
 track_selector_state = digitalRead(track_selector_switch);
 length_selector_state = 1; //digitalRead(length_switch); //seems like it's being pulled low somewhere
 
 track_selector();
 
 step_switches(selected_track);
 
 oled8x8(); //the oled routine slows down the clock considerably
 
// track_length(); //temp. commented out
 

 
 //one_shot_trigger(DELAY_TIME);
 
 for(int i=0; i<number_of_tracks;i++){
 trigger_with_and(i,i,count[i]);
}

  
 //debug("counter_0: ", count_1);
 
 

}

//-----------------------
void internalClock(){
  
 if(use_internal_clock == true){ //check if internal clock is enabled
  
    unsigned long currentMillis = millis();
     
    //if((clock_state == HIGH)  && (timer(onTime))) //(millis() % onTime == 0)) /*(currentMillis - previousMillis >= onTime))//old routine*/
	  if((clock_state == HIGH)  && (currentMillis - previousMillis >= onTime))//old routine
    {
      clock_state = LOW;  // Turn it off
      previousMillis = currentMillis;  // Remember the time
      digitalWrite(clock_indicator, clock_state);  // Update the actual LED
      
         
    }
    //else if ((clock_state == LOW) && (timer(offTime))) //(millis() % offTime == 0)) /*(currentMillis - previousMillis >= offTime)) //old routine*/
    else if ((clock_state == LOW) && (currentMillis - previousMillis >= offTime)) //(millis() % offTime == 0)) /*(currentMillis - previousMillis >= offTime)) //old routine*/
    {
      clock_state = HIGH;  // turn it on
      for(int i=0; i<number_of_tracks;i++){
      counter(i); //increment the counter
	  }
      previousMillis = currentMillis;   // Remember the time
      digitalWrite(clock_indicator, clock_state);   // Update the actual LED
      
      //oled8x8(); //update the oled on each new clock tick
    }
  }
  
  
  
  
 }

//the timer() function takes care of all functions that use the millis()
int timer(unsigned long TIME){
	bool timer_state = 0;
	
	if(millis() % TIME == 0) timer_state = 1;
	else timer_state = 0;
	
	return timer_state;
	
	}

//the main counter function that takes care of all the counting
void counter(int C){
	if(count[C] >= number_of_steps[C]-1) count[C] = 0;
        else {
      count[C]++; //increment the counter
      } 
  }
  
  
  
//---------------------

void track_selector(){
  for(int i=0;i<number_of_step_switches;i++){
  track_selector_state = digitalRead(track_selector_switch);
  buttonState[i] = digitalRead(buttonPin[i]);  // read the pushbutton input pin:

  if (digitalRead(track_selector_switch) == pressed && buttonState[i] == pressed && debounce_f(time2, debounce_var) /*millis() - time2 > debounce_var*/) {   // compare the buttonState to its previous state
    selected_track = i;
    time2 = millis();    
  }
  //else selected_track = 0;
 }
}

//debounce function using millis
int debounce_f(unsigned long TIME, unsigned long DEBOUNCE_VAR){
	bool debounce_state = 0;
	//TIME = 0;
	if(millis() - TIME > DEBOUNCE_VAR){ 
	debounce_state = 1;
	//TIME = millis();
	}
	else debounce_state = 0;
	
	return debounce_state;
}

  
//------------------- 
void step_switches(int TRACK){
  for(int i=0;i<number_of_step_switches;i++){
 
  if (buttonState[i] == pressed && lastButtonState[i] != pressed && track_selector_state != pressed && length_selector_state != pressed && debounce_f(time, debounce_var) /*millis() - time > debounce_var*/) {   // compare the buttonState to its previous state
    buttonState[i] = digitalRead(buttonPin[i]);  // read the pushbutton input pin:
    if (seq[TRACK][i] == 0) 
         seq[TRACK][i] = 1; 
    else seq[TRACK][i] = 0;  
    
  time = millis();    
  }

  lastButtonState[i] = buttonState[i]; // save the current state as the last state, for next time through the loop
  
 }
} 







void debug(char TEKST[], int INFO){
  Serial.print(TEKST);
  Serial.println(INFO); 
  }

int track_length(){ 
 for(int i=0;i<number_of_step_switches;i++){
  if(length_selector_state == pressed && digitalRead(buttonPin[i]) == pressed) number_of_steps[selected_track] = i+1; //set the track length of the selected track to the number represented by the pressed switch (+1)
   }
   return number_of_steps[selected_track];
  
}

//**********************************************
//------------ The output gates work by AND'ing the active step with the clock. This way the output pins are only HIGH when both clock AND step is 1
//This makes it possible to make a global trigger length and is also easily adaptable to using external analog clock 
void trigger_with_and(int OUT, int TRACK, int COUNT){
  digitalWrite(ledPin[OUT], seq[TRACK][COUNT] && clock_state);
  
  //MIDI output, outputs a midi note, the length of the step (not the trigger), each note an octave apart
  byte note_offset = 12; //octave
  byte vel = seq[TRACK][COUNT] * 45; //returns either 0 or 45, middle velocity
  byte note = TRACK * note_offset;
  byte channel = 0x90; //midi channel 1
  //channel += TRACK; //set a diffenrent channel for each track
  //noteOn(channel, note, vel);
  
  }
  
//**********************************************

void oled8x8(){
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setCursor(0, 0);
  u8x8.print("TRACK:");
  u8x8.setCursor(6, 0);
  u8x8.print(selected_track+1, DEC);

  u8x8.setCursor(8, 0);
  u8x8.print("LENGTH: ");
  u8x8.setCursor(15, 0);
  u8x8.print(number_of_steps[selected_track], DEC);

    int space = 2;
    int offset =0;
  u8x8.setCursor(offset+((count[selected_track])*space), 10); //u8x8.setCursor(count[selected_track]*15, 20);
  //u8x8.print(" ");//seq[selected_track][count[selected_track]], BIN);
  u8x8.print("x                             ");//seq[selected_track][count[selected_track]], BIN);
  //u8x8.drawString((count[0]-1)*space, 10,"        ");
  u8x8.print(" ");//seq[selected_track][count[selected_track]], BIN);
  u8x8.print(" ");//seq[selected_track][count[selected_track]], BIN);
  
  for(int i=0;i<8;i++){
  u8x8.setCursor(offset+(i*space), 20);
  u8x8.print(seq[selected_track][i], BIN);
    }  
    
  for(int i=0;i<number_of_tracks;i++){
  u8x8.setCursor(offset+(i*space), 30);
  u8x8.print(count[i]+1, DEC);
    }   
    
    /*
    //doesn't work proberly yet, will be used to write seq info to eeprom
    byte receivedID[] = {0};
    for(int j=0; j<4; j++){
	receivedID[j] = {0};
	for(int i=0; i<8; i++){
	receivedID[j] |= seq[j][i] << i;
	u8x8.setCursor(offset+(3*space), 30);
	u8x8.print(receivedID[selected_track], BIN);
	}
   }*/
   
   
	
	
	/*
	for (int i = 0; i < 8; i++ )
	{
		bin = bin >> 1; //bin *= 2; //
		bin = bin + seq[0][i];
	}
	*/
	
	/*
	for(int i = 0; i < 8; i++)
  {
	  for(int j = 7; j >= 0; j--)
	  {
    // if(/*seq[0][i]) bin = bin | (seq[0][i] << i);
    
    } */
    
    /*
    for(int i = 0; i < 8; i++)
  {
	  for(int j = 7; j >= 0; j--)
	  {
    if(seq[0][i] == 1) bin |= (1 >> j);
    if(seq[0][i] == 0) bin &= ~(1 >> j);
	}	
  }
  */
  //boolu();
  //u8x8.setCursor(offset+(3*space), 35);
  //u8x8.print(128, BIN);
  
  
//if (f) w |= m; else w &= ~m;
  


		
	
  //byte seq_1_byte = BoolArrayToByte(seq[0][]);
  

	
}


//-- One shot trigger to shape the length of the trigger outputs
int one_shot_trigger(unsigned long TIME){
  /*//int one_shot_count = 0;
  //old_clock_state == LOW;
  
  if(clock_state != old_clock_state) //time_to_trig = true;
  {
    if(clock_state == HIGH){
  //else time_to_trig = false;
  
  //if(time_to_trig == true){ 
    delayStart = millis();
    delayRunning = true;
    one_shot_state = HIGH;
  //}
  if (delayRunning == true && ((millis() - delayStart) >= TIME)) {
    delayRunning = false; // finished delay -- single shot, once only
    one_shot_state = LOW;
    old_clock_state = clock_state;
    delayStart = millis();
    
    //one_shot_count ++;
    }
  }
 }
  //old_clock_state = clock_state;
  return one_shot_state;*/
  
    // check if delay has timed out
  if(clock_state == HIGH){
    delayStart = millis();
    delayRunning = true;
    one_shot_state = HIGH; //digitalWrite(LED, HIGH);
  }
  if (delayRunning && ((millis() - delayStart) >= TIME)) {
    delayRunning = false; // finished delay -- single shot, once only
    one_shot_state = LOW; //digitalWrite(LED, LOW); // turn led off
    delayStart = millis();
  }
 }
 
 


byte BoolArrayToByte(bool boolArray[8])
{
  byte result = 0; 

  for(int i = 0; i < 8; i++)
  {
    if(boolArray[i]) result = result | (1 << i); //if (f) w |= m; else w &= ~m;
    else result = result & ~(1 << i);

  }

  return result;
}




byte boolu(){
	byte bin = 0;
	for (int i = 0; i<8; i++){
    bin += seq[0][i]*(1 << (7-i));
  }
  
	return bin;
	
	}
	

//MIDI*****************************************************
	
	/*		  220r	
	 * 5V----^^^^^-----midi pin 4 (ring)
	 * TX(01)-^^^^-----midi pin 5 (tip)
	 * GND ------------midi pin 2 (optional?) (sleeve)
	 * 
	 * 
	 * midi baud rate: Serial.begin(31250);
	 * 
	 */
	 
	 
	 

/*
  // play notes from F#-0 (0x1E) to F#-5 (0x5A):
  for (int note = 0x1E; note < 0x5A; note ++) {
    //Note on channel 1 (0x90), some note value (note), middle velocity (0x45):
   
    noteOn(0x90, note, 0x45);
    delay(100);
    //Note on channel 1 (0x90), some note value (note), silent velocity (0x00):
    noteOn(0x90, note, 0x00);
    delay(100);
  }*/
	

// plays a MIDI note. Doesn't check to see that cmd is greater than 127, or that
// data values are less than 127:
void noteOn(int channel, int pitch, int velocity) {
  Serial.write(channel);
  Serial.write(pitch);
  Serial.write(velocity);
}
