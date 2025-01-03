#include "gameboy.h"

static gameboy GB = {0};

gameboy* get_gb(){
    return &GB;
}
void gameboy_init() {
    GB.paused = false;
    GB.ticks = 0;
    GB.die = false;

    // START TIMER
    timer_init();
    printf("Timer initialized\n");

    // START CPU - AWAIT OPCODES
    cpu_init();
    printf("CPU initialized\n");

    // start UI
    ui_init();
    printf("UI initialized\n");
}

void run_gb() {
    while(!GB.die){
        Sleep(1000); // usleep(1000) for linux
        ui_event_handler();
        ui_update();
    }
}

