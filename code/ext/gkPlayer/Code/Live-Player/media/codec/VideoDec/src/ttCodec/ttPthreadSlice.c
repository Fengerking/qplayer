#include "config.h"

#if HAVE_PTHREADS
#include <pthread.h>
#elif HAVE_W32THREADS
#include "compat/w32pthreads.h"
#elif HAVE_OS2THREADS
#include "compat/os2threads.h"
#endif

#include "ttAvcodec.h"
#include "ttInternal.h"
#include "ttPthreadInternal.h"
#include "ttThread.h"

#include "ttInternal.h"
#include "ttCommon.h"
#include "ttCpu.h"
#include "ttMem.h"

typedef int (action_func)(TTCodecContext *c, void *arg);
typedef int (action_func2)(TTCodecContext *c, void *arg, int jobnr, int threadnr);

typedef struct SliceThreadContext {
    action_func *func;
    action_func2 *func2;
    pthread_t *workers;
	void *args;
    int *rets;
    int rets_count;
    int job_count;
    int job_size;

    pthread_cond_t last_job_cond;
    pthread_cond_t current_job_cond;
    pthread_mutex_t current_job_lock;
    unsigned current_execute;
    int current_job;
    int done;

    int *entries;
    int entries_count;
    int thread_count;
    pthread_cond_t *progress_cond;
    pthread_mutex_t *progress_mutex;
} SliceThreadContext;

static void* attribute_align_arg worker(void *v)
{
    TTCodecContext *avctx = v;
    SliceThreadContext *c = avctx->internal->thread_ctx;
    unsigned last_execute = 0;
    int our_job = c->job_count;
    int thread_count = avctx->thread_count;
    int self_id;

    pthread_mutex_lock(&c->current_job_lock);
    self_id = c->current_job++;
    for (;;){
        while (our_job >= c->job_count) {
            if (c->current_job == thread_count + c->job_count)
                pthread_cond_signal(&c->last_job_cond);

            while (last_execute == c->current_execute && !c->done)
                pthread_cond_wait(&c->current_job_cond, &c->current_job_lock);
            last_execute = c->current_execute;
            our_job = self_id;

            if (c->done) {
                pthread_mutex_unlock(&c->current_job_lock);
                return NULL;
            }
        }
        pthread_mutex_unlock(&c->current_job_lock);

        c->rets[our_job%c->rets_count] = c->func ? c->func(avctx, (char*)c->args + our_job*c->job_size):
                                                   c->func2(avctx, c->args, our_job, self_id);

        pthread_mutex_lock(&c->current_job_lock);
        our_job = c->current_job++;
    }
}

void tt_slice_thread_free(TTCodecContext *avctx)
{
    SliceThreadContext *c = avctx->internal->thread_ctx;
    int i;

    pthread_mutex_lock(&c->current_job_lock);
    c->done = 1;
    pthread_cond_broadcast(&c->current_job_cond);
    pthread_mutex_unlock(&c->current_job_lock);

    for (i=0; i<avctx->thread_count; i++)
         pthread_join(c->workers[i], NULL);

    pthread_mutex_destroy(&c->current_job_lock);
    pthread_cond_destroy(&c->current_job_cond);
    pthread_cond_destroy(&c->last_job_cond);
    ttv_freep(&c->workers);
    ttv_freep(&avctx->internal->thread_ctx);
}

static ttv_always_inline void thread_park_workers(SliceThreadContext *c, int thread_count)
{
    while (c->current_job != thread_count + c->job_count)
        pthread_cond_wait(&c->last_job_cond, &c->current_job_lock);
    pthread_mutex_unlock(&c->current_job_lock);
}

static int thread_execute(TTCodecContext *avctx, action_func* func, void *arg, int *ret, int job_count, int job_size)
{
    SliceThreadContext *c = avctx->internal->thread_ctx;
    int dummy_ret;

    if (!(avctx->active_thread_type&TT_THREAD_SLICE) || avctx->thread_count <= 1)
        return ttcodec_default_execute(avctx, func, arg, ret, job_count, job_size);

    if (job_count <= 0)
        return 0;

    pthread_mutex_lock(&c->current_job_lock);

    c->current_job = avctx->thread_count;
    c->job_count = job_count;
    c->job_size = job_size;
    c->args = arg;
    c->func = func;
    if (ret) {
        c->rets = ret;
        c->rets_count = job_count;
    } else {
        c->rets = &dummy_ret;
        c->rets_count = 1;
    }
    c->current_execute++;
    pthread_cond_broadcast(&c->current_job_cond);

    thread_park_workers(c, avctx->thread_count);

    return 0;
}

