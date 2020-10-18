#ifndef __PJ_OS_H__
#define __PJ_OS_H__

/**
 * @file os.h
 * @brief OS dependent functions
 */
#include "types.h"
#include <sys/time.h>

#include <pthread.h>



#define PJ_MAX_OBJ_NAME 32

#define THREAD_FUNC
typedef int (THREAD_FUNC thread_proc)(void*);


struct pj_thread_t
{
    char	    obj_name[PJ_MAX_OBJ_NAME];
    pthread_t	threadId;
    thread_proc *proc;
    void	    *arg;
    int         thread_quit;
};

int pj_thread_create( const char *thread_name, thread_proc *proc, void *arg, pj_size_t stack_size, unsigned flags, pj_thread_t *pthread_out);

int pj_thread_destroy(pj_thread_t *pthread);

#endif