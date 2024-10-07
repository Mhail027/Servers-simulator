// Copyright Necula Mihail 313CAa 2023-2024
#include "server.h"

void free_request(void *r)
{
	// Make the needed cast.
	request_t *req = (request_t *)r;

	// Free the memory.
	free(req->doc_name);
	free(req->doc_content);
	free(req);
}

void key_doc_free_function(void *data)
{
	// Make the needed cast.
	info_t *pair = (info_t *)data;

	// Free the key's memory
	free(pair->key);

	// Free the value's memory.
	doc_t *file = *(doc_t **)pair->value;
	free(file->name);
	free(file->content);
	free(file);
	free(pair->value);
}

request_t *duplicate_request(request_t *req)
{
	// Create a request
	request_t *req_dup = (request_t *)malloc(sizeof(request_t));
	DIE(req_dup == NULL, "malloc() failed\n");

	// Coppy the type.
	req_dup->type = req->type;

	// Coppy the doc's name.
	req_dup->doc_name = (char *)malloc(strlen(req->doc_name) + 1);
	strcpy(req_dup->doc_name, req->doc_name);

	// Coppy the doc's contents.
	req_dup->doc_content = (char *)malloc(strlen(req->doc_content) + 1);
	strcpy(req_dup->doc_content, req->doc_content);

	// Return the duplicate.
	return req_dup;
}

response_t *create_response()
{
	// Alocate memory for reponse's structure.
	response_t *rsp = (response_t *)malloc(sizeof(response_t));
	DIE(rsp == NULL, "malloc() failed\n");

	// Allocate memory for the server's log.
	rsp->server_log = (char *)malloc(MAX_LOG_LENGTH * sizeof(char));
	DIE(rsp->server_log== NULL, "malloc() failed\n");

	// Allocate memory for the server's reponse.
	rsp->server_response = (char *)malloc(MAX_RESPONSE_LENGTH * sizeof(char));
	DIE(rsp->server_response == NULL, "malloc() failed\n");

	// Return the created response.
	return rsp;
}

doc_t *init_doc(char *doc_name, char *doc_content)
{
	// Allocate memory for doc's structure.
	doc_t *file = (doc_t *)malloc(sizeof(doc_t));
	DIE(file == NULL, "malloc() failed\n");

	// Allocate memory for doc's name and init that field.
	file->name = (char *)malloc((strlen(doc_name) + 1) * sizeof(char));
	DIE(file->name == NULL, "malloc() failed\n");
	strcpy(file->name, doc_name);

	// Allocate memory dor doc's content and init that field.
	file->content = (char *)malloc((strlen(doc_content) + 1) * sizeof(char));
	DIE(file->content == NULL, "malloc() failed\n");
	strcpy(file->content, doc_content);

	// Return the created file.
	return file;
}

hashtable_t *db_redistribute_docs(hashtable_t *old_db, int new_hmax)
{
	// Create the new databse.
	hashtable_t *new_db = ht_create(new_hmax, old_db->hash_function,
						  old_db->compare_function,
						  old_db->key_val_free_function);

	// Add every doc from the old db in the new db.
	for (u_int i = 0; i < old_db->hmax; ++i) {
		// Get a bucket from the old ht.
		ll_t *curr_list = old_db->buckets[i];

		// Go through bucket.
		ll_node_t *curr_node = curr_list->head;
		while (curr_node) {
			info_t *pair = (info_t *)curr_node->data;
			char *doc_name = (char *)pair->key;
			ht_put(new_db, doc_name, strlen(doc_name) + 1, pair->value,
				   sizeof(doc_t *));
			curr_node = curr_node->next;

			free(pair->key);
			free(pair->value);
		}

		// Deallocate the memory of the bucket.
		ll_free(&curr_list);
	}

	// Free the unncesary memory.
	free(old_db->buckets);
	free(old_db);

	// Return the new created ht / database.
	return new_db;
}

hashtable_t *db_increase_hmax(hashtable_t *db)
{
	u_int prime_number[9] = {17, 173, 1733, 17383, 173839, 1738391,
							17383913, 173839157, 1738391579};

	for (int i = 0; i < 8; ++i)
		if(prime_number[i] == db->hmax)
			return db_redistribute_docs(db, prime_number[i + 1]);

	return db;
}

void db_add_doc(server_t *s, doc_t *file)
{
	// Verify if we need to add more buckets in the database's hashtable.
	if ((*s->local_db)->size / 10 == (*s->local_db)->hmax)
		*s->local_db = db_increase_hmax(*s->local_db);

	// Add the document.
	char *name = file->name;
	ht_put(*s->local_db, name, strlen(name) + 1, &file, sizeof(doc_t *));
}

void db_remove_doc(server_t *s, char *doc_name)
{
	ht_remove_entry(*s->local_db, doc_name);
}

server_t *init_server(u_int server_id, u_int cache_size)
{
	// Allocate memory for the server's structure.
	server_t *srv = (server_t *)malloc(sizeof(server_t));
	DIE(!srv, "malloc() failed\n");

	// Allocate memory for every complex field from structure.
	srv->cache = init_lru_cache(cache_size);
	srv->local_db = (hashtable_t **)malloc(sizeof(hashtable_t *));
	DIE(srv->local_db == NULL, "malloc() failed\n");
	*srv->local_db = ht_create(17, hash_string, compare_function_strings,
					key_doc_free_function);
	srv->task_queue = q_create(sizeof(request_t *), TASK_QUEUE_SIZE, free_request);

	// Initialize the parameters of the server.
	srv->id = server_id;
	srv->hash_id = 0;

	// Return the created server.
	return srv;
}

