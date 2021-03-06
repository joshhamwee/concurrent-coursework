// COMS20001 - Cellular Automaton Farm
// (using the XMOS i2c accelerometer demo code)

#include <platform.h>
#include <xs1.h>
#include <stdio.h>
#include "pgmIO.h"
#include "i2c.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define _OPEN_SYS_ITOA_EXT
#define  IMHT 16                                //image height
#define  IMWD 16                                //image width

on tile[0] : in port buttons = XS1_PORT_4E; //port to access xCore-200 buttons
on tile[0] : out port leds = XS1_PORT_4F;       //port to access xCore-200 LEDs

typedef unsigned char uchar;                    //using uchar as shorthand

uchar BLACK = 0xFF;
uchar WHITE = 0x00;

on tile[0]: port p_scl = XS1_PORT_1E;                       //interface ports to orientation
on tile[0]: port p_sda = XS1_PORT_1F;

#define FXOS8700EQ_I2C_ADDR 0x1E                //register addresses for orientation
#define FXOS8700EQ_XYZ_DATA_CFG_REG 0x0E
#define FXOS8700EQ_CTRL_REG_1 0x2A
#define FXOS8700EQ_DR_STATUS 0x0
#define FXOS8700EQ_OUT_X_MSB 0x1
#define FXOS8700EQ_OUT_X_LSB 0x2
#define FXOS8700EQ_OUT_Y_MSB 0x3
#define FXOS8700EQ_OUT_Y_LSB 0x4
#define FXOS8700EQ_OUT_Z_MSB 0x5
#define FXOS8700EQ_OUT_Z_LSB 0x6

//#define SetBit(A,k,m)     ( A[(k/8)][m] |= (1 << (k%8)) )
//#define ClearBit(A,k,m)   ( A[(k/8)][m] &= ~(1 << (k%8)) )
//#define TestBit(A,k,m)    ( A[(k/8)][m] & (1 << (k%8)) )

void SetBitFULL( uchar A[IMWD/8][IMHT],  int k , int m)
   {
      A[k/8][m] |= 1 << (k%8);  // Set the bit at the k-th position in A[i]
   }

void  ClearBitFULL( int A[IMWD/8][IMHT],  int k ,int m)
   {
      A[k/8][m] &= ~(1 << (k%8));
   }

int TestBitFULL( int A[IMWD/8][IMHT],  int k ,int m)
   {
      return ( (A[k/8][m] & (1 << (k%8) )) != 0 ) ;
   }

void SetBitQUAD( uchar A[IMWD/8 + 2][IMHT/2 + 2],  int k , int m)
   {
      A[k/8][m] |= 1 << (k%8);  // Set the bit at the k-th position in A[i]
   }

void  ClearBitQUAD( int A[IMWD/8 + 2][IMHT/2 + 2],  int k ,int m)
   {
      A[k/8][m] &= ~(1 << (k%8));
   }

int TestBitQUAD( int A[IMWD/8 + 2][IMHT/2 + 2],  int k ,int m)
   {
      return ( (A[k/8][m] & (1 << (k%8) )) != 0 ) ;
   }
