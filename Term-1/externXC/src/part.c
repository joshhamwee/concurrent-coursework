#include <platform.h>
#include <stdio.h>

extern void delay(uint delay);

void hello(int tileNo){
    delay((3-tileNo)*1000);
    printf("hello from tile #%d. \n", tileNo);
}
