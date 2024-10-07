// Copyright Necula Mihail 313CAa 2023-2024
#include "queue.h"

queue_t * q_create(u_int data_size, u_int max_size, void (*free_elem)(void *))
{
	// Create the queque.
	queue_t *q = (queue_t *)malloc(sizeof(queue_t));
	DIE(q == NULL, "malloc() failed\n");

	// Initialize the fields of queue.
	q->max_size = max_size;
	q->size = 0;
	q->data_size = data_size;
	q->read_idx = 0;
	q->write_idx = 0;
	q->buff = malloc(max_size * sizeof(void *));
	DIE(q->buff == NULL, "malloc() failed\n");
	q->free_elem = free_elem;

	// Return the queque.
	return q;
}

u_int q_get_size(queue_t *q)
{
	if (!q)
		return 0;
	return q->size;
}

u_int q_is_empty(queue_t *q)
{
	if (!q || !q->size)
		return 1;
	return 0;
}

void *q_front(queue_t *q)
{
	// Verify the parameter of the function.
	if (!q || !q->buff || !q->size) {
		return NULL;
	}

	// Return the first element from queue.
	return q->buff[q->read_idx];
}

u_int q_dequeue(queue_t *q)
{
	// Verify the parameter of the function.
	if (!q || !q->buff || !q->size)
		return 0;

	// Take out the element and free its memory.
	if (q->free_elem) {
		void *elem = q_front(q);
		q->free_elem(elem);
	}

	// Update the queue's parameters.
	q->read_idx = (q->read_idx + 1) % q->max_size;
	q->size--;


	// The operation was done successfully.
	return 1;
}

u_int q_enqueue(queue_t *q, void *new_data)
{
	// Verify the parameter of the function.
	if (!q || q->size == q->max_size)
		return 0;

	// Make a coppy of the given data and put it in queue.
	memcpy(&q->buff[q->write_idx], new_data, q->data_size);

	// Update queue's parameters.
	q->write_idx = (q->write_idx + 1) % q->max_size;
	q->size++;

	// The operation was done succesfully.
	return 1;
}

void q_clear(queue_t *q)
{
	// Take out every element and free its memory.
	while (!q_is_empty(q))
		q_dequeue(q);

	// Update queue's parameters.
	q->size  = 0;
	q->read_idx = 0;
	q->write_idx = 0;
}

void q_free(queue_t *q)
{
	// Verify the parameter.
	if (!q || !q->buff) {
		fprintf(stderr, "q_free() - queue isn/t valid\n");
		return;
	}

	// Free the memory of every elemnt.
	q_clear(q);

	// Free the memory of the array.
	free(q->buff);

	// Free the queue structure's memory.
	free(q);
}
