/* mytimer.c */

#include <stdint.h>
#include <string.h>
#include <sys/timerfd.h>
#include <pthread.h>
#include <poll.h>
#include <stdio.h>

#include "mytimer.h"

#define MAX_TIMER_COUNT 1000

struct timer_node
{
    int                 fd;
    timer_handler       callback;
    void*               user_data;
    unsigned int        interval;
    t_timer             type;
    struct timer_node*  next;
   
}

// Module internal members
static void*                _timer_thread(void* data);
static pthread_t            g_thread_id;
static struct timer_node*   g_head = NULL;

int intialize()
{
// int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
//                    void *(*start_routine) (void *), void *arg);
    
    if (pthread_create( &g_thread_id, 
                        NULL, 
                        _timer_thread(void* data),
                        NULL))
    {
        printf("Thread creation failed \n");
        return 0;
    }
    
    return 1;
}

size_t start_timer (unsigned int interval, /* milli seconds */
                    time_handler handler, 
                    t_timer      type, 
                    void*        user_data)
{
    struct timer_node*  new_node = NULL;
        
    // struct timespec {
       // time_t tv_sec;                /* Seconds */
       // long   tv_nsec;               /* Nanoseconds */
    // };

    // struct itimerspec {
       // struct timespec it_interval;  /* Interval for periodic timer */
       // struct timespec it_value;     /* Initial expiration */
    // };    
    struct itimerspec   new_value;
    
    new_node = (struct timer_node*) malloc (sizeof(struct timer_node));
    
    if (new_node == NULL) 
    {
        printf("Error allocating new timer node\n");
        return 0;
    }
    
    new_node->callback  = handler;
    new_node->user_data = user_data;
    new_node->interval  = interval;
    new_node->type      = type;

    new_node->fd = timerfd_create(CLOCK_REALTIME, 0);
    
    if (new_node->fd == -1)
    {
        printf("Error in  timerfd_create\n");
        free(new_node);
        return 0;
    }
    
    new_value.it_value.tv_sec = interval / 1000;
    new_value.it_value.tv_nsec = (interval % 1000) * 1000000;
    
    if (type == TIMER_PERIODIC)
    {
        new_value.it_interval.tv_sec = interval / 1000;
        new_value.it_interval.tv_nsec = (interval % 1000) * 1000000;        
    }
    else
    {
        new_value.it_interval.tv_sec = 0;
        new_value.it_interval.tv_nsec = 0;                
    }
    
    // int timerfd_settime(int fd, int flags,
    //                      const struct itimerspec *new_value,
    //                      struct itimerspec *old_value);
    
    timerfd_settime(new_node->fd,
                    0,
                    &new_value,
                    NULL);
    
    /* Insert the timer node into the head of the linked list */
    new_node->next = g_head;
    g_head = new_node;
    
    return (size_t)new_node;   
}
                        
void stop_timer(size_t timer_id)
{
    struct timer_node*  tmp  = g_head;
    struct timer_node* node = (struct timer_node *)timer_id;

    if (node == NULL) 
    {
        printf ("Received NULL pointer for deletion\n");
        return;
    }
    
    if (node == g_head)
    {
        g_head = g_head->next;
    } else 
    {   // Search for timer_node node to remove it from the linked list
        while ((tmp != NULL) && (tmp->next != node)) 
        {
            tmp = tmp->next;
        }
        
        /* tmp->next cannot be NULL here , because there is more then one node
           in list. One node and empty list were handled in the previous two if's */
        tmp->next = tmp->next->next;
    }

    // Close file descriptor and free node memory
    close(node->fd);
    free(node);
}



void finalize()
{
    while (g_head != NULL) 
    {
        stop_timer((size_t)g_head);
    }
    
    pthread_cancel(g_thread_id);
    pthread_join(g_thread_id, NULL);
}

/* Translate a file descriptor to a timer node object
   by iterating on the linked list
*/
struct timer_node* _get_timer_from_fd(int fd)
{
    struct timer_node* tmp = g_head;
    
    while (tmp != NULL)
    {
        if (tmp->fd == fd)
        {
            return tmp;
        }
        
        tmp = tmp->next;
    }
    
    return NULL;
}

void* _timer_thread(void* data)
{
// struct pollfd {
//      int   fd;         /* file descriptor */
//      short events;     /* requested events */
//      short revents;    /* returned events */
// };    
    struct pollfd ufds[MAX_TIMER_COUNT] = {{0}};
    int iMaxCount = 0;
    struct timer_node* tmp = NULL;
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
        tmp = g_head;
        
        memset(ufds, sizeof(struct pollfd)*MAX_TIMER_COUNT);
        // Walk over the linked list, and initialize ufds 
        // with timers data
        while(tmp != NULL)
        {
            ufds[iMaxCount].fd = tmp->fd;
            ufds[iMaxCount].events = POLLIN;
            iMaxCount++;
            
            tmp = tmp->next;
        } // while (tmp!=NULL)

        // int poll(struct pollfd *fds, nfds_t nfds, int timeout);            
        // From man poll : 
        // poll waits for one of a set of file descriptors to become ready to perform I/O.
        // The timeout argument specifies the number of milliseconds that poll() should block 
        // waiting for a file descriptor to become ready.  The call will block until either:
        // *  a file descriptor becomes ready;
        // *  the call is interrupted by a signal handler; or
        // *  the timeout expires.        
        read_fds = poll(ufds, iMaxCount, 100);    
        
        if (read_fds <= 0)
        {
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
                tmp = _get_timer_from_fd(ufds[i].fd);
                if ((tmp != NULL) && (tmp->callback != NULL))
                {
                    tmp->callback((size_t)tmp, tmp->user_data);
                }
                
            } // if revents has bit POLLIN up
        } // for iterating the fds array, ufds        
    } // while(1)
    
    // This line looks redundant - unreachable line
    return NULL;
}

