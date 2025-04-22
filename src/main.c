#include "alphaESS.h"

int main(){
    stdio_init_all();

    while(true){
        alphaESS_run();
        sleep_ms(10000);
    }
}