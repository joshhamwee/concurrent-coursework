/*
 * parallelAnts.xc
 *
 *  Created on: 5 Oct 2018
 *      Author: joshhamwee
 */

#include <platform.h>
#include <stdio.h>



struct Ant{
    int id;
    int food;
    int x;
    int y;
};

void ant(struct Ant Ant, const unsigned char boardArray[3][4]){
    Ant.x++;
    if(Ant.x==3){
        Ant.x = 0;
    }
    Ant.food += boardArray[Ant.x][Ant.y];

    printf("Ant[%d] moved east to [%d][%d] \n",Ant.id,Ant.x,Ant.y);
    printf("Ant[%d] has food value %d \n",Ant.id,Ant.food);
    Ant.y++;
       if(Ant.y==2){
           Ant.y = 0;
       }
    Ant.food += boardArray[Ant.x][Ant.y];

    printf("Ant[%d] moved south to [%d][%d] \n",Ant.id,Ant.x,Ant.y);
    printf("Ant[%d] has food value %d \n",Ant.id,Ant.food);

}

int main(){
    const unsigned char boardArray[3][4] = {{10,0,1,7},{2,10,0,3},{6,8,7,6}};
    struct Ant Ant1;
    struct Ant Ant2;
    struct Ant Ant3;
    struct Ant Ant4;

    Ant1.id = 1;
    Ant1.food = 0;
    Ant1.x = 1;
    Ant1.y = 0;


    Ant2.id = 2;
    Ant2.food = 0;
    Ant2.x = 2;
    Ant2.y = 1;


    Ant3.id = 3;
    Ant3.food = 0;
    Ant3.x = 2;
    Ant3.y = 0;


    Ant4.id = 4;
    Ant4.food = 0;
    Ant4.x = 0;
    Ant4.y = 1;

    par{
        ant(Ant1, boardArray);
        ant(Ant2, boardArray);
        ant(Ant3, boardArray);
        ant(Ant4, boardArray);
    }

    return 0;
}




