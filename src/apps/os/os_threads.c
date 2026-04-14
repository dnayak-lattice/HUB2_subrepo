/******************************************************************************
 * Copyright (c) 2026 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "os_funcs.h"
#include "os_threads.h"

/* OS Abstracted threads operations implemented by the Application. */

/**
 * Initialize the mutex.
 *
 * @param p_mutex Pointer to the mutex to initialize.
 *
 * @return 0 on success, -1 on failure
 */
int os_mutex_init(os_mutex_t *p_mutex)
{
	int ret = pthread_mutex_init((pthread_mutex_t *)p_mutex, NULL);

	if (ret != 0) {
		os_pr_err("Failed to initialize mutex: %d\n", ret);
		return -1;
	}
	return 0;
}

/**
 * Destroy the mutex.
 *
 * @param p_mutex Pointer to the mutex to destroy.
 *
 * @return 0 on success, -1 on failure
 */
int os_mutex_destroy(os_mutex_t *p_mutex)
{
	int ret = pthread_mutex_destroy((pthread_mutex_t *)p_mutex);

	if (ret != 0) {
		os_pr_err("Failed to destroy mutex: %d\n", ret);
		return -1;
	}
	return 0;
}

/**
 * Lock a mutex.
 *
 * @param p_mutex Pointer to the mutex to lock.
 *
 * @return 0 on success, -1 on failure
 */
int os_mutex_lock(os_mutex_t *p_mutex)
{
	int ret = pthread_mutex_lock((pthread_mutex_t *)p_mutex);

	if (ret != 0) {
		os_pr_err("Failed to lock mutex: %d\n", ret);
		return -1;
	}
	return 0;
}

/**
 * Unlock a mutex.
 *
 * @param p_mutex Pointer to the mutex to unlock.
 *
 * @return 0 on success, -1 on failure
 */
int os_mutex_unlock(os_mutex_t *p_mutex)
{
	int ret = pthread_mutex_unlock((pthread_mutex_t *)p_mutex);

	if (ret != 0) {
		os_pr_err("Failed to unlock mutex: %d\n", ret);
		return -1;
	}
	return 0;
}

/**
 * Create a thread.
 *
 * @param p_thread Pointer to storage for the new thread ID.
 * @param p_attr Pointer to thread attributes.
 * @param p_start_routine Entry function for the new thread.
 * @param p_arg Argument passed to p_start_routine.
 *
 * @return 0 on success, -1 on failure
 */
int os_thread_create(os_thread_t      *p_thread,
					 os_thread_attr_t *p_attr,
					 void             *(*p_start_routine)(void *),
					 void             *p_arg)
{
	int ret = pthread_create(p_thread, p_attr, p_start_routine, p_arg);

	if (ret != 0) {
		os_pr_err("Failed to create thread: %d\n", ret);
		return -1;
	}
	return 0;
}

/**
 * Join a thread.
 *
 * @param thread The thread to join.
 * @param p_exit_status Where to store the thread's return value.
 *
 * @return 0 on success, -1 on failure
 */
int os_thread_join(os_thread_t thread, void **p_exit_status)
{
	int ret = pthread_join(thread, (void **)p_exit_status);

	if (ret != 0) {
		os_pr_err("Failed to join thread: %d, exit status: %p\n", ret,
				  p_exit_status != NULL ? *p_exit_status : NULL);
		return -1;
	}
	return 0;
}

/**
 * Request cancellation of a thread.
 *
 * @param thread The thread to cancel.
 *
 * @return 0 on success, -1 on failure
 */
int os_thread_cancel(os_thread_t thread)
{
	int ret = pthread_cancel(thread);

	if (ret != 0 && ret != ESRCH) {
		os_pr_err("Failed to cancel thread: %d\n", ret);
		return -1;
	}
	return 0;
}
