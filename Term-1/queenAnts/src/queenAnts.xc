/*
 * queenAnts.xc
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

void queen(struct Ant Ant, const unsigned char boardArray[3][4], chanend antData1, chanend antData2){
    int fertility1,fertility2;
    antData1 :> fertility1;
    antData2 :> fertility2;
    if(fertility1 > fertility2){
        antData1 <: 1;
        antData2 <: 0;
    }else if(fertility1 < fertility2){
        antData1 <: 0;
        antData2 <: 1;
    }else{
        antData1 <: 1;
        antData2 <: 1;
    }

}

void ant(struct Ant Ant, const unsigned char boardArray[3][4], chanend data){
    int highest;
    int fertility = boardArray[Ant.y][Ant.x] - '0';
    data <: fertility;
    data :> highest;

    if(highest == 0){
        Ant.x++;
        if(Ant.x==4){
            Ant.x = 0;
        }
        Ant.food += boardArray[Ant.y][Ant.x];

        printf("Ant[%d] moved east to [%d][%d] \n",Ant.id,Ant.x,Ant.y);
        printf("Ant[%d] has food value %d \n",Ant.id,Ant.food);
        Ant.y++;
           if(Ant.y==3){
               Ant.y = 0;
           }
        Ant.food += boardArray[Ant.y][Ant.x];

        printf("Ant[%d] moved south to [%d][%d] \n",Ant.id,Ant.x,Ant.y);
        printf("Ant[%d] has food value %d \n",Ant.id,Ant.food);
    }

}

int main(){
    chan a1,a2;
    const unsigned char boardArray[3][4] = {{10,0,1,7},{2,10,0,3},{6,8,7,6}};
    struct Ant Ant1;
    struct Ant Ant2;
    struct Ant Queen;

    Ant1.id = 1;
    Ant1.food = 0;
    Ant1.x = 0;
    Ant1.y = 0;


    Ant2.id = 2;
    Ant2.food = 0;
    Ant2.x = 0;
    Ant2.y = 1;


    Queen.id = 3;
    Queen.food = 0;
    Queen.x = 1;
    Queen.y = 1;



    par{
        queen(Queen, boardArray,a1,a2);
        ant(Ant1, boardArray,a1);
        ant(Ant2, boardArray,a2);
    }

    return 0;
}
