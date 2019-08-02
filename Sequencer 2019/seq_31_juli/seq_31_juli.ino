//The EKKOFLOK sequencer, July 2019
//Version 1, rev 2, geany
//-------------------------------------
//Currently working on implementing C++ classes to organize the code.
//EEprom writing and reading works to and from track 1 and 2 -- debugging needed

#include <Arduino.h>
#include <U8g2lib.h>
#include <EEPROM.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

uint32_t clock_speed = 1000000; //clock speed of the oled

//U8G2_SH1106_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);

//_____

#define DISPLAY //comment out if bypassing the display

#define eepromf //uncomment to work with the eeprom
#define NELEMS(x)  (sizeof(x) / sizeof((x)[0])) //macro to calculate the size of an array //int n = sizeof myArray / sizeof *myArray;
//#define NELEMS(x) (sizeof(x) / *sizeof(x))
//int a[17];
//int n = NELEMS(a);

//Comment out the following #define statement if not using breadboard
//#define BREADBOARD

#ifdef BREADBOARD //if using breadboard
	const int tempo_pin = A3; //for the breadboard version
	const int ledPin[] =  {A0, A1, A2, 10}; //breadboard 
	const int length_switch = 12; //breadboard
	const int buttonPin[11] = {2, 3, 4, 5, 6, 7, 8, 9, 2, 11, 12};
#endif

#ifndef BREADBOARD  //if not using breadboard	
	const int tempo_pin = A0;
	const int ledPin[] = {A2, A3, A4, A5}; //the pins that the LEDs are attached to
	const int length_switch = 10; //perf version
	const int buttonPin[11] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; 
#endif


//pin connections
const int clock_indicator = 13;
const int track_selector_switch = 11;

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

// Sequencer logic variables
//int count = 0;   //The main counter, increments on each new clock tick
const int number_of_tracks = 4; //4 analog outputs
const int max_count = 8;
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


//eeprom variables
int address = 0;
int eeprom_offset = 128; //was 8;
int eeprom_parameter[8] = {0}; //eight slots of sixteen bytes each: 0 pattern, 1 length, 2 selected track, 3 div/mult, 4 random, 5 empty, 6 empty, 7 active bank 
int eeprom_bank[8] = {}; //eight banks of 128 bytes each
int cell_offset = 16; //16 bytes for each "cell", 8 cells for each track: 0 steps, 1 length, 2 selected track

//eeprom CLASS

class EEprom
{
//variables

public:
    EEprom()
    {
    
    }
    
    int eeprom_address(int t)
    {
       int eeprom_adr = eeprom_offset * t;
       return eeprom_adr;
    }    

};

EEprom EE; //initialize
  
void setup() {


  for(int i=0;i<NELEMS(buttonPin);i++){pinMode(buttonPin[i], INPUT_PULLUP);}   // initialize the button pins as a input:
  pinMode(buttonPin[11], INPUT_PULLUP);
    
  //Serial.begin(9600); // initialize serial communication: //remember to disable - makes the code horribly slow!!!
  
  for(int i=0; i<NELEMS(ledPin);i++){pinMode(ledPin[i], OUTPUT);} 
  pinMode(clock_indicator, OUTPUT);
  pinMode(tempo_pin, INPUT);
  
#ifdef DISPLAY
   //DISPLAY
  u8x8.begin();
  u8x8.setPowerSave(0);
  
  //uint32_t getBusClock(void);
  void setBusClock(uint32_t clock_speed);
  
 
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.setCursor(0, 0);
  u8x8.print("TRACK:");
  
  u8x8.setCursor(8, 0);
  u8x8.print("LENGTH: ");
  
#endif


//EEPROM

//eeprom addressing  
  for(int i=0;i<NELEMS(eeprom_parameter);i++)
  {	
    eeprom_parameter[i] = cell_offset * i; //add 16 bytes (cell_offset) to each parameter, making it easier to address parameter settings
   }
  
  for(int i=0;i<NELEMS(eeprom_bank);i++)
  {	
    eeprom_bank[i] = eeprom_offset * i; //add 128 bytes (eeprom_offset) to each parameter, making it easier to address parameter settings
   } 
  //----------- read the eeprom
  //step patterns
  /*
  for(int track = 0; track<number_of_tracks; track ++){
			//int track = 0;//
			int eeprom_address = eeprom_offset * track;
	  		for(int i=0;i<8;i++){ //should change this to a variable
			seq[track][i] = EEPROM.read(track * eeprom_address + i); //read the recorded settings for each step on each track
		    }
	}
    */
    
   

    for(int track = 0; track<number_of_tracks; track ++){
			//int track = 0;//
			//int eeprom_address = eeprom_offset * track;
	  		for(int i=0;i<8;i++){ //should change this to a variable
			seq[track][i] = EEPROM.read(EE.eeprom_address(track) + i); //read the recorded settings for each step on each track
		    }
	}

	
  
  /*
   //Read the length of each track from the eeprom
  for(int track = 0; track<4; track ++){
		
  number_of_steps[track] = EEPROM.read(track * eeprom_offset + i + 32); //read the recorded settings for each track
		
	}*/


}

