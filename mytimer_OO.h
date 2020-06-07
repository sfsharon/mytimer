/* mytimer_OO.h 
 * References : 
 * - Timer file descriptor library implementation for C example (using a background thread) :
 *   https://qnaplus.com/implement-periodic-timer-linux/
 * - Solution for using pthread start_routine as a class method was taken from :
 *   https://stackoverflow.com/questions/1151582/pthread-function-from-a-class
 * - Using std::map :
 *   https://www.geeksforgeeks.org/inserting-elements-in-stdmap-insert-emplace-and-operator/
 * - Erasing element from std::map :
 *   https://stackoverflow.com/questions/10038985/remove-a-key-from-a-c-map
*/
#include <iostream>
#include <sys/timerfd.h>
#include <poll.h>
#include <pthread.h>
#include <system_error>
#include <string.h>
#include <unistd.h>

#include <map>

using namespace std;

class TimerBase
{       
protected:
    TimerBase(unsigned int initialTime);
    
    // Perform timer system calls (timer create and set)
    int initTimer(void);
    
public:    
    int                 m_fd;    
    struct itimerspec   m_timerSpec;        
};

/* Synchronous Timer - Blocking timer - Equivalent to sleep but with millisecond resolution*/
class TimerSync : public TimerBase
{
private :
    int m_timeout;
    
public:
    TimerSync (unsigned int initialTime, int timeout);
  
  
    void startTimer(void);
  
   ~TimerSync(void) {close(m_fd);}  
};  // class TimerSync


// -------------------------------------------------------------------------
// Library Constants
#define MAX_TIMER_COUNT       1000

// Polling timeout determines the minimum frequency in which 
// the background thread wakes up, and updates the timer file descriptor array
#define POLL_TIMEOUT_MILLISEC 100

// Library types 
// -------------
// Callback function when timer expires
typedef void (*time_handler)(size_t timer_id);

class TimerASyncMng
{
public :
    TimerASyncMng();
    
    // Creates and adds a timer to the container, 
    // and returns the timer fd as an identifier
    int startTimer (unsigned int initialTime,   /* milliseconds to start timer */
                    unsigned int intervalTime,  /* milliseconds for periodic operation */
                    time_handler handler);

    int changeTimer (int fd, 
                     unsigned int initialTime,   /* milliseconds to start timer */
                     unsigned int intervalTime   /* milliseconds for periodic operation */ );
    
    void stopTimer (int fd);
    
    // Debug method
    void printMap(void);
    
    // Background method for polling the timer file descriptors and calling the callback functions
    void TimerThreadEntry(void);

    // DTOR 
    ~TimerASyncMng(void);
    
private : // Private Inner class - Not to be published to user (Unlike TimerSynch)

    /* Asynchronous Timer - Non Blocking Timer - Relies on a timer thread to carry out the callback */
    class TimerASync : public TimerBase
    {
    public:
      TimerASync (unsigned int initialTime,
                  unsigned int intervalTime,
                  time_handler callback) : TimerBase(initialTime),
                                           m_callback (callback) 
      {
        setTime(initialTime, intervalTime); 
      }

      void setTime(unsigned int initialTime,
                   unsigned int intervalTime)
      {
        m_timerSpec.it_interval.tv_sec = intervalTime / 1000;
        m_timerSpec.it_interval.tv_nsec = (intervalTime % 1000) * 1000000; 
      }
      
      
      void startTimer(void)
      {
            /* Create timer file descriptor, and initialize settime */
            int rc = initTimer();
            if (rc != 0) 
            {
                throw std::runtime_error("Init timer failed ");
            }
              
      }
        // void stopTimer(void); 
    public:
        // ASynchronous callback function, to be called in the context of the periodic thread
        time_handler m_callback;  
    }; // class TimerASync
    
    
private : // Class TimerASyncMng attributes    
    pthread_t m_thread_id;    
    
    // map contains the key as fd, and value as TimerASynch object
    map <int, TimerASync> m_map;
    
    // Using a static wrap function because pthread_create will not accept a class
    // method with a this pointer as a first parameter
    static void* TimerThreadEntryFunc(void* This) 
    {
        ((TimerASyncMng *)This)->TimerThreadEntry(); 
        return NULL;
    }    
    
    
}; // class TimerASyncMng
