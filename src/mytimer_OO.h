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

#include <map>

using namespace std;

class TimerBase
{       
protected:
    TimerBase(unsigned int initialTime)
    {   
        // cout << "TimerBase CTOR" << endl;

        /* Translate time from initialTime milliseconds 
           to struct itimerspec/struct timespec */
        m_timerSpec.it_value.tv_sec = initialTime / 1000;
        m_timerSpec.it_value.tv_nsec = (initialTime % 1000) * 1000000;
    }
    
    int initTimer(void) 
    {
        int rVal = 0; // Init return value to success (value 0)
        
        // Create timer file descriptor 
        m_fd = timerfd_create(CLOCK_REALTIME, /* int clockid */
                              0);             /* int flags   */
        if (m_fd == -1)
        {            
            cout << "timerfd_create : Failure" << endl;
            rVal = -1;
        }
        else
        {
            // cout << "initTimer : m_timerSpec.it_value.tv_sec " << m_timerSpec.it_value.tv_sec << endl;
            // cout << "initTimer : m_timerSpec.it_value.tv_nsec " << m_timerSpec.it_value.tv_nsec << endl;
            // cout << "initTimer : m_timerSpec.it_interval.tv_sec " << m_timerSpec.it_interval.tv_sec << endl;
            // cout << "initTimer : m_timerSpec.it_interval.tv_nsec " << m_timerSpec.it_interval.tv_nsec << endl;            
            // Arm timer and activate it                        
            int rc = timerfd_settime(m_fd,
                                     0,
                                     &m_timerSpec,
                                     NULL);
            if (rc != 0) 
            {
                cout <<"timerfd_settime : Failure with return code : " << rc << endl;
                rVal = -2 ;
            }                                       
        }

        return rVal;                        
    }
    
    virtual void startTimer(void) = 0;
    
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
  TimerSync (unsigned int initialTime, int timeout) :  TimerBase(initialTime),
                                                       m_timeout(timeout)
  {
    // cout << "TimerSync CTOR - Start" << endl;      
    // Synchronous timers are blocking, so cannot use periodic timers with interval
    m_timerSpec.it_interval.tv_sec  = 0;
    m_timerSpec.it_interval.tv_nsec = 0;  

    if (timeout < 0)
    {
        cout << "TimerSync CTOR - Negative timeout means Infinite timeout" << endl;      
    }
    else if (timeout < initialTime)  
    // Make sure that timeout is larger then the initial time if it is not negative
    {
        throw std::runtime_error("timeout should be larger then initial time");
    }
  }
  
  
  void startTimer(void)
  {
    /* Create timer file descriptor, and initialize settime */
    int rc = initTimer();
    if (rc != 0) 
    {
        throw std::runtime_error("Init timer failed ");
    }
      
    int read_fd = 0;
    
    /* Use a single pollfd array for the synchronous timer*/
    struct pollfd ufds[1] = {{ .fd = m_fd,
                               .events = POLLIN,
                               .revents = 0}};    

    // cout << "TimerSync startTimer " << endl; 

    /* Block on the file descriptor */
    read_fd = poll(ufds,                 /* struct   pollfd *fds */
                   1,                    /* nfds_t   nfds */
                   m_timeout);           /* int timeout : Specifying a negative value in  timeout means  an infinite timeout.*/
                            

    /* Error checking for poll system call */
    if (read_fd <= 0)
    {
        // Either an error has occurred (read_fs < 0)
        // or timed out, without any event on the file descriptors (read_fd == 0)
        cout << "TimerSync : Error in read_fd : " << read_fd << endl;
    }
    
    /* Check if event POLLIN was raised */
    if (ufds[0].revents & POLLIN)
    {
        // cout << "TimerSync : Returned with POLLIN " << endl;
    }
    else
    {
        cout << "TimerSync : Returned without POLLIN " << endl;
    }
  } 
  
    ~TimerSync(void)
    {
        close(m_fd);
    }  
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
    TimerASyncMng()
    {
        int rc = pthread_create(&m_thread_id,           /* pthread_t *thread                */
                                NULL,                   /* const pthread_attr_t *attr       */
                                TimerThreadEntryFunc,   /* void *(*start_routine) (void *)  */
                                this);                  /* void *arg                        */
        if (rc != 0)
        {
            throw std::runtime_error("pthread_create : Thread creation failed");
        }        
    }
    
