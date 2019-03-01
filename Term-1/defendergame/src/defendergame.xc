/////////////////////////////////////////////////////////////////////////////////////////
//
// COMS20001
// CODE SKELETON FOR X-CORE200 EXPLORER KIT
// TITLE: "Console Ant Defender Game"
// Rudimentary game combining concurrent programming and I/O.
//
/////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <print.h>
#include <xs1.h>
#include <platform.h>

on tile[0] : in port buttons = XS1_PORT_4E; //port to access xCore-200 buttons
on tile[0] : out port leds = XS1_PORT_4F;   //port to access xCore-200 LEDs

/////////////////////////////////////////////////////////////////////////////////////////
//
//  Helper Functions provided for you
//
/////////////////////////////////////////////////////////////////////////////////////////

//DISPLAYS an LED pattern
int showLEDs(out port p, chanend fromVisualiser) {
  int pattern; //1st bit...separate green LED
               //2nd bit...blue LED
               //3rd bit...green LED
               //4th bit...red LED
  while (1) {
    fromVisualiser :> pattern;   //receive new pattern from visualiser
    p <: pattern;                //send pattern to LED port
  }
  return 0;
}

//READ BUTTONS and send button pattern to userAnt
void buttonListener(in port b, chanend toUserAnt) {
  int r;
  while (1) {
    b when pinseq(15)  :> r;    // check that no button is pressed
    b when pinsneq(15) :> r;    // check if some buttons are pressed
    if ((r==13) || (r==14))     // if either button is pressed
    toUserAnt <: r;             // send button pattern to userAnt
  }
}

//WAIT function
void waitMoment() {
  timer tmr;
  int waitTime;
  tmr :> waitTime;                       //read current timer value
  waitTime += 40000000;                  //set waitTime to 0.4s after value
  tmr when timerafter(waitTime) :> void; //wait until waitTime is reached
}

//PRINT WORLD TO CONSOLE
void consolePrint(unsigned int userAntToDisplay,
                  unsigned int attackerAntToDisplay) {
  char world[25]; //world of size 23, plus 1 byte for line return
                  //                  plus 1 byte for 0-termination

  //make the current world string
  for(int i=0;i<23;i++) {
    if ((i>7) && (i<15)) world[i]='-';
    else world[i]='.';
    if (userAntToDisplay==i) world[i]='X';
    if (attackerAntToDisplay==i) world[i]='O';
  }
  world[23]='\n';  //add a line break
  world[24]=0;     //add 0-termination
  printstr(world); //send off to console
}