/////////////////////////////////////////////////////////////////////////////////////////
//
// Read button press and send down correct channel
//
/////////////////////////////////////////////////////////////////////////////////////////
void buttonListener(in port b, chanend toDistributer) {
  int r;
  while (1) {
    b when pinseq(15)  :> r;
    b when pinsneq(15) :> r;    // check if some buttons are pressed
    if ((r==14)){               // if button is pressed
        toDistributer <: 14;             // send buttonpressed to distributer to read image
    }
    else if ((r==13)){
        toDistributer <: 13;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// Wait function. May be unnessacary later
//
/////////////////////////////////////////////////////////////////////////////////////////
void waitMoment() {
  timer tmr;
  int waitTime;
  tmr :> waitTime;                       //read current timer value
  waitTime += 40000000;                  //set waitTime to 0.7s after value
  tmr when timerafter(waitTime) :> void; //wait until waitTime is reached
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// Show LEDs corresponding to the game state
//
/////////////////////////////////////////////////////////////////////////////////////////
int showLEDs(out port p, chanend fromDistributer) {
  int pattern; //1st bit...separate green LED
               //2nd bit...blue LED
               //3rd bit...green LED
               //4th bit...red LED
  while (1) {
    fromDistributer :> pattern; //RECEIVE the LED's colour
    p <: pattern;               //SEND the LED's colour to the LED's
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// Read Image from PGM file from path infname[] to channel c_out
//
/////////////////////////////////////////////////////////////////////////////////////////
void DataInStream(chanend c_out)
{
  char infname[] = "test.pgm";                            //put your input image path here
  int res;
  uchar line[ IMWD ];
  printf( "DataInStream: Start...\n" );

  //Open PGM file
  res = _openinpgm( infname, IMWD, IMHT );
  if( res ) {
    printf( "DataInStream: Error openening %s\n.", infname );
    return;
  }

  //Read image line-by-line and send byte by byte to channel c_out
  for( int y = 0; y < IMHT; y++ ) {
      _readinline( line, IMWD );
      for( int x = 0; x < IMWD; x++ ) {
          if(line[x]==BLACK) c_out <: 1;
          else c_out <: 0;
      }
    }
  //Close PGM image file
  _closeinpgm();
  printf( "DataInStream: Done...\n" );
  return;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// This function sorts out the world overlap making it a closed system
//
/////////////////////////////////////////////////////////////////////////////////////////
int overlap(int i){
    if(i == -1) return IMWD - 1;
    else if(i == IMWD) return 0;
    else return i;
}

///////////////////////////////////////////////////////////////////////////////////////////////
//
// Game Logic happens within here
// A 3x3 matrix is passed into the function, and it returns if the middle cell is alive or dead
//
///////////////////////////////////////////////////////////////////////////////////////////////
uchar CellNewState(uchar workingWorld[3][3]){
    uchar currentCellState = workingWorld[1][1];
    int numberAliveSurrounding = 0;
    for(int y=0; y<3; y++){ //For loop to go through and check how many alive surrounding cells there are
        for(int x=0; x<3; x++){
            if(workingWorld[x][y] == BLACK) numberAliveSurrounding++;
        }
    }
    if(currentCellState == BLACK) numberAliveSurrounding = numberAliveSurrounding - 1;

    if (currentCellState == BLACK) { //Game logic to see if the cell in question changes state
        if(numberAliveSurrounding < 2) currentCellState = WHITE;
        else if(numberAliveSurrounding > 3) currentCellState = WHITE;
    }else{
        if(numberAliveSurrounding == 3) currentCellState = BLACK;
    }
    return currentCellState;
}


/////////////////////////////////////////////////////////////////////////////////////////
//
// Worker function that takes in a quadrant, processes it and sends it back to DIST
//
/////////////////////////////////////////////////////////////////////////////////////////

void worker(chanend fromtoDist){
    while(1){
        int quadrantSize = (IMHT/2) + 2;
        int xQuadrantSize = IMHT/8 + 2;
        uchar quadrant[IMHT/8 + 2][IMWD/2 + 2];

        for(int j = 0; j < quadrantSize; j++){ //Fill the quadrant from the distributer
            for(int i = 0; i < xQuadrantSize; i++){
                uchar tempVal;
                fromtoDist :> tempVal;
                if(tempVal==1)SetBitQUAD(quadrant,i,j);
                else ClearBitQUAD(quadrant,i,j);
            }
        }

        for(int y = 1; y < IMHT/2 + 1; y++){ //Create the 3x3 microworld to send to CellNewState
              for(int x = 1; x < xQuadrantSize + 1; x++){
                  uchar microWorld[3][3];

                  microWorld[0][0] = TestBitQUAD(quadrant,x-1,y-1);
                  microWorld[1][0] = TestBitQUAD(quadrant,x,y-1);
                  microWorld[2][0] = TestBitQUAD(quadrant,x+1,y-1);

                  microWorld[0][1] = TestBitQUAD(quadrant,x-1,y);
                  microWorld[1][1] = TestBitQUAD(quadrant,x,y);
                  microWorld[2][1] = TestBitQUAD(quadrant,x+1,y);

                  microWorld[0][2] = TestBitQUAD(quadrant,x-1,y+1);
                  microWorld[1][2] = TestBitQUAD(quadrant,x,y+1);
                  microWorld[2][2] = TestBitQUAD(quadrant,x+1,y+1);

                  fromtoDist <: CellNewState(microWorld); //Return new cell state to the distributer

              }
          }
    }
}


/////////////////////////////////////////////////////////////////////////////////////////
//
// Start your implementation by changing this function to implement the game of life
// by farming out parts of the image to worker threads who implement it...
// Currently the function just inverts the image
//
/////////////////////////////////////////////////////////////////////////////////////////
void distributor(chanend c_in, chanend c_out, chanend fromAcc, chanend fromButtons, chanend toLEDs, chanend tofromWorker[4])
{
  //Runtime variables
  int numberOfProcesses = 0;
  int numberCellsAlive = 0;
  int val = 0;
  int tilted;
  uchar tempBit = 0;
  uchar world[IMHT/8][IMWD];

  //Timers
  unsigned int timer_start;
  unsigned int timer_end;
  timer t;
  long long int totalTime = 0;

  printf("Waiting for button SW1 to be pressed \n");

  while(val != 14){ //If SW1 is pressed then continue
      fromButtons :> val;
  }

  toLEDs <: 1; //Set green LED from reading world image

  for( int y = 0; y < IMHT; y++ ) {             //go through all lines
    for( int x = 0; x < IMWD; x++ ) {           //go through each pixel per line
      c_in :> tempBit;                      //read the pixel value and input into world
      if(tempBit == 1) SetBitFULL(world,x,y);
      if(tempBit == 0) ClearBitFULL(world,x,y);
    }
  }



  printf( "\n ProcessImage: Image Read, World Created, size = %dx%d\n", IMHT, IMWD );

  //While loop for continuous processing of the world
  while(1){
      t :> timer_start;
      select{  //Select between process world, output data or pause

          case fromButtons :> val:      //Output data case
              if(val == 13){
                  toLEDs <: 2;     //Set blue LED
                  for( int y = 0; y < IMHT; y++ ) {
                      for( int x = 0; x < IMWD; x++ ) {
                          c_out <: world[x][y]; //Send each value to the dataOutStream
                      }
                  }
              }
              break;


          default: //Main Processing case
              fromAcc :> tilted;
              if(tilted == 1){
                  long long int tempTime = totalTime * (42/(pow(2,32)-1));
                  printf("Paused because board is tilted \n");
                  printf("Number of rounds Processed: %d \n", numberOfProcesses);
                  printf("Number of alive cells: %d \n", numberCellsAlive);
                  printf("Time elapsed: %d \n", tempTime);
                  toLEDs <: 8; //Set red LED as game is paused
                  waitMoment();
                  waitMoment();
                  waitMoment();
                  break;
              }


              toLEDs <: 0;  //Send blank down LED's to make it flashing
              printf("Processing the world \n");

              int quadrantSizeSend = (IMHT / 2) + 2;
              int xQuadrantSize = IMHT/8;
              int quadrantSizeReceive = IMHT/2;
              numberCellsAlive = 0;

              //SEND TO WORKERS
              for(int j = -1; j < quadrantSizeSend - 1; j++){
                  for(int i = -1; i < quadrantSizeSend - 1; i++){
                      tofromWorker[0] <: TestBitFULL(world,overlap(i),overlap(j));
                      tofromWorker[1] <: TestBitFULL(world,overlap(i+xQuadrantSize),overlap(j));
                      tofromWorker[2] <: TestBitFULL(world,overlap(i),overlap(j+quadrantSizeReceive));
                      tofromWorker[3] <: TestBitFULL(world,overlap(i+xQuadrantSize),overlap(j+quadrantSizeReceive));
                  }
              }

              toLEDs <: 4; //Set flashing green LED for processing

              //RECEIVE FROM WORKERS
              for(int j = 0; j < quadrantSizeReceive; j++){
                  for(int i = 0; i < quadrantSizeReceive; i++){

                    uchar tempValReceive;
                    tofromWorker[0] :> tempValReceive;
                    if(tempValReceive == BLACK){
                        SetBitFULL(world,i,j);
                        numberCellsAlive += 1;
                    }
                    else{
                        ClearBitFULL(world,i,j);
                    }

                    tofromWorker[1] :> tempValReceive;
                    if(tempValReceive == BLACK){
                        SetBitFULL(world,i+xQuadrantSize,j);
                        numberCellsAlive += 1;
                    }
                    else{
                        ClearBitFULL(world,i+xQuadrantSize,j);
                    }

                    tofromWorker[2] :> tempValReceive;
                    if(tempValReceive == BLACK){
                        SetBitFULL(world,i,j+quadrantSizeReceive);
                        numberCellsAlive += 1;
                    }
                    else{
                        ClearBitFULL(world,i,j+quadrantSizeReceive);
                    }

                    tofromWorker[3] :> tempValReceive;
                    if(tempValReceive == BLACK){
                        SetBitFULL(world,i+xQuadrantSize,j+quadrantSizeReceive);
                        numberCellsAlive += 1;
                    }
                    else{
                        ClearBitFULL(world,i+xQuadrantSize,j+quadrantSizeReceive);
                    }

//                    //Check number of cells alive and add to the variable
//                    if(world[i][j] == BLACK) numberCellsAlive += 1;
//                    if(world[i+quadrantSizeReceive][j] == BLACK) numberCellsAlive += 1;
//                    if(world[i][j+quadrantSizeReceive] == BLACK) numberCellsAlive += 1;
//                    if(world[i+quadrantSizeReceive][j+quadrantSizeReceive] == BLACK) numberCellsAlive += 1;
                    }
                }

              numberOfProcesses += 1; //Increment 1 each round
              break;
      }
      //Append timers after each processing round
      t :> timer_end;
      totalTime = totalTime + (timer_end - timer_start);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// Write pixel stream from channel c_in to PGM image file
//
/////////////////////////////////////////////////////////////////////////////////////////
void DataOutStream(chanend c_in)
{
  int exportCounter = 0; //Variable to switch between the two output files

  char outfname[] = "testout.pgm";                        //put your output image path here
  char outfname2[] = "testout2.pgm";

  while(1){

      int res;              //Resource variable
      uchar line[ IMWD ];   //Line variable

      if(exportCounter == 0) res = _openoutpgm( outfname, IMWD, IMHT );         //Open file 1
      else if(exportCounter == 1) res = _openoutpgm( outfname2, IMWD, IMHT );   //Open file 2

      if( res ) {
        printf( "DataOutStream: Error opening %s\n.", outfname );               //Error catching
        return;
      }

      //Compile each line of the image and write the image line-by-line
      for( int y = 0; y < IMHT; y++ ) {
        for( int x = 0; x < IMWD; x++ ) {
          c_in :> line[ x ];
        }
        _writeoutline( line, IMWD );
        //printf( "DataOutStream: Line written...\n" );
      }

      //Close the PGM image
      _closeoutpgm();
      printf( "DataOutStream: Done...\n" );
      printf("New image created \n");

      exportCounter++;
      if(exportCounter == 2) exportCounter = 0;
  }
  return;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// Initialise and  read orientation, send first tilt event to channel
//
/////////////////////////////////////////////////////////////////////////////////////////
void orientation( client interface i2c_master_if i2c, chanend toDist) {
  i2c_regop_res_t result;
  char status_data = 0;

  // Configure FXOS8700EQ
  result = i2c.write_reg(FXOS8700EQ_I2C_ADDR, FXOS8700EQ_XYZ_DATA_CFG_REG, 0x01);
  if (result != I2C_REGOP_SUCCESS) {
    printf("I2C write reg failed\n");
  }
  
  // Enable FXOS8700EQ
  result = i2c.write_reg(FXOS8700EQ_I2C_ADDR, FXOS8700EQ_CTRL_REG_1, 0x01);
  if (result != I2C_REGOP_SUCCESS) {
    printf("I2C write reg failed\n");
  }

  //Probe the orientation x-axis forever
  while (1) {

    //check until new orientation data is available
    do {
      status_data = i2c.read_reg(FXOS8700EQ_I2C_ADDR, FXOS8700EQ_DR_STATUS, result);
    } while (!status_data & 0x08);

    //get new x-axis tilt value
    int x = read_acceleration(i2c, FXOS8700EQ_OUT_X_MSB);

    //send signal to distributor after first tilt

    if (x>30) {
      toDist <: 1;
      }
    else toDist <: 0;

  }
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// Orchestrate concurrent system and start up all threads
//
/////////////////////////////////////////////////////////////////////////////////////////
int main(void) {

    i2c_master_if i2c[1];   //interface to orientation


    chan c_inIO,            //Channel between Data In and Distributer
         c_outIO,           //Channel between Distributer and Data Out
         c_control,         //Channel between orientation and distributer to check if board tilted
         c_buttons,         //Channel passing button data from buttonListener to Distributer
         c_LEDs,            //Channel passing LED data from distributer to showLEDs
         c_worker[4];       //Chanlist for each worker

    par {
        on tile[1]: worker(c_worker[0]);
        on tile[1]: worker(c_worker[1]);
        on tile[1]: worker(c_worker[2]);
        on tile[1]: worker(c_worker[3]);
        on tile[0].core[0]: i2c_master(i2c, 1, p_scl, p_sda, 10);                           //server thread providing orientation data
        on tile[0]: orientation(i2c[0],c_control);                                          //client thread reading orientation data
        on tile[0]: DataInStream(c_inIO);                                                   //thread to read in a PGM image
        on tile[0]:DataOutStream(c_outIO);                                                  //thread to write out a PGM image
        on tile[0]:distributor(c_inIO, c_outIO, c_control, c_buttons, c_LEDs, c_worker);    //thread to coordinate work on image
        on tile[0]: buttonListener(buttons,c_buttons);                                      //client thread reading button data
        on tile[0]: showLEDs(leds,c_LEDs);                                                  //thread to output the LED's
      }

  return 0;
}

//WEEK AFTER: COMMENT WHOLE THING, REPORT EFFICIENCY STUFF