    // Creates and adds a timer to the container, 
    // and returns the timer fd as an identifier
    int startTimer (unsigned int initialTime,   /* milliseconds to start timer */
                    unsigned int intervalTime,  /* milliseconds for periodic operation */
                    time_handler handler)
    {
        // Create new ASynchTimer object on stack, and then push it onto the container
        TimerASync newTimer(initialTime, 
                            intervalTime, 
                            handler);
                            
        // Start timer 
        newTimer.startTimer();    

        // Declaring pair for return value of map containing 
        // map iterator and bool 
        pair <map<int, TimerASync>::iterator, bool> ptr;           
        ptr = m_map.insert( pair <int, TimerASync>(newTimer.m_fd, newTimer) ); 
          
        // checking if the key was already present or newly inserted 
        if(ptr.second) 
        {
            // cout << "The key " << newTimer.m_fd << " was newly inserted" << endl; 
        }
        else 
        {
            cout << "The key " << newTimer.m_fd << " was already present" << endl; 
            throw std::runtime_error("File descriptor already exists in container");
        }

        return (newTimer.m_fd);
    }

    void stopTimer (int fd)
    {
      // Remove element from map
      map<int, TimerASync>::iterator iter = m_map.find(fd) ;
      if( iter != m_map.end() )
      {
        m_map.erase( iter );
      }
      else 
      {
        cout << "Cannot find fd " << fd << " in container" << endl; 
        throw std::runtime_error("Cannot find fd in container for erasure");
      }
      
      // Close file descriptor 
      close(fd);
    }
    
    // Debug method
    void printMap(void)
    {
        cout << "Printing fd map :" << endl;
        map<int, TimerASync>::iterator it;

        for ( it = m_map.begin(); it != m_map.end(); it++ )
        {
            std::cout << "*** File descriptor : " << it->first << endl ;
        }
    }
        
    void TimerThreadEntry(void)
    {        
        struct pollfd ufds[MAX_TIMER_COUNT] = {{0}};
        int iMaxCount = 0;
        map<int, TimerASync>::iterator it;
        
        int read_fds = 0, i, s;
        
        // The value to be read from the fd after a POLLIN event
        // has occurred
        uint64_t exp;
        
        while(1)
        {
            pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
            pthread_testcancel();
            pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
            
            iMaxCount = 0;
            // tmp = g_head;
            
            // Initialize pollfd array
            memset(ufds, 0, sizeof(struct pollfd)*MAX_TIMER_COUNT);
            
            // Walk over the map, and initialize ufds with timers file descriptors
            for ( it = m_map.begin(); it != m_map.end(); it++ )
            {
                ufds[iMaxCount].fd = it->first;
                ufds[iMaxCount].events = POLLIN;
                
                // cout << "THREAD : Adding [" << iMaxCount << "]" << ufds[iMaxCount].fd << endl;
                
                iMaxCount++;
            }                       
                                    
            read_fds = poll(ufds, iMaxCount, POLL_TIMEOUT_MILLISEC);    
            
            if (read_fds <= 0)
            {
                // cout << "read_fds : " << read_fds << endl;
                // Either an error has occurred (read_fs < 0)
                // or timed out, without any event on the file descriptors (read_fds == 0)
                // Then loop back to the beginning of the while loop
                continue;
            }
            
            // Loop over the fds with returned events
            for (i = 0; i < iMaxCount; i++)
            {
                if (ufds[i].revents & POLLIN)
                {
                    s = read(ufds[i].fd, &exp, sizeof(uint64_t));
                    
                    // Make sure all the 64 bits were read from the fd
                    if (s != sizeof(uint64_t)) continue;
                    
                    // Call the callback function if applicable
                    // tmp = _get_timer_from_fd(ufds[i].fd);
                    it = m_map.find(ufds[i].fd);
                    
                    if ((it != m_map.end()) && (it->second.m_callback != NULL))
                    {
                        it->second.m_callback(ufds[i].fd);
                    }                    
                } // if revents has bit POLLIN up
            } // for iterating the fds array, ufds        
        } // while(1)        
    } // TimerThreadEntry(void)

    // DTOR 
    ~TimerASyncMng(void)
    {
        // Clearing all elements in map in destructor
        map<int, TimerASync>::iterator it;
        for ( it = m_map.begin(); it != m_map.end(); it++ )
        {
            stopTimer(it->first);
        }
        
        pthread_cancel(m_thread_id);
        pthread_join(m_thread_id, NULL);    
    }
    
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
