// Copyright Necula Mihail 313CAa 2023-2024
#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

/******************************
 * The queue is implemented using an array of pointers.
*******************************/
typedef struct queue_t
{
	/* The maximum number of elements which can be stored in queue. */
	u_int max_size;
	/* The current number of elements from the queue. */
	u_int size;
	/* The dimension, in bytes, of an element. */
	u_int data_size;
	/* The index from where will take place the operations
	of front and dequeue. */
	u_int read_idx;
	/*  The index from where will take place the operation
	 of enqueue. */
	u_int write_idx;
	/* The buffer where will be stored the elements from the queue. */
	void **buff;
	/* Pointer at the function which free the memory of an element. */
	void (*free_elem)(void *);
}queue_t;

/******************************
 * q_create() - Create and initialize a queue.
 *
 * @param data_size: The size / number of bytes of an element.
 * @param max_size: Maximum number of elements which can be stored.
 * @param free_elemen: Pointer to the function which free the memory
 * 		allocated for an element.
 *
 * @return - The created queue.
*******************************/
queue_t *q_create(u_int data_size, u_int max_size, void (*free_elem)(void *));

/******************************
 * @return - The number of elements from the given queue.
*******************************/
u_int q_get_size(queue_t *q);

/******************************
 * q_is_empty() - Verify if a queue is vacant /  empty.
 * 
 * @param q: The queue with which we work.
 *
 * @return - 1, if the queue is empty
 *			 0, in contary case
*******************************/
u_int q_is_empty(queue_t *q);

/******************************
 * @return - The first element from the given queue, without
 * 		to eliminate it.
*******************************/
void *q_front(queue_t *q);

/******************************
 * q_dequeue() - Take out the first element from a queue.
 *
 * @param q: The queue with which we work
 * 
 * @return 1, if the operation was done succesfully
 *		   0, in contrary case
*******************************/
u_int q_dequeue(queue_t *q);

/******************************
 * q_enqueue() - Introduce a new element in a queue. 
 * 
 * @param q: The queue with which we work.
 * @param new_data: The new element.
 *
 * @return 1, if the operation was done succesfuly
 *		   0, in contrary case
*******************************/
u_int q_enqueue(queue_t *q, void *new_data);

/******************************
 * @brief Eliminate all the elements from the given queue. It's
 * 		the responsability of the function to free the memory
 * 		ehich was allocated for them.
*******************************/
void q_clear(queue_t *q);

/******************************
 * @brief Free the memory allocated for the given queue.
*******************************/
void q_free(queue_t *q);

#endif

