/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef __APP_OS_THREADS_H__
#define __APP_OS_THREADS_H__

#include <pthread.h>

/* OS Threads operations. */

/* Opaque handle to a mutex. */
typedef pthread_mutex_t os_mutex_t;

/* Opaque handle to a thread. */
typedef pthread_t os_thread_t;

/* Opaque handle to a thread attributes. */
typedef pthread_attr_t os_thread_attr_t;

/**
 * Initialize the mutex.
 *
 * @param p_mutex Pointer to the mutex to initialize.
 *
 * @return 0 on success, -1 on failure
 */
int os_mutex_init(os_mutex_t *p_mutex);

/**
 * Destroy the mutex.
 *
 * @param p_mutex Pointer to the mutex to destroy.
 *
 * @return 0 on success, -1 on failure
 */
int os_mutex_destroy(os_mutex_t *p_mutex);

/**
 * Lock the mutex.
 *
 * @param p_mutex Pointer to the mutex to lock.
 *
 * @return 0 on success, -1 on failure
 */
int os_mutex_lock(os_mutex_t *p_mutex);

/**
 * Unlock the mutex.
 *
 * @param p_mutex Pointer to the mutex to unlock.
 *
 * @return 0 on success, -1 on failure
 */
int os_mutex_unlock(os_mutex_t *p_mutex);

/**
 * Create a thread.
 *
 * @param p_thread Pointer to storage for the new thread ID.
 * @param p_attr Pointer to thread attributes (may be NULL per pthread_create).
 * @param p_start_routine Entry function for the new thread.
 * @param p_arg Argument passed to p_start_routine.
 *
 * @return 0 on success, -1 on failure
 */
int os_thread_create(os_thread_t      *p_thread,
					 os_thread_attr_t *p_attr,
					 void             *(*p_start_routine)(void *),
					 void             *p_arg);

/**
 * Join a thread.
 *
 * @param thread The thread to join (by value).
 * @param p_exit_status Where to store the thread's return value; may be NULL.
 *
 * @return 0 on success, -1 on failure
 */
int os_thread_join(os_thread_t thread, void **p_exit_status);

/**
 * Request cancellation of a thread (POSIX pthread_cancel).
 *
 * Used to unblock threads waiting on I/O during shutdown. The caller must
 * still pthread_join to reap thread resources.
 *
 * @param thread The thread to cancel (by value).
 *
 * @return 0 on success, -1 on failure
 */
int os_thread_cancel(os_thread_t thread);

#endif /* __APP_OS_THREADS_H__ */
