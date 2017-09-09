/*
Copyright 2014 Efstathios Lyberidis & Thodoris Bais

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


/////////////// Master Arduino /////////



#include "StopWatch.h"
#include <Wire.h>
#include "stdlib.h"


//define the total number of turns
#define nrofturns 8

// initialize a stopwatch
StopWatch MySW;

// player switches unlocking or locking their buttons
//needs volatile for interrupt visibility and modification from interrupt
//perhaps shot int would be better.
//values: 0-> input not accepted from player( key locked)
//values: 1-> input is accepted
//variable changed from: game states & interrupt
volatile int player_sw1=0;
volatile int player_sw2=0;
volatile int player_sw3=0;
volatile int player_sw4=0;

// variables to be updated by the interrupt
// required to see which button was pressed
// all buttons use the same interrupt
// we use x? to mark the button to see if a button has been pressed
// values: 0 or 1 
// modified by digitalread
volatile int x1 = 0;
volatile int x2 = 0;
volatile int x3 = 0;
volatile int x4 = 0;


//variables used for the turns
// rand_unique ensures that ever 4 turns all colors are used 
// so nobody will be unhappy
boolean rand_unique=false ;
volatile int player_turn=1;
int rands[]={-1,-1,-1,-1};
volatile int k =0;
volatile int game_turn=0;
int tmp=0;
int tmp2=0;

// array to identify player pressed to send the data to the other arduino
// this variable could be changed to an integer value [1,2,3,4] for player number
// array send via serial to other arduino
int playerpressed[]={0,0,0,0}; 

// the players' buttons pins
int button1pin = 6;
int button2pin = 5;
int button3pin = 7;
int button4pin = 8;

// the rgb led pins
int pin_rgb_red = 9;
int pin_rgb_green = 10;
int pin_rgb_blue = 11;

// ints for the score
//needs volatile because interrupt modifies it
volatile int c1,c2,c3,c4=0;

// send signal for win / lose sounds
// values=0 no key pressed
// values=1 key corrent
// values=2 key wrong
int winorlose=0;


void setup() {                
  pinMode(pin_rgb_red, OUTPUT);
  pinMode(pin_rgb_blue, OUTPUT);
  pinMode(pin_rgb_green, OUTPUT);
  
  pinMode(button1pin, INPUT);
  pinMode(button2pin, INPUT);
  pinMode(button3pin, INPUT);
  pinMode(button4pin, INPUT);

  //pin 2, players' buttons interrupt
  attachInterrupt(0, checkButton, RISING);

  //turn on communication between the 2 arduinos
  Wire.begin();
  
  //turn on serial communication
  Serial.begin(9600);  
  
  //Seed the random function
  randomSeed(analogRead(0));
}

void loop() {
  
  //buffer delay	
  //50 msec after game turn in order to allow the buffer to clean
  delay(50);
  
  //Communication between arduinos
  //Values: 0
  //		-1 	stage1	  (init & plays demo music)
  //		-2  stage 2.1 (plays a beep sound before led & light led)
  //		-3  stage 2.2a (correct button pressed)
  //		-4  stage 2.2b (wrong button pressed)
  //		-5  stage 3		(end of game high score printing)
  //		+1	player 1 (after a -3 or -4)
  //		+2  player 2 (--<<--)
  //		+3  player 3 (--<<--)
  //		+4  player 4 (--<<--)
  //		+5  all players scored will be transmitted
  //		e.g.	+1 <score> +2 <score> +3 <score> +4 <score> -5
  //		
  
  //Game uses an FSM of 4 stages
  
  if(game_turn==0)
  {
  	// Game initialization,  Stage 1
  	/// buttons disabled (aside restart)
  	/// rgb closed
  	/// game starting music
  	
    player_sw1=0;
    player_sw2=0;
    player_sw3=0;
    player_sw4=0;
    c1=0;
    c2=0;
    c3=0;
    c4=0;
    
    digitalWrite(pin_rgb_red, HIGH);
    digitalWrite(pin_rgb_green,HIGH);
    digitalWrite(pin_rgb_blue, HIGH);

	//I2C from TWI (two wire interface) at pins 4 ( Serial Data Line (SDA)) (pin 5 is  Serial Clock Line (SCL))
    Wire.beginTransmission(4);
	// use of (+) to declare a user
	// use of (-) to declare a game stage
	Wire.write("-1"); //game start
  	Wire.endTransmission();
  	//lets play the music from the 2n arduino
    delay(10000);
    
  }
  //define  rounds!
  else if(game_turn>0 && game_turn< nrofturns)
  {
  	// Main game,  Stage 2
  	/// buttons enabled
  	/// rgb randomly gets a value asigned to a player
  	/// short delay for the button press
  	/// turn results
  	/// results send to screen, one player each turn, all the players at the ending stage
  	/// preturn-music before the rgb lights, win/loss music after a button is pressed
  	
  	//Init our new following 4 colors sequence to -1
    if(k==4){
       k=0; 
       for (int i = 0; i < 4; i = i + 1) {
         rands[i]=-1;
       }
    }
    rand_unique=false;
    player_turn = random(4);
    
    //We ensure that only one out of four for every player
    while (rand_unique==false)
    {
      rand_unique=true;
      for(int u=0 ; u<4 ; u++)
      {
        if(rands[u]==player_turn)
        {
          player_turn = random(4);
          rand_unique=false;
        }
      }
    }
    

	delay(50);
  	Wire.beginTransmission(4);
	// use of (+) to declare a user
	// use of (-) to declare a game stage
	Wire.write("-2"); //turn begin
  	Wire.endTransmission();
  	
  	//wait for other arduino to perform
  	delay(1200);
  	
    rands[k]=player_turn;
    
    //k measures the step inside the 4-colors-sequence
    k++;
    
    //stop watch to measure user reaction
    MySW.reset();
    MySW.start();
    player_sw1=1;
    player_sw2=1;
    player_sw3=1;
    player_sw4=1;
    
    
    //Show the appropriate color
    if(player_turn==1){
      digitalWrite(pin_rgb_blue, HIGH);
      analogWrite(pin_rgb_red, 140);
      digitalWrite(pin_rgb_green, LOW);
    }else if(player_turn == 0){
      digitalWrite(pin_rgb_red, HIGH);
      digitalWrite(pin_rgb_green,HIGH);
      digitalWrite(pin_rgb_blue, LOW);
    }else if(player_turn == 2)
    {
      digitalWrite(pin_rgb_red, LOW);
      digitalWrite(pin_rgb_green,HIGH);
      digitalWrite(pin_rgb_blue, HIGH);
    }else if(player_turn == 3)
    {
      digitalWrite(pin_rgb_red, HIGH);
      digitalWrite(pin_rgb_green,LOW);
      digitalWrite(pin_rgb_blue, HIGH);
    }
    
    
    //Never ending loop. Value is changed by interrup, and performs loop exit
    tmp=0;
    tmp2=0;
    while(winorlose==0){
    	//delay(100);
    	tmp=tmp+1;
    	
    	//after reaching this value, just skip it , nobody pressed smth
    	if (tmp==32767) {tmp2=tmp2+1;} 
    	if (tmp2==100){winorlose=2;playerpressed[player_turn]=1; }
    }
    //key pressed. Stop watchdog
    //interrupt modifed winorlose with 1 (correct) or 2  (wrong)
	MySW.stop();


	//Turn off rgb
	digitalWrite(pin_rgb_red, 200);
    digitalWrite(pin_rgb_green,150);
    digitalWrite(pin_rgb_blue, 150);
	
	if(winorlose==1){ //winorlose==1 when the correct button was pushed
		winorlose=0;
		delay(50);
		Wire.beginTransmission(4);
		// use of (+) to declare a user
		// use of (-) to declare a game stage
		Wire.write("-3"); //correct
	  	Wire.endTransmission();
	  	delay(750);
	}
	if(winorlose==2){ //winorlose==2 when the wrong button was pushed
		winorlose=0;
		delay(50);
	  	Wire.beginTransmission(4);
		// use of (+) to declare a user
		// use of (-) to declare a game stage
		Wire.write("-4"); //false
	  	Wire.endTransmission();
	  	delay(750);
	}
	
	//The second step is to send the player number  +score
	//
	//
	//lets see who pressed the value. In the array only one will have a '1' at their possition
	
	if(playerpressed[0]==1){
		delay(50);
		Wire.beginTransmission(4);
		// use of (+) to declare a user
		// use of (-) to declare a game stage
		Wire.write("+1"); //send for 1st player
	  	Wire.endTransmission();
	  	delay(50);
	  	Wire.beginTransmission(4);
	  	char score1[4];
	  	itoa(c1,score1,10);
		Wire.write(score1); //send 1st player score
	  	Wire.endTransmission();
	  	playerpressed[0]=0;
	  	delay(50);
	  	
	}else if(playerpressed[1]==1){
		delay(50);
		Wire.beginTransmission(4);
		// use of (+) to declare a user
		// use of (-) to declare a game stage
		Wire.write("+2"); //send for 2nd player
	  	Wire.endTransmission();
	  	delay(50);
	  	Wire.beginTransmission(4);
	  	char score2[4];
	  	itoa(c2,score2,10);
		Wire.write(score2); //send 2nd player score
	  	Wire.endTransmission();
	  	playerpressed[1]=0;
	  	delay(50);
	  	
	}else if(playerpressed[2]==1){
		delay(50);
		Wire.beginTransmission(4);
		// use of (+) to declare a user
		// use of (-) to declare a game stage
		Wire.write("+3"); //send for 3rd player
	  	Wire.endTransmission();
	  	delay(50);
	  	Wire.beginTransmission(4);
	  	char score3[4];
	  	itoa(c3,score3,10);
		Wire.write(score3); //send 3rd player score
	  	Wire.endTransmission();
	  	playerpressed[2]=0;
	  	delay(50);
	  	
	}else if(playerpressed[3]==1){
		delay(50);
		Wire.beginTransmission(4);
		// use of (+) to declare a user
		// use of (-) to declare a game stage
		Wire.write("+4"); //send for 4th player
	  	Wire.endTransmission();
	  	delay(50);
	  	Wire.beginTransmission(4);
	  	char score4[4];
	  	itoa(c4,score4,10);
		Wire.write(score4); //send 4th player score
	  	Wire.endTransmission();
	  	playerpressed[3]=0;
	  	delay(50);
	  	
	}
  }else if(game_turn == nrofturns + 1)
  {
  	// End game,  Stage 3
  	/// buttons disabled (aside restart)
  	/// display scores for all players
  	/// play winner music
  	
    player_sw1=0;
    player_sw2=0;
    player_sw3=0;
    player_sw4=0;
    
    delay(50);
    Wire.beginTransmission(4);
	// use of (+) to declare a user
	// use of (-) to declare a game stage
	Wire.write("+5"); //prepare to send for all players
  	Wire.endTransmission();
  	delay(150);
  	
    Wire.beginTransmission(4);
	// use of (+) to declare a user
	// use of (-) to declare a game stage
	Wire.write("+1"); //send for 1st player
  	Wire.endTransmission();
  	delay(50);
  	Wire.beginTransmission(4);
  	char finalscore1[4];
  	itoa(c1,finalscore1,10);
	Wire.write(finalscore1); //send 1st player score
  	Wire.endTransmission();
    delay(50);
        
    Wire.beginTransmission(4);
	// use of (+) to declare a user
	// use of (-) to declare a game stage
	Wire.write("+2"); //send for 2nd player
  	Wire.endTransmission();
  	delay(50);
  	Wire.beginTransmission(4);
  	char finalscore2[4];
  	itoa(c2,finalscore2,10);
	Wire.write(finalscore2); //send 2nd player score
  	Wire.endTransmission();
  	delay(50);
  	
  	Wire.beginTransmission(4);
	// use of (+) to declare a user
	// use of (-) to declare a game stage
	Wire.write("+3"); //send for 3nd player
  	Wire.endTransmission();
  	delay(50);
  	Wire.beginTransmission(4);
  	char finalscore3[4];
  	itoa(c3,finalscore3,10);
	Wire.write(finalscore3); //send 3nd player score
  	Wire.endTransmission();
  	delay(50);
  	
  	Wire.beginTransmission(4);
	// use of (+) to declare a user
	// use of (-) to declare a game stage
	Wire.write("+4"); //send for 4th player
  	Wire.endTransmission();
  	delay(50);
  	Wire.beginTransmission(4);
  	char finalscore4[4];
  	itoa(c4,finalscore4,10);
	Wire.write(finalscore4); //send 4th player score
  	Wire.endTransmission();
  	delay(50);
  
  	Wire.beginTransmission(4);
	// use of (+) to declare a user
	// use of (-) to declare a game stage
	Wire.write("-5"); //game end
  	Wire.endTransmission();
  	delay(50);
  	
  	//Lets skip the next if
    game_turn =nrofturns + 2;
  }
  
  if(game_turn<=nrofturns+1){
  	//next turn while there are turns
    game_turn += 1;
    
  }
}




//Interrupt handling function

void checkButton() {
	//Player button at interrupt 0 (pin 2)
	//if the buttons are enabled, read them to see which is pressed
	//calculate the score based on the turn  using the stopwatch 
	
	if(player_sw1==1){  
		player_sw1=0;
		x1 = digitalRead(button1pin);
		if(x1 == HIGH){
			if(player_turn==0){
				c1=c1+MySW.elapsed();
				winorlose=1;
			}else{
				c1=c1+2*MySW.elapsed();
				winorlose=2;
			}
			playerpressed[0]=1;
		}
		x1 = LOW;
	}
	
	if(player_sw2==1){ 
		player_sw2=0;
		x2 = digitalRead(button2pin);
		if(x2 == HIGH){
			if(player_turn==1){
				c2=c2+MySW.elapsed();
				winorlose=1;
			}else{
				c2=c2+2*MySW.elapsed();
				winorlose=2;
			}
			playerpressed[1]=1;
		}
		x2 = LOW;
	}
	
	if(player_sw3==1){ 
		player_sw3=0;
		x3 = digitalRead(button3pin);
		if(x3 == HIGH){
			if(player_turn==2){
				c3=c3+MySW.elapsed();
				winorlose=1;
			}else{
				c3=c3+2*MySW.elapsed();
				winorlose=2;
			}
			playerpressed[2]=1;
		}
		x3 = LOW;
	}
	if(player_sw4==1){ 
		player_sw4=0;
		x4 = digitalRead(button4pin);
		if(x4 == HIGH){
			if(player_turn==3){
				c4=c4+MySW.elapsed();
				winorlose=1;
			}else{
				c4=c4+2*MySW.elapsed();
				winorlose=2;
			}
			playerpressed[3]=1;
		}
		x4 = LOW;
	}
    
    return;
	
}
