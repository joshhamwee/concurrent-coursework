// hellotile.xc
#include <platform.h>
#include <stdio.h>

void hello(int tileNo);

// main starting two tasks in parallel on different tiles
int main(void) {
    par {
        on tile[0] : hello(0); //start on tile 0
        on tile[1] : hello(1); //start on tile 1
    }
    return 0;
}

// function to print message
void hello(int tileNo){
    printf("Hello from tile #%d.\n", tileNo);
}

