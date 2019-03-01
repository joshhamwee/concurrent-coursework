/*
 * externXC.xc
 *
 *  Created on: 5 Oct 2018
 *      Author: joshhamwee
 */

#include <platform.h>
#include <stdio.h>

extern void hello(int tileNo);

int main(void){
    par{
        on tile[1] : hello(1);
        on tile[0] : hello(0);
    }
    return 0;
}


void delay(uint delay){
    uint time, tmp;
    timer t;

    t :> time;

    t when timerafter ( time + delay ) :> tmp;
}


