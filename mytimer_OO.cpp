#include "mytimer_OO.h"

TimerBase::TimerBase(unsigned int initialTime)
{   
    /* Translate time from initialTime milliseconds 
       to struct itimerspec/struct timespec */
    m_timerSpec.it_value.tv_sec = initialTime / 1000;
    m_timerSpec.it_value.tv_nsec = (initialTime % 1000) * 1000000;
}

int TimerBase::initTimer(void) 
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



TimerSync::TimerSync (unsigned int initialTime, int timeout) :  TimerBase(initialTime),
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



void TimerSync::startTimer(void)
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


TimerASyncMng::TimerASyncMng()
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


int TimerASyncMng::startTimer (unsigned int initialTime,   /* milliseconds to start timer */
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


int TimerASyncMng::changeTimer ( int fd, 
                                 unsigned int initialTime,   /* milliseconds to start timer */
                                 unsigned int intervalTime   /* milliseconds for periodic operation */ )   
{
  int rVal = 0; // Init to success
  
  map<int, TimerASync>::iterator iter = m_map.find(fd) ;
  if( iter != m_map.end() )
  {
    iter->second.setTime(initialTime, intervalTime);
    
    // Update file descriptor time                        
    timerfd_settime(fd,
                    0,
                    &(iter->second.m_timerSpec),
                    NULL);
  }
  else 
  {        
    rVal = -1;
  }
  
  return rVal;
}


void TimerASyncMng::stopTimer (int fd)
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

void TimerASyncMng::printMap(void)
{
    cout << "Printing fd map :" << endl;
    map<int, TimerASync>::iterator it;

    for ( it = m_map.begin(); it != m_map.end(); it++ )
    {
        std::cout << "*** File descriptor : " << it->first << endl ;
        std::cout << "it_value.tv_sec     " << it->second.m_timerSpec.it_value.tv_sec << endl;
        std::cout << "it_value.tv_nsec    " << it->second.m_timerSpec.it_value.tv_nsec << endl;
        std::cout << "it_interval.tv_sec  " << it->second.m_timerSpec.it_interval.tv_sec << endl;
        std::cout << "it_interval.tv_nsec " << it->second.m_timerSpec.it_interval.tv_nsec << endl;

    }
}


void TimerASyncMng::TimerThreadEntry(void)
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
TimerASyncMng::~TimerASyncMng(void)
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