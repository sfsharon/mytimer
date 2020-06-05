/* mytimer.h */
#ifndef MYTIMER_H
#define MYTIMER_H
#include <stdlib.h>

/* Declare library types */
typedef enum
{
    TIMER_SINGLE_SHOT = 0, /* Single Shot Timer */
    TIMER_PERIODIC         /* Periodic Timer */  
} t_timer;

typedef void (*time_handler)(size_t timer_id, void* user_data);

/* Declare library functions */
int initialize();
size_t start_timer(unsigned int interval, time_handler handler, t_timer type, void* user_data);
void stop_timer(size_t timer_id);
void finalize();

#endif
