class EEprom
{
//eeprom variables
int address = 0;
int eeprom_offset = 128; //was 8;
int eeprom_parameter[8] = {0}; //eight slots of sixteen bytes each: 0 pattern, 1 length, 2 selected track, 3 div/mult, 4 random, 5 empty, 6 empty, 7 active bank 
int eeprom_bank[8] = {}; //eight banks of 128 bytes each
int cell_offset = 16; //16 bytes for each "cell", 8 cells for each track: 0 steps, 1 length, 2 selected track

};

//EEPROM read

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
  for(int track = 0; track<number_of_tracks; track ++){
			//int track = 0;//
			//int eeprom_address = eeprom_offset * track;
	  		for(int i=0;i<8;i++){ //should change this to a variable
			seq[track][i] = EEPROM.read(track * eeprom_address(track) + i); //read the recorded settings for each step on each track
		    }
	}

int eeprom_address(int t){
    int eeprom_adr = eeprom_offset * t;
    return eeprom_adr;
}	


	
int settings_eeprom(int tracks){
 //eeprom write function
 byte bank = 8; //8 banks, each with 128 bytes of memory for saving settings
 //byte track = 0;
 int eeprom_address = 0;
 byte parameter = 0; //8 parameters / cells for each track: 0 steps, 1 length, 2 selected track

 ///////////step patterns
 for(int track=0;track<tracks/*number_of_tracks*/;track++){
 byte parameter = 0; //8 parameters / cells for each track: 0 steps, 1 length, 2 selected track
 //eeprom_offset += cell_offset * parameter; //add an offset to make room in the eeprom for all the parameters
 eeprom_address = eeprom_offset * track;
 for(int step = 0; step<number_of_step_switches; step++){  //Save the current state of the switches to next power on
   //for(int j = 0; j<8; j++){
 eeprom(seq[track][step], eeprom_address * track + step, bank); //write settings to eeprom - seq[track 0-3][step 0-7] | eeprom(value 0-1, address 0-1023, bank 0-7)
     }
   }


}

//OLED
 //write the content of the eeprom to the oled  
  for(int i=0;i<8;i++){
  u8x8.setCursor(offset+(i*space), 30);
  int eeprom_address = eeprom_offset * selected_track;
  u8x8.print(EEPROM.read(selected_track * eeprom_address + i));
    //u8x8.print(EEPROM.read(temp_track * eeprom_offset + i));
    } 
    
void eeprom(int val, int address, int bank) { //write to eeprom
		
if( EEPROM.read(address) != val ){
      EEPROM.write(address, val);
    }


  if (address == EEPROM.length()) {
    address = 0;
  }

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
