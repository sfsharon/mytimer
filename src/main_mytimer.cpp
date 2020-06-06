/* main_mytimer.cpp */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include "mytimer_OO.h"

using namespace std;

void testTimer(void)
{
    cout << "Creating testTimer" << endl;
    TimerSync mytimer(4000);    // Synchronous timer with 4 seconds blocking time
    
    cout << "Starting testTimer" << endl;
    mytimer.startTimer();
    
    cout << "Exiting testTimer" << endl;
}

int main()
{
    cout << "Starting main" << endl;
    testTimer();
    
    return 0;
}


// #include "mytimer.h"

// void time_handler1(size_t timer_id, void* user_data)
// {
    // printf ("Single shot timer expired (%zu)\n", timer_id);
// }

// void time_handler2(size_t timer_id, void* user_data)
// {
    // printf ("1000ms periodic timer expired (%zu)\n", timer_id);
// }

// void time_handler3(size_t timer_id, void* user_data)
// {
    // printf ("2000ms periodic timer expired (%zu)\n", timer_id);
// }

// int main()
// {
    // size_t timer1, timer2, timer3;
    
    // initialize();
    
    // timer1 = start_timer(3000, time_handler1, TIMER_SINGLE_SHOT, NULL);
    // timer2 = start_timer(1000, time_handler2, TIMER_PERIODIC, NULL);
    // timer3 = start_timer(2000, time_handler3, TIMER_PERIODIC, NULL);    
    
    // sleep(6);
    
    // stop_timer(timer1);
    // stop_timer(timer2);
    // stop_timer(timer3);
    
    // finalize();
// }
    