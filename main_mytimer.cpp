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

// Timer callback
void time_handler_cb(size_t timer_id)
{
    cout << "time_handler called with fd " << timer_id << endl;
}

void testSync(void)
{
    // TEST : Synchronous timer (Blocking)
    // -----------------------------------
    
    // Init test
    // *********
    cout << "\n--- TEST : Synchronous timer (Blocking)" << endl;
    unsigned int initTime = 2000; // Millisecond time 
    int          timeout  = 3000; // Negative value means infinite loop
    
    TimerSync mytimer(initTime, /* Initial time */
                      timeout   /* timeout      */ );    
    
    // Run test
    // *********    
    TimeVar t1=timeNow();    
        mytimer.startTimer();
    TimeVar t2=timeNow();
    
    // Calculate test PASS/FAIL criteria
    // *********************************
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

void testASync(void)
{
    // TEST : ASynchronous timer (Non-Blocking)
    // ----------------------------------------   
    cout << "\n--- TEST : ASynchronous timer (Non-Blocking)" << endl;  
    int timerSingle, timerPeriodic;
    
    TimerASyncMng mytimerMng;
   
    // TIMER_SINGLE_SHOT
    timerSingle = mytimerMng.startTimer(3000, 0,    time_handler_cb); 
    
    // TIMER_PERIODIC
    cout << "--- Fast periodic timer " << endl;        
    timerPeriodic = mytimerMng.startTimer(10, 500, time_handler_cb);        
    sleep(3);

    cout << "--- Slowing down periodic frequency " << endl;    
    mytimerMng.changeTimer(timerPeriodic, 10, 1000);    
    sleep(3);

    cout << "--- Stopping periodic frequency " << endl;    
    mytimerMng.changeTimer(timerPeriodic, 0, 0);    
    sleep(6);
    
    mytimerMng.stopTimer(timerSingle);
    mytimerMng.stopTimer(timerPeriodic);     
}

int main()
{
    testSync();    // Test Blocking timer    
    testASync();   // Test Non-Blocking timers
    
    return 0;
}    