void loop() {
 int tempo = map(analogRead(tempo_pin), 0, 1023, 1023, 50);

 offTime = tempo;

 internalClock();
 
 track_selector_state = digitalRead(track_selector_switch);
 length_selector_state = digitalRead(length_switch);
 
 track_selector();
 
 step_switches(selected_track);
 
 track_length();
 
 //if(timer(10)) oled8x8(); //update the oled 
 
 //one_shot_trigger(DELAY_TIME);
 
 //send the triggers to the outputs
 for(int i=0; i<number_of_tracks;i++){
 trigger_with_and(i,i,count[i]);
 }

  //int repeats = 3;
  settings_eeprom(number_of_tracks); 
  
  
  //zeros_in_eeprom(); //uncomment to fill the eeprom with zeros
  
  
  
  /*-------------------
  //temporary eeprom code for track 4 only:
   ///////////step patterns
 int bank = 0;  
 int track = temp_track;  
 int eeprom_address = eeprom_offset * track;
 for(int step=0; step<number_of_step_switches;step++){
 eeprom(seq[track][step], eeprom_address + step, bank); //write settings to eeprom - seq[track 0-3][step 0-7] | eeprom(value 0-1, address 0-1023, bank 0-7)
 }    
 //--end of temp  
-------------*/

  
 //debug("counter_0: ", count_1);
 
 





}//end of loop

int settings_eeprom(int tracks){
 //eeprom function
 #ifdef eepromf
 byte bank = 8; //8 banks, each with 128 bytes of memory for saving settings
 //byte track = 0;
 int eeprom_address = 0;
 byte parameter = 0; //8 parameters / cells for each track: 0 steps, 1 length, 2 selected track

 ///////////step patterns
 for(int track=0;track<tracks/*number_of_tracks*/;track++){
 byte parameter = 0; //8 parameters / cells for each track: 0 steps, 1 length, 2 selected track
 //eeprom_offset += cell_offset * parameter; //add an offset to make room in the eeprom for all the parameters
 //eeprom_address = eeprom_offset * track;
 for(int step = 0; step<number_of_step_switches; step++){  //Save the current state of the switches to next power on
   //for(int j = 0; j<8; j++){
 eeprom(seq[track][step], EE.eeprom_address(track) + step, bank); //write settings to eeprom - seq[track 0-3][step 0-7] | eeprom(value 0-1, address 0-1023, bank 0-7)
     }
   }

  /* work in progress
 ///////////step patterns
 for(int track=0;track<number_of_tracks;track++){
 byte parameter = 0; //8 parameters / cells for each track: 0 steps, 1 length, 2 selected track
 eeprom_offset += cell_offset * parameter; //add an offset to make room in the eeprom for all the parameters
 eeprom_address = eeprom_offset * track;
 for(int step = 0; step<number_of_step_switches; step++){  //Save the current state of the switches to next power on
   //for(int j = 0; j<8; j++){
 eeprom(seq[track][step], eeprom_address + step, bank); //write settings to eeprom - seq[track 0-3][step 0-7] | eeprom(value 0-1, address 0-1023, bank 0-7)

 }
}
*/

/* work in progress
///////////track lengths
 for(int track=0;track<number_of_tracks;track++){
 parameter = 1; //8 parameters / cells for each track: 0 steps, 1 length, 2 selected track
 eeprom_offset += cell_offset * parameter; //add an offset to make room in the eeprom for all the parameters
 eeprom_address = eeprom_offset * track;
 
 eeprom(number_of_steps[track], eeprom_address + track, bank); //write settings to eeprom - seq[track 0-3][step 0-7] | eeprom(value 0-1, address 0-1023, bank 0-7)
 }
*/


#endif 

}

//-----------------------
void internalClock(){ //the main clock that takes care of the counting
 if(use_internal_clock == true){ //check if internal clock is enabled
  
    unsigned long currentMillis = millis();
     
    if((clock_state == HIGH)  && (currentMillis - previousMillis >= onTime)) //check if the the tempo interval is reached //(millis() % onTime == 0)) /*//old routine*/ 
    {
      clock_state = LOW;  // Turn it off
      previousMillis = currentMillis;  // Remember the time
      digitalWrite(clock_indicator, clock_state);  // Update the actual LED     
    }
    else if ((clock_state == LOW) && (currentMillis - previousMillis >= offTime)) //(timer(offTime))) //(millis() % offTime == 0)) /*(currentMillis - previousMillis >= offTime)) //old routine*/
    {
      clock_state = HIGH;  // turn it on
      for(int i=0; i<number_of_tracks;i++){
      counter(i); //increment the counter
	  }
      previousMillis = currentMillis;   // Remember the time
      digitalWrite(clock_indicator, clock_state);   // Update the actual LED
      
      oled8x8(); //update the oled on each new clock tick
    }
  }
  
  
  
  
 }


