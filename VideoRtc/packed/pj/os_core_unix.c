#include "os.h"
#include <string.h>


static void *thread_main(void *param)
{
    pj_thread_t *rec = (pj_thread_t*)param;
        /* Call user's entry! */
    void *result = (void*)(long)(*rec->proc)(rec->arg);
    return result;
}

//return thread id
int pj_thread_create( const char *thread_name,
				      thread_proc *proc,
				      void *arg,
				      pj_size_t stack_size,
				      unsigned flags,
				      pj_thread_t *pthread_out)
{
    int status = 0;
    if(!pthread_out)
        return -1;

    pthread_out->arg  = arg;
    pthread_out->proc = proc;
    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    status = pthread_create( &pthread_out->threadId, &thread_attr, &thread_main, pthread_out);
    if (status != 0) {
	    return -2;
    }
    strcpy(pthread_out->obj_name, thread_name);
    return pthread_out->threadId;
}

int pj_thread_destroy(pj_thread_t *pthread) {
    int status = 0;
    void *ret;
    if(!pthread)
        return -1;
    pthread->thread_quit = 1;

    status = pthread_join( pthread->threadId, &ret);
    if (status == 0)
        return 1;

    return status;
}