static int thread_execute2(TTCodecContext *avctx, action_func2* func2, void *arg, int *ret, int job_count)
{
    SliceThreadContext *c = avctx->internal->thread_ctx;
    c->func2 = func2;
    return thread_execute(avctx, NULL, arg, ret, job_count, 0);
}

int tt_slice_thread_init(TTCodecContext *avctx)
{
    int i;
    SliceThreadContext *c;
    int thread_count = avctx->thread_count;

#if HAVE_W32THREADS
    w32thread_init();
#endif

    if (!thread_count) {
        int nb_cpus = ttv_cpu_count();
        if  (avctx->height)
            nb_cpus = FFMIN(nb_cpus, (avctx->height+15)/16);
        // use number of cores + 1 as thread count if there is more than one
        if (nb_cpus > 1)
            thread_count = avctx->thread_count = FFMIN(nb_cpus + 1, MAX_AUTO_THREADS);
        else
            thread_count = avctx->thread_count = 1;
    }

    if (thread_count <= 1) {
        avctx->active_thread_type = 0;
        return 0;
    }

    c = ttv_mallocz(sizeof(SliceThreadContext));
    if (!c)
        return -1;

    c->workers = ttv_mallocz_array(thread_count, sizeof(pthread_t));
    if (!c->workers) {
        ttv_free(c);
        return -1;
    }

    avctx->internal->thread_ctx = c;
    c->current_job = 0;
    c->job_count = 0;
    c->job_size = 0;
    c->done = 0;
    pthread_cond_init(&c->current_job_cond, NULL);
    pthread_cond_init(&c->last_job_cond, NULL);
    pthread_mutex_init(&c->current_job_lock, NULL);
    pthread_mutex_lock(&c->current_job_lock);
    for (i=0; i<thread_count; i++) {
        if(pthread_create(&c->workers[i], NULL, worker, avctx)) {
           avctx->thread_count = i;
           pthread_mutex_unlock(&c->current_job_lock);
           tt_thread_free(avctx);
           return -1;
        }
    }

    thread_park_workers(c, thread_count);

    avctx->execute = thread_execute;
    avctx->execute2 = thread_execute2;
    return 0;
}

void tt_thread_report_progress2(TTCodecContext *avctx, int field, int thread, int n)
{
    SliceThreadContext *p = avctx->internal->thread_ctx;
    int *entries = p->entries;

    pthread_mutex_lock(&p->progress_mutex[thread]);
    entries[field] +=n;
    pthread_cond_signal(&p->progress_cond[thread]);
    pthread_mutex_unlock(&p->progress_mutex[thread]);
}

void tt_thread_await_progress2(TTCodecContext *avctx, int field, int thread, int shift)
{
    SliceThreadContext *p  = avctx->internal->thread_ctx;
    int *entries      = p->entries;

    if (!entries || !field) return;

    thread = thread ? thread - 1 : p->thread_count - 1;

    pthread_mutex_lock(&p->progress_mutex[thread]);
    while ((entries[field - 1] - entries[field]) < shift){
        pthread_cond_wait(&p->progress_cond[thread], &p->progress_mutex[thread]);
    }
    pthread_mutex_unlock(&p->progress_mutex[thread]);
}

int tt_alloc_entries(TTCodecContext *avctx, int count)
{
    int i;

    if (avctx->active_thread_type & TT_THREAD_SLICE)  {
        SliceThreadContext *p = avctx->internal->thread_ctx;
        p->thread_count  = avctx->thread_count;
        p->entries       = ttv_mallocz_array(count, sizeof(int));

        p->progress_mutex = ttv_malloc_array(p->thread_count, sizeof(pthread_mutex_t));
        p->progress_cond  = ttv_malloc_array(p->thread_count, sizeof(pthread_cond_t));

        if (!p->entries || !p->progress_mutex || !p->progress_cond) {
            ttv_freep(&p->entries);
            ttv_freep(&p->progress_mutex);
            ttv_freep(&p->progress_cond);
            return AVERROR(ENOMEM);
        }
        p->entries_count  = count;

        for (i = 0; i < p->thread_count; i++) {
            pthread_mutex_init(&p->progress_mutex[i], NULL);
            pthread_cond_init(&p->progress_cond[i], NULL);
        }
    }

    return 0;
}

void tt_reset_entries(TTCodecContext *avctx)
{
    SliceThreadContext *p = avctx->internal->thread_ctx;
    memset(p->entries, 0, p->entries_count * sizeof(int));
}
