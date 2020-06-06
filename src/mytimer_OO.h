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
        cout << "TimerBase CTOR" << endl;

        /* Translate time from initialTime milliseconds 
           to struct itimerspec/struct timespec */
        m_timerSpec.it_value.tv_sec = initialTime / 1000;
        m_timerSpec.it_value.tv_nsec = (initialTime % 1000) * 1000000;
    }
    
    void initTimer(void) 
    {
        // Create timer file descriptor 
        m_fd = timerfd_create(CLOCK_REALTIME, /* int clockid */
                              0);             /* int flags   */
        if (m_fd == -1)
        {
            throw system_error (errno,
                                system_category(),
                                "Failed to timerfd_create");
        }

        // Arm timer and activate it
        timerfd_settime(m_fd,           /* int fd */
                        0,              /* int flags */
                        &m_timerSpec,   /* const struct itimerspec *new_value */
                        NULL);          /* struct itimerspec *old_value */
    }
    
    virtual void startTimer(void) = 0;
    virtual void stopTimer(void) = 0; 
    
protected:
    int                 m_fd;
    struct itimerspec   m_timerSpec;    
    
    // time_handler       callback;
    // void*               user_data;
    // unsigned int        interval;
    // t_timer             type;
    // struct timer_node*  next;
};




/* Synchronous Timer - Blocking timer - Equivalent to sleep but with millisecond resolution*/
class TimerSync : public TimerBase
{
public:
  TimerSync (unsigned int initialTime) :  TimerBase(initialTime)
  {
      cout << "TimerSync CTOR" << endl;      
      // Synchronous timers are blocking, so cannot use periodic timers with interval
        m_timerSpec.it_interval.tv_sec = 0;
        m_timerSpec.it_interval.tv_nsec = 0;  
  }
  
  ~TimerSync(void)
  {
    cout << "TimerSync DTOR" << endl;
    stopTimer();
  }
  
  void startTimer(void)
  {
    int read_fd = 0;
    
    /* Use a single pollfd array for the synchronous timer*/
    struct pollfd ufds[1] = {{ .fd = m_fd,
                               .events = POLLIN,
                               .revents = 0}};    
    
    /* Create timer fd, and initialize settime */
    initTimer();

    cout << "TimerSync startTimer " << endl; 
    #define INFINITE_TIMEOUT (-1)
    /* Block on the file descriptor */
    read_fd = poll(ufds,                 /* struct   pollfd *fds */
                   1,                    /* nfds_t   nfds */
                   INFINITE_TIMEOUT);    /* int timeout : Specifying a negative value in  timeout means  an infinite timeout.*/
                   // 6000);    /* int timeout : Specifying a negative value in  timeout means  an infinite timeout.*/                   
                            

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
        cout << "TimerSync : Returned with POLLIN " << endl;
    }
    else
    {
        cout << "TimerSync : Returned without POLLIN " << endl;
    }
  }

  void stopTimer(void)
  {
      close(m_fd);
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
