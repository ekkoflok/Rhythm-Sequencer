/*//debounce/ millis function
//Variables related to the step switches
long time = 0;         // the last time the output pin was toggled
long debounce = 20;   // the debounce time, increase if the output flickers
int debounce_var = 200;

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


time2 = millis();  
*/

class Time
{
//variables
unsigned long time;         // the last time the output pin was toggled
unsigned long timer_var; // the debounce time, increase if the output flickers

bool timer_state;

public:
    Time()
    {
    timer_var = 200; 
    timer_state = LOW; 
	time = 0;
	
    }
    
    bool update_time(unsigned long TIMER_VAR){
	    //bool timer_state = 0;
	    
        unsigned long current_time = millis();
	    if(current_time - time >= TIMER_VAR){ 
	    time = current_time;
	    timer_state = 1;
	    //TIME = millis(); //this part is done from the code calling the function
	    }
	    else timer_state = 0;
	
	    return timer_state;
}    

};

Time T; //initialize


int state = 0;
int state2 = 0;
int state3 = 0;
int led = 17;
int count = 0;
void setup()
{
    pinMode(17, OUTPUT);
    pinMode(9, OUTPUT);
    pinMode(8, OUTPUT);
}

void loop()
{
if(T.update_time(50))
{
count ++;
}

if( (count % 5 == 0 ) ){state = 1; }
else state = 0;

if( (count % 7 == 0 ) ){state2 = 1; }
else state2 = 0;

if( (count % 2 == 0 ) ){state3 = 1; }
else state3 = 0;

digitalWrite(led, (state) );
digitalWrite(9, (state2) );
digitalWrite(8, (state3) );

}
