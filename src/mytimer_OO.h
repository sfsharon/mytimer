/* mytimer_OO.h */
#include <iostream>
#include <sys/timerfd.h>
#include <system_error>

using namespace std;

class TimerBase
{
protected:
    TimerBase(unsigned int initialTime) : m_initialTime(initialTime)
    {   
        cout << "TimerBase CTOR" << endl;
    }
    
    virtual void startTimer(void); // shared capability between Sync and ASync
    // virtual void stopTimer(void); // This is an Async timer capability only
private:
    int                m_fd;
    unsigned int       m_initialTime;   // Initial time of timer. For sync timer, the only timer value
    // time_handler       callback;
    // void*               user_data;
    // unsigned int        interval;
    // t_timer             type;
    // struct timer_node*  next;
};

class TimerSync : public TimerBase
{
public:
  TimerSync (unsigned int initialTime) : TimerBase(initialTime)
  {
    m_fd = timerfd_create(CLOCK_REALTIME, 0);
    if (m_fd == -1)
    {
        throw system_Error (errno,
                            system_category(),
                            "Failed to timerfd_create");
    }
  }

};


class TimerASync : public TimerBase
{
public:
  TimerSync (unsigned int waitTime)
  {
    m_fd = timerfd_create(CLOCK_REALTIME, 0);
    if (m_fd == -1)
    {
        throw system_Error (errno,
                            system_category(),
                            "Failed to timerfd_create");
    }
  }

private:
    unsigned int m_intervalTime;
};