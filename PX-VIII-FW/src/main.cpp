#include <Arduino.h>

#ifdef ESP8266
 #include <ESP8266WiFi.h>
#else //ESP32
 #include <WiFi.h>
#endif
#include <ModbusIP_ESP8266.h>

#include "SPI_shiftreg.h"

#include "ELRing.h"

#include "creds.h"
#include "ModeControl.h"
#include "WifiControl.h"

#include "patterns.h"

#define PX_NUM 8


//PX units mostly need just 1 register for the 5 states they can be in
//registers start at 101 for PX-I
#define PX_REG 100 + (PX_NUM * 10) //base operational modbus register
#define PX_MOD_REG PX_REG + 1 //a register for operational mode -> film or generative (random path)
#define PX_STATE_REG 200 + (PX_NUM * 10) // base status modbus register
//#define PX_REG 104
 enum
  {
    PX_ERR = -1,
    PX_OK
  };

  enum
  {
    PX_FILM_MODE,
    PX_GEN_MODE
  };

#define NUM_EL_RINGS 6 //includes the center button, which is just 1 unit...

#define NUM_EL_CARDS 8


ELRing rings[6];
ELRing *el_rings_ptr = rings;//new ELRing[6];
int16_t el_spiral_idx(uint8_t ring_n, uint8_t idx, ELRing *rings_ptr);

//OLD CONFIG
//SPI_shiftreg px8elb(D4, D2, D3, NUM_EL_CARDS);
//new config
SPI_shiftreg px8elb(D4, D3, D2, NUM_EL_CARDS * 2);
uint8_t pixel = 0;

void el_to_shiftreg(ELRing *rings_ptr, SPI_shiftreg *regs_ptr);
void clear_rings(void);

//ModbusIP object
ModbusIP pxModbus;

int16_t pxState = 0;
void setState(int16_t state);

int8_t pxRunMode = PX_FILM_MODE;
void setRunMode(int8_t run_mode);

//to select to prefab patterns
int8_t active_pattern = 0;
int8_t pattern_idx = 0;
int8_t pattern_len = 0;
uint32_t pattern_time = 0;
uint32_t pattern_period = 3000; //3 seconds per step

//previous generated location [ring, quadr, quadr_idx]
uint8_t prev_gen_loc[3] = {0, 0, 0};

void get_pattern_data(uint8_t *pattern, uint8_t idx, el_status_t action);
void move_same_ring(uint8_t move);
void move_ring(uint8_t move);

char ssid[] = SSID ;        // your network SSID (name)
char pass[] = PW;                    // your network password
WifiControl pxWifi(ssid, pass, PX_NUM);

#define DEMO_SW_PIN 3
int16_t demoState = 0;
void demoCallback(uint32_t dTime, px_mode_t mode);
ModeControl pxMC(DEMO_SW_PIN, &demoCallback, 1000, &pxWifi);

void setup() {
  Serial.begin(57600, SERIAL_8N1, SERIAL_TX_ONLY);

  delay(1000);


  pxWifi.setTimeOut(30000);
  Serial.println("Connecting to C&C...");
  int8_t res = pxWifi.init();
  if(res == -1){
    Serial.println("No C&C found, starting up in demo mode!");
  }
  

  //create the modbus server, and add a holding register (addHreg) and Ireg
  pxModbus.server(502);
  pxModbus.addHreg(PX_REG, 0);
  pxModbus.addIreg(PX_REG, 0);
  pxModbus.addHreg(PX_MOD_REG, 0);
  pxModbus.addIreg(PX_MOD_REG, 0);

  pxModbus.addHreg(PX_STATE_REG, PX_OK);


  for(uint8_t i = 0; i < NUM_EL_RINGS; i++){
    if(i > 0){
      (el_rings_ptr + i)->init(i * 8, i);
    }else{
      (el_rings_ptr + i)->init(1, 0);
    }
  }
  //turn on center el
  el_rings_ptr->set_el(0, ACT);
  
  el_to_shiftreg(el_rings_ptr, &px8elb);
  px8elb.shift_data();

  px8elb.enable_output();

  
  pxMC.init();

  Serial.println("Setup complete");
}