void free_resources_of_server(server_t *srv)
{
	// Free the memory of the cache.
	free_lru_cache(&srv->cache);
	// Free the memory of the local data base.
	ht_free(srv->local_db);
	free(srv->local_db);
	// Free the memory of the requests's queue.
	q_free(srv->task_queue);
}

void free_server(server_t **s)
{
	// Get the server's address.
	server_t *srv = *s;

	// Free the memory of srver's resources.
	free_resources_of_server(srv);

	// Free the memory of the server's structure.
	free(srv);

	// Lose the address of the server, which
	// doesn't exist anymore.
	*s = NULL;
}

response_t *server_edit_document(server_t *s, char *doc_name, char *doc_content)
{
	// Do the response.
	response_t *rsp = create_response();
	if (lru_cache_has_key(s->cache, doc_name)) {
		sprintf(rsp->server_log, LOG_HIT, doc_name);
		sprintf(rsp->server_response, MSG_B, doc_name);
	} else {
		// Will update the log later if the cache is full
		// and the curent doc isn't in cache.
		sprintf(rsp->server_log, LOG_MISS, doc_name);
		if (!ht_has_key(*s->local_db, doc_name)) {
			sprintf(rsp->server_response, MSG_C, doc_name);
		} else {
			sprintf(rsp->server_response, MSG_B, doc_name);
		}
	}
	rsp->server_id = s->id;


	// Create the file which will put in the data base and in the cache.
	doc_t *file = init_doc(doc_name, doc_content);

	// Put the file in the server's data base.
	db_add_doc(s, file);

	// Put the file in the cache.
	char *evicted_doc_name = NULL;
	lru_cache_put(s->cache, doc_name, &file, (void **)&evicted_doc_name);

	// Actualize the log if it's the case.
	if (evicted_doc_name)
		sprintf(rsp->server_log, LOG_EVICT, doc_name, evicted_doc_name);

	// Free the unncecesary memory.
	free(evicted_doc_name);

	// Return the response.
	return rsp;
}

response_t *server_get_document(server_t *s, char *doc_name)
{
	// Do the response.
	response_t *rsp = create_response();
	rsp->server_id = s->id;

	// Create the log message and verify if the document is in the server.
	if (lru_cache_has_key(s->cache, doc_name)) {
		sprintf(rsp->server_log, LOG_HIT, doc_name);
	} else {
		if (!ht_has_key(*s->local_db, doc_name)) {
			// If the document doesn't exist, will create also
			// the response message and will exit from the function.
			sprintf(rsp->server_log, LOG_FAULT, doc_name);
			free(rsp->server_response);
			rsp->server_response = NULL;
			return rsp;
		} else {
			// Will update the log later if the cache is full
			// and the curent doc isn't in cache.
			sprintf(rsp->server_log, LOG_MISS, doc_name);
		}
	}

	// Find the pointer to the wanted doc.
	doc_t *file = *(doc_t **)ht_get(*s->local_db, doc_name);

	// Create the response message.
	strcpy(rsp->server_response, file->content);

	// Put the file in the cache.
	char *evicted_doc_name;
	lru_cache_put(s->cache, doc_name, &file, (void **)&evicted_doc_name);

	// Actualize the log if it's the case.
	if (evicted_doc_name)
		sprintf(rsp->server_log, LOG_EVICT, doc_name, evicted_doc_name);

	// Free the unncecesary memory.
	free(evicted_doc_name);

	// Return the response.
	return rsp;
}

// Resolve all tasks / requests from queue.
void do_tasks_from_queue(server_t *s)
{
	while (!q_is_empty(s->task_queue)) {
		// Take the first request from q.
		request_t *req = (request_t *)q_front(s->task_queue);
		// Do the request.
		response_t *rsp = server_edit_document(s, req->doc_name,
						req->doc_content);
		// Print the response of the request.
		PRINT_RESPONSE(rsp);
		// Eliminate the request from q.
		q_dequeue(s->task_queue);
	}
}

response_t *server_handle_request(server_t *s, request_t *req)
{
	// EDIT request
	if (req->type == 0) {
		// Duplicate the current request because to put it in the q
		// to make it later.
		request_t *req_dup = duplicate_request(req);

		// Put the duplicate request in q.
		q_enqueue(s->task_queue, &req_dup);

		// Make the response.
		response_t *rsp = create_response();
		sprintf(rsp->server_log, LOG_LAZY_EXEC, s->task_queue->size);
		sprintf(rsp->server_response, MSG_A, "EDIT", req->doc_name);
		rsp->server_id = s->id;

		// Return the response.
		return rsp;
	}

	// GET request
	if (req->type == 1) {
		// Resolve all (edit) requests from the task queue.
		do_tasks_from_queue(s);
		// Do the get request and return its response.
		return server_get_document(s, req->doc_name);
	}

	return NULL;
}

