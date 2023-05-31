// SPDX-License-Identifier: BSD-3-Clause

#include "os_threadpool.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* === TASK === */

/* Creates a task that thread must execute */
os_task_t *task_create(void *arg, void (*f)(void *))
{
	os_task_t *task = (os_task_t *)malloc(sizeof(os_task_t));

	task->argument = arg;
	task->task = f;
	return task;
}

/* Add a new task to threadpool task queue */
void add_task_in_queue(os_threadpool_t *tp, os_task_t *t)
{
	os_task_queue_t *node = (os_task_queue_t *)malloc(sizeof(os_task_queue_t));

	node->task = t;
	node->next = NULL;
	// make adding to queue atomic
	pthread_mutex_lock(&tp->taskLock);
	// If head is null set it to node
	if (tp->tasks == NULL) {
		tp->tasks = node;
	} else {
		// Otherwise iterate to the end of the list and add node
		os_task_queue_t *current = tp->tasks;

		while (current->next != NULL)
			current = current->next;
		current->next = node;
	}
	pthread_mutex_unlock(&tp->taskLock);
}

/* Get the head of task queue from threadpool */
os_task_t *get_task(os_threadpool_t *tp)
{
	// make getting from queue atomic
	pthread_mutex_lock(&tp->taskLock);
	os_task_queue_t *node = tp->tasks;

	// If head is null return null
	if (node == NULL) {
		pthread_mutex_unlock(&tp->taskLock);
		return NULL;
	}
	// Set head to the node next to the head
	tp->tasks = node->next;
	os_task_t *task = node->task;

	pthread_mutex_unlock(&tp->taskLock);
	free(node);
	return task;
}

/* === THREAD POOL === */

/* Initialize the new threadpool */
os_threadpool_t *threadpool_create(unsigned int nTasks, unsigned int nThreads)
{
	// Allocate memory for threadpool
	os_threadpool_t *tp = (os_threadpool_t *)malloc(sizeof(os_threadpool_t));

	tp->should_stop = 0;
	tp->num_threads = nThreads;
	tp->tasks = NULL;
	// Initialize mutex
	pthread_mutex_init(&tp->taskLock, NULL);
	// Create threads
	tp->threads = (pthread_t *)malloc(sizeof(pthread_t) * nThreads);
	for (unsigned int i = 0; i < nThreads; i++) {
		int result = pthread_create(&tp->threads[i], NULL, thread_loop_function, tp);

		if (result != 0)
			printf("Failed to create thread.\n");
	}
	return tp;
}

/* Loop function for threads */
void *thread_loop_function(void *args)
{
	os_threadpool_t *tp = (os_threadpool_t *)args;

	while (tp->should_stop == 0) {
		os_task_t *task = get_task(tp);

		// Run task
		if (task != NULL) {
			task->task(task->argument);
			free(task);
		}
	}
	return NULL;
}

/* Stop the thread pool once a condition is met */
void threadpool_stop(os_threadpool_t *tp, int (*processingIsDone)(os_threadpool_t *))
{
	while (1) {
		// Check if there are no more tasks and if processing is done
		if (tp->tasks == NULL && processingIsDone(tp) == 1) {
			tp->should_stop = 1;
			// Join threads
			for (unsigned int i = 0; i < tp->num_threads; i++) {
				int result = pthread_join(tp->threads[i], NULL);

				if (result != 0)
					printf("Failed to join thread.\n");
			}
			break;
		}
	}
}
