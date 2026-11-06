/* macOS compatibility shim for unnamed POSIX semaphores.
   macOS declares sem_init() but does not implement it (it returns -1/ENOSYS),
   so programs that use sem_init/sem_wait/sem_post would silently run with NO
   synchronisation. This header is force-included ONLY when building on macOS:
       gcc ... -include macos_sem.h bb.c
   so the assignment's source files stay exactly as given. On Linux it is not
   used. It implements a counting semaphore with a mutex + condition variable
   (i.e. a small monitor).                                                    */
#ifndef MACOS_SEM_H
#define MACOS_SEM_H
#ifdef __APPLE__
#include <pthread.h>
#include <semaphore.h>

typedef struct { pthread_mutex_t m; pthread_cond_t c; unsigned v; } compat_sem_t;

static inline int compat_sem_init(compat_sem_t *s, int pshared, unsigned v)
{ (void)pshared; pthread_mutex_init(&s->m, NULL); pthread_cond_init(&s->c, NULL); s->v = v; return 0; }
static inline int compat_sem_wait(compat_sem_t *s)
{ pthread_mutex_lock(&s->m); while (s->v == 0) pthread_cond_wait(&s->c, &s->m); s->v--; pthread_mutex_unlock(&s->m); return 0; }
static inline int compat_sem_post(compat_sem_t *s)
{ pthread_mutex_lock(&s->m); s->v++; pthread_cond_signal(&s->c); pthread_mutex_unlock(&s->m); return 0; }
static inline int compat_sem_destroy(compat_sem_t *s)
{ pthread_mutex_destroy(&s->m); pthread_cond_destroy(&s->c); return 0; }

#define sem_t       compat_sem_t
#define sem_init    compat_sem_init
#define sem_wait    compat_sem_wait
#define sem_post    compat_sem_post
#define sem_destroy compat_sem_destroy
#endif
#endif