//PROCESS THAT COORDINATES DISPLAY
void visualiser(chanend fromUserAnt, chanend fromAttackerAnt, chanend toLEDs) {
  unsigned int userAntToDisplay = 11;
  unsigned int attackerAntToDisplay = 2;
  int pattern = 0;
  int round = 0;
  int distance = 0;
  int dangerzone = 0;
  while (1) {
    if (round==0) printstr("ANT DEFENDER GAME (press button to start)\n");
    round++;
    select {
      case fromUserAnt :> userAntToDisplay:
        consolePrint(userAntToDisplay,attackerAntToDisplay);
        break;
      case fromAttackerAnt :> attackerAntToDisplay:
        consolePrint(userAntToDisplay,attackerAntToDisplay);
        break;
    }
    distance = userAntToDisplay-attackerAntToDisplay;
    dangerzone = ((attackerAntToDisplay==7) || (attackerAntToDisplay==15));
    pattern = round%2 + 8 * dangerzone + 2 * ((distance==1) || (distance==-1));
    if ((attackerAntToDisplay>7)&&(attackerAntToDisplay<15)) pattern = 15;
    toLEDs <: pattern;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//  MOST RELEVANT PART OF CODE TO EXPAND FOR YOU
//
/////////////////////////////////////////////////////////////////////////////////////////

//DEFENDER PROCESS... The defender is controlled by this process userAnt,
//                    which has channels to a buttonListener, visualiser and controller
void userAnt(chanend fromButtons, chanend toVisualiser, chanend toController) {
  unsigned int userAntPosition = 11;       //the current defender position
  int buttonInput;                         //the input pattern from the buttonListener
  unsigned int attemptedAntPosition = 0;   //the next attempted defender position after considering button
  int moveForbidden;                       //the verdict of the controller if move is allowed
  toVisualiser <: userAntPosition;         //show initial position
  while (1) {
    fromButtons :> buttonInput; //expect values 13 and 14

    // Checking button input, and assigning attempted move for both left and right.
    if (buttonInput == 13){
      if (userAntPosition == 0){
        attemptedAntPosition = 22;
      }
      else{
      attemptedAntPosition = userAntPosition - 1;
      }
    }

    else if (buttonInput == 14){
      if (userAntPosition == 22){
        attemptedAntPosition = 0;
      }
      else{
      attemptedAntPosition = userAntPosition + 1;
      }
    }


    // Sending attempted move to controller, and receiving back a yes/no.
    toController <: attemptedAntPosition;
    toController :> moveForbidden;

    // If legal carry out attempted move, if not do nothing.
    if (!moveForbidden) {userAntPosition = attemptedAntPosition;}

    // Visualise.
    toVisualiser <: userAntPosition;
  }
}

//ATTACKER PROCESS... The attacker is controlled by this process attackerAnt,
//                    which has channels to the visualiser and controller
void attackerAnt(chanend toVisualiser, chanend toController) {
  int moveCounter = 0;                       //moves of attacker so far
  unsigned int attackerAntPosition = 2;      //the current attacker position
  unsigned int attemptedAntPosition;         //the next attempted  position after considering move direction
  int currentDirection = 1;                  //the current direction the attacker is moving
  int moveForbidden = 0;                     //the verdict of the controller if move is allowed
  int running = 1;                           //indicating the attacker process is alive
  toVisualiser <: attackerAntPosition;       //show initial position

  while (running) {

    // Logic to check divisible by numbers.
    if ((moveCounter % 31 == 0) || (moveCounter % 37 == 0)){
      currentDirection = (currentDirection * (-1));
    }


    // Creates a attempted position and sends to controller. Receive back a yes/no.
    attemptedAntPosition = attackerAntPosition + currentDirection;
    if(attemptedAntPosition == (-1)) attemptedAntPosition = 22;
    else if(attemptedAntPosition == 23) attemptedAntPosition = 0;
    toController <: attemptedAntPosition;

    toController :> moveForbidden;

    // If move is illegal then switch direction1.
    if (moveForbidden) {
      currentDirection = currentDirection * (-1);
      attemptedAntPosition = attackerAntPosition + 2*currentDirection;
    }

    // If legal, carry out attempted move and increment move.
     attackerAntPosition = attemptedAntPosition;
     moveCounter++;


    // Visualise..
    toVisualiser <: attackerAntPosition;
    waitMoment();
  }
}

//COLLISION DETECTOR... the controller process responds to �permission-to-move� requests
//                      from attackerAnt and userAnt. The process also checks if an attackerAnt
//                      has moved to winning positions.
void controller(chanend fromAttacker, chanend fromUser) {
  unsigned int lastReportedUserAntPosition = 11;      //position last reported by userAnt
  unsigned int lastReportedAttackerAntPosition = 2;   //position last reported by attackerAnt
  unsigned int attempt = 0;                           //incoming data from ants
  int gameEnded = 0;                                  //indicates if game is over
  fromUser :> attempt;                                //start game when user moves
  fromUser <: 1;                                      //forbid first move
  while (!gameEnded) {
    select {
      case fromAttacker :> attempt:
        if(attempt == lastReportedUserAntPosition){
          fromAttacker <: 1;
        }

        else{
          lastReportedAttackerAntPosition = attempt;
          if ((lastReportedAttackerAntPosition > 7) && (lastReportedAttackerAntPosition < 15)){
            gameEnded = 1;
          }
          fromAttacker <: 0;
        }
        break;

      case fromUser :> attempt:
        if (attempt == lastReportedAttackerAntPosition){
          fromUser <: 1;
        }

        else {
          lastReportedUserAntPosition = attempt;
          fromUser <: 0;
        }
        break;
    }
  }
}

//MAIN PROCESS defining channels, orchestrating and starting the processes
int main(void) {
  chan buttonsToUserAnt,         //channel from buttonListener to userAnt
       userAntToVisualiser,      //channel from userAnt to Visualiser
       attackerAntToVisualiser,  //channel from attackerAnt to Visualiser
       visualiserToLEDs,         //channel from Visualiser to showLEDs
       attackerAntToController,  //channel from attackerAnt to Controller
       userAntToController;      //channel from userAnt to Controller

  par {
    //PROCESSES FOR YOU TO EXPAND
    on tile[1]: userAnt(buttonsToUserAnt,userAntToVisualiser,userAntToController);
    on tile[1]: attackerAnt(attackerAntToVisualiser,attackerAntToController);
    on tile[1]: controller(attackerAntToController, userAntToController);

    //HELPER PROCESSES USING BASIC I/O ON X-CORE200 EXPLORER
    on tile[0]: buttonListener(buttons, buttonsToUserAnt);
    on tile[0]: visualiser(userAntToVisualiser,attackerAntToVisualiser,visualiserToLEDs);
    on tile[0]: showLEDs(leds,visualiserToLEDs);
  }
  return 0;
}
