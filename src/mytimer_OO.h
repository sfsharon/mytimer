/* mytimer_OO.h */
#include <iostream>
#include <sys/timerfd.h>
#include <poll.h>
#include <system_error>

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
    
  void stopTimer(void)
  {
      close(m_fd);
  }
  
  ~TimerBase(void)
  {
    cout << "TimerBase DTOR" << endl;
    stopTimer();
  }
    
protected:
    int                 m_fd;
    struct itimerspec   m_timerSpec;    
    
    // time_handler       callback;
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


    
};

// -------------------------------------------------------------------------

/* Asynchronous Timer - Non Blocking Timer - Relies on a timer thread to carry out the callback */
class TimerASync : public TimerBase
{
public:
  TimerASync (unsigned int initialTime,
              unsigned int intervalTime) : TimerBase(initialTime)
  {
    cout << "TimerASync CTOR" << endl;      
    m_timerSpec.it_interval.tv_sec = intervalTime / 1000;
    m_timerSpec.it_interval.tv_nsec = (intervalTime % 1000) * 1000000;  
  }

    // void startTimer(void)
    // {
        
    // }
    // void stopTimer(void); 
};