void loop() {

  //Call once inside loop() - all magic here
  pxModbus.task();

  pxMC.run();

  pxWifi.run();

  //this should  copy the holding reg value to ireg
  if(pxModbus.Hreg(PX_REG) != pxModbus.Ireg(PX_REG)){
    pxModbus.Ireg(PX_REG, pxModbus.Hreg(PX_REG));
  }

   if(pxModbus.Hreg(PX_MOD_REG) != pxModbus.Ireg(PX_MOD_REG)){
    pxModbus.Ireg(PX_MOD_REG, pxModbus.Hreg(PX_MOD_REG));
  }
  if(pxRunMode != pxModbus.Ireg(PX_MOD_REG)){
    setRunMode((int8_t)pxModbus.Ireg(PX_MOD_REG));
  }

  if(pxState != pxModbus.Ireg(PX_REG)){
   
    setState((int16_t)pxModbus.Ireg(PX_REG));
  }

  

}

void setState(int16_t state)
{
  
  if(pxRunMode == PX_FILM_MODE){
    
    el_status_t action;

    if((uint32_t)state > (sizeof(film_pattern0) / 3) - 1){
      //check for valid value of pxState otherwise return
        return;
    }
    
    if(state > pxState){
      action = ACT;
      
      for(uint8_t i = pxState; i < state; i++){
        get_pattern_data(film_pattern0[0], i, action);
      }
    }else if(state < pxState){
      action = INACT;
      
      for(uint8_t i = pxState; i >= state; i--){
        get_pattern_data(film_pattern0[0], i, action);
      }
    }
    
    
    

  }else if(pxRunMode == PX_GEN_MODE){
    //previous position onthouden
    //[ring, quadr, quadr_idx]
   
    //0 ->geen verandering
    //1 -> zelfde ring, 1 naar links
    //2 -> zelfde ring, 1 naar r
    //3 -> hogere ring, zelfde pos
    //4 -> lagere ring, zelfde pos
    //
    
      Serial.println("GEN MODE SET STATE");
      if(state == 0){
        clear_rings();
        prev_gen_loc[0] = 0;
        prev_gen_loc[1] = 0;
        prev_gen_loc[2] = 0;
        pxState = state;
        return;
      }
     uint32_t randNumber = RANDOM_REG32;
     uint8_t move = randNumber % 9;

     //detect if only origin is on, exclude same / lower ring left / right moves
     //set previous location to 'fake' spot in ring 1
     if(prev_gen_loc[0] == 0 && move > 0){
       move = 3;
       prev_gen_loc[0] = 1;
       prev_gen_loc[1] = randNumber % 7;
       prev_gen_loc[2] = 0;
       //go on to pick a random spot in ring 1!
       randNumber = RANDOM_REG32;
       move = 1 + (randNumber % 2); 
     }
     Serial.print("move: ");
     Serial.println(move);
     switch(move){
        case 0: //no change
          break;
        case 1: //same ring, right 1
          move_same_ring(move);
          break;
        case 2: //same ring, left 1
          move_same_ring(move);
          break;
        case 3:
          move_ring(move);
          break;
        case 4:
          move_ring(move);
          break;
        case 5:
          move_same_ring(1);
          break;
        case 6:
          move_same_ring(2);
          break;
        case 7:
          move_ring(3);
          break;
        case 8:
          move_ring(4);
        default:
          break;
     }


  }

  px8elb.shift_data();
  pxState = state;
  
  

}

void setRunMode(int8_t run_mode){
  //change run mode and clear the rings
  Serial.println("setting run mode");
  pxRunMode = run_mode;
  clear_rings();
  //set holding reg and pxState to zero
  pxModbus.Hreg(PX_REG, 0);
  pxState = 0;
  prev_gen_loc[0] = 0;
  prev_gen_loc[1] = 0;
  prev_gen_loc[2] = 0;

}

void clear_rings(void)
{
  
  for(uint8_t i = 1; i < NUM_EL_RINGS; i ++){
    (el_rings_ptr + i)->turn_off_ring();
  }

  
  el_to_shiftreg(el_rings_ptr, &px8elb);
  px8elb.shift_data();



}



void get_pattern_data(uint8_t *pattern, uint8_t idx, el_status_t action)
{
  uint8_t ring = 0;
  uint8_t quadr = 0;
  uint8_t quadr_idx = 0;
  idx *= 3;
  ring = *(pattern + idx);
  

  quadr = *(pattern + idx + 1);
  
  quadr_idx = *(pattern + idx + 2);
  
  ELRing *ring_ptr;
  ring_ptr = (el_rings_ptr + ring);
  ring_ptr->set_el(quadr, quadr_idx, action);
  el_to_shiftreg(el_rings_ptr, &px8elb);
}

