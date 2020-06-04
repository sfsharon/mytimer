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

static void*                _timer_thread(void* data);
static pthread_t            g_thread_id;
static struct timer_node*   g_head = NULL;

int intialize()
{
// int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
// void *(*start_routine) (void *), void *arg);
    
    if (pthread_create( &g_thread_id, 
                        NULL, 
                        _timer_thread(void* data),
                        NULL))
    {
        printf("Thread creation faile \n");
        return 0;
    }
    
    return 1;
}

size_t start_timer (unsigned int interval, /* milli seconds */
                    time_handler handler, 
                    t_timer      type, 
                    void* user_data)
{
    struct timer_node*  new_node = NULL;
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
    
    /* Inserting the timer node into the linked list */
    new_node->next = g_head;
    g_head = new_node;
    
    return (size_t)new_node;   
}
                        
void stop_timer(size_t timer_id)
{
    struct time_node* tmp = NULL;
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
    {
        tmp = g_head;
        
        while ((tmp !=NULL) && (tmp->next != node)) 
        {
            tmp = tmp->next;
        }
        
        if (tmp != NULL)
        {
            /* tmp->next cannot be NULL here */
            tmp->next = tmp->next->next;
            close(node->fd);
            free(node);
        }        
    }
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
    struct pollfd ufds[MAX TIMER_COUNT] =
}