//the main counter function that takes care of all the counting
void counter(int C){
	if(count[C] >= number_of_steps[C]-1) count[C] = 0;
        else {
      count[C]++; //increment the counter
      } 
  }
  
  
  
//---------------------

//this part reads the push buttons and determines which track should be active
void track_selector(){ 
  for(int i=0;i<number_of_step_switches;i++){ //read through the step switches 
  track_selector_state = digitalRead(track_selector_switch);
  buttonState[i] = digitalRead(buttonPin[i]);  // read the pushbutton input pin:

  if (digitalRead(track_selector_switch) == pressed && buttonState[i] == pressed && debounce_f(time2, debounce_var) && i<number_of_tracks) {   // compare the buttonState to its previous state and see if it is pressed
    selected_track = i; 
    time2 = millis();    
  }
  //else selected_track = 0;
 }
}

//debounce function to prevent unintended button presses
int debounce_f(unsigned long TIME, unsigned long DEBOUNCE_VAR){
	bool debounce_state = 0;
	//TIME = 0;
	if(millis() - TIME > DEBOUNCE_VAR){ 
	debounce_state = 1;
	//TIME = millis(); //this part is done from the code calling the function
	}
	else debounce_state = 0;
	
	return debounce_state;
}

  
//------------------- 
//this functions takes the states of the step switches and puts them in to the sequences of the tracks
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






//debug function using the serial monitor
void debug(char TEKST[], int INFO){
  Serial.print(TEKST);
  Serial.println(INFO); 
  }

//change the length of each track
int track_length(){ 
 for(int i=0;i<number_of_step_switches;i++){
  if(length_selector_state == pressed && digitalRead(buttonPin[i]) == pressed) number_of_steps[selected_track] = i+1; //set the track length of the selected track to the number represented by the pressed switch (+1)
   }
   return number_of_steps[selected_track];
  
}


//------------ The output gates work by AND'ing the active step with the clock. This way the output pins are only HIGH when both clock AND step is 1
//This makes it possible to make a global trigger length and is also easily adaptable to using external analog clock 
void trigger_with_and(int OUT, int TRACK, int COUNT){
  digitalWrite(ledPin[OUT], seq[TRACK][COUNT] && clock_state);
 
	}
//////////////

//write to the OLED screen
void oled8x8(){
  
  //TRACK
  u8x8.setCursor(6, 0);
  u8x8.print(selected_track+1, DEC);

  
  //LENGTH
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
  
  //write the content of the eeprom to the oled  
  for(int i=0;i<8;i++){
  u8x8.setCursor(offset+(i*space), 30);
  //int eeprom_address = eeprom_offset * selected_track;
  u8x8.print(EEPROM.read(selected_track * EE.eeprom_address(selected_track) + i));
    //u8x8.print(EEPROM.read(temp_track * eeprom_offset + i));
    }  
      
 /*   
  for(int i=0;i<number_of_tracks;i++){
  u8x8.setCursor(offset+(i*space), 30);
  u8x8.print(count[i]+1, DEC);
    }   
    */
    
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
  /*
  u8x8.setCursor(offset+(3*space), 35);
  u8x8.print(128, BIN);
  */
  
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
    if(boolArray[i] == 1) result = result | (1 << i);
    if(boolArray[i] == 0) result &= ~(1 << i);

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

#ifdef eepromf
void eeprom(int val, int address, int bank) {
		
  /***
    Write the value to the appropriate byte of the EEPROM.
    these values will remain there when the board is
    turned off.
  ***/
if( EEPROM.read(address) != val ){
      EEPROM.write(address, val);
    }


  if (address == EEPROM.length()) {
    address = 0;
  }

  /***
    As the EEPROM sizes are powers of two, wrapping (preventing overflow) of an
    EEPROM address is also doable by a bitwise and of the length - 1.

    ++addr &= EEPROM.length() - 1;
  ***/
/*
	int track_4_address = 128 * 3;
    for(int i=0;i<8;i++){
    if( EEPROM.read(track_4_address) != val ){
      EEPROM.write(track_4_address+i, 9);
    }
    
   }*/
	}
	
void zeros_in_eeprom(){ //fill the eeprom with zeros to make debugging easier and to ensure the eeprom doesn't contain corrupted data
    byte val = 0;
    
    for(address=0;address<EEPROM.length();address++){
    if( EEPROM.read(address) != val ){
      EEPROM.write(address, val);
    }


  if (address == EEPROM.length()) {
    address = 0;
  }
 }

}
#endif