void move_same_ring(uint8_t move)
{

  uint8_t prev_ring = prev_gen_loc[0];
  uint8_t prev_quadr = prev_gen_loc[1];
  uint8_t prev_idx = prev_gen_loc[2];

  
  uint8_t n_els_per_quadr = (el_rings_ptr + prev_ring)->els_per_quadrant();

  uint8_t idx = prev_idx;
  uint8_t quadr = prev_quadr;

  switch(move){
    case 1: //move right
      idx += 1;
      if(idx > n_els_per_quadr - 1){
        idx = 0;
        quadr += 1;
        if(quadr > NUM_QUADRANTS - 1){
          quadr = 0;
        }
      }
    break;
    case 2: // move left
      if(prev_idx == 0){
        if(quadr == 0){
          quadr = NUM_QUADRANTS - 1;
        }else{
          quadr -= 1;
        }
        idx = n_els_per_quadr - 1;
      }else{
        idx -= 1;
      }
    break;
    default:
      break;
  }
  Serial.print("quadr: ");
  Serial.println(quadr);
  Serial.println("idx: ");
  Serial.println(idx);
  prev_gen_loc[1] = quadr;
  prev_gen_loc[2] = idx;
  (el_rings_ptr + prev_ring)->set_el(quadr, idx, ACT);
  el_to_shiftreg(el_rings_ptr, &px8elb);

}

void move_ring(uint8_t move)
{
  uint8_t prev_ring = prev_gen_loc[0];
  uint8_t prev_quadr = prev_gen_loc[1];
  uint8_t prev_idx = prev_gen_loc[2];

  

  uint8_t ring = prev_ring;
  uint8_t idx = prev_idx;
  uint8_t quadr = prev_quadr;

  if(move < 4){
    ring += 1;
  }else{
    if(ring > 0){
      ring -= 1;
    }
  }
  
  if(ring  > NUM_EL_RINGS - 1 ){
    ring = 5; //max ring
  }
  prev_gen_loc[0] = ring;

  (el_rings_ptr + ring)->set_el(quadr, idx, ACT);
        el_to_shiftreg(el_rings_ptr, &px8elb);

  
}

//returns the index of in the hardware spiral of and EL button based on ring number and index in that ring
int16_t el_spiral_idx(uint8_t ring_n, uint8_t idx, ELRing *rings_ptr){

      int16_t n_elbs = 0;

      for(uint8_t i = 0; i < ring_n; i++){
        n_elbs += (rings_ptr + i)->get_el_num();
      }
      n_elbs += idx;
      return n_elbs;
}

//shifts data from EL rings to the shiftregs
void el_to_shiftreg(ELRing *rings_ptr, SPI_shiftreg *regs_ptr)
{
  for(uint8_t i = 0; i < NUM_EL_RINGS; i++){
    uint8_t n_els_ring = (rings_ptr + i)->get_el_num();

    for(uint8_t j = 0; j < n_els_ring; j++){
      int16_t el_idx = el_spiral_idx(i, j, rings_ptr);
      
      el_status_t el_st = (rings_ptr + i)->get_el_status(j);
      if(el_st == ACT){
        
        regs_ptr->set_data_bit(el_idx, 1);
      }else if(el_st == INACT){
        regs_ptr->set_data_bit(el_idx, 0);
      }
    }

  }
}


void demoCallback(uint32_t dTime, px_mode_t mode)
{
  
 if(mode == PX_DEMO_MODE){
    if(demoState == 0){
      
      pxModbus.Hreg(PX_MOD_REG, PX_GEN_MODE);
    }
    if(demoState < 350){
      demoState++;
      pxModbus.Hreg(PX_REG, demoState);
      
    }else{
      demoState = 0;
      pxModbus.Hreg(PX_REG, 0);
    }
  }else if(mode == PX_CC_MODE){
    Serial.println("to CC mode demo calback");
    
    demoState = 0;
    pxModbus.Hreg(PX_REG, 0);
    pxModbus.Hreg(PX_MOD_REG, PX_GEN_MODE);
    clear_rings();
  }
}

