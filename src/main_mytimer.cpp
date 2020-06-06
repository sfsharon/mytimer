/* main_mytimer.cpp */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include "mytimer_OO.h"

using namespace std;

// Testing timers
#include <chrono>
#include <utility>
#include <math.h>
typedef std::chrono::high_resolution_clock::time_point TimeVar;
#define duration(a) std::chrono::duration_cast<std::chrono::nanoseconds>(a).count()
#define timeNow() std::chrono::high_resolution_clock::now()

// Test constants
static const double PASS_ERROR_MARGIN = 10; // 10ms error margin for passing the unit test

void testTimer(void)
{
    // Synchronous timer with 1 second blocking time
    unsigned int initTime = 1000; // Millisecond time 
    int          timeout  = 4000; // Negative value means infinite loop
    
    TimerSync mytimer(initTime, /* Initial time */
                      timeout   /* timeout      */ );    
    
    TimeVar t1=timeNow();    
        mytimer.startTimer();
    TimeVar t2=timeNow();
    double delta = (duration(t2 - t1))/10e5;
    double margin = fabs(delta - double(initTime));
    cout << "Duration of timer : " << delta  << endl;
    if (margin > PASS_ERROR_MARGIN) 
    {
        cout << "Test failed : Timer margin " << margin << " exceeds limit of " << PASS_ERROR_MARGIN << endl;
    }
    else
    {
        cout << "Test passed !: Timer margin " << margin << " below limit of " << PASS_ERROR_MARGIN << endl;
    }
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
    