// Copyright Necula Mihail 313CAa 2023-2024
#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include "server.h"
#include "lru_cache.h"
#include "hash_map.h"
#include "queue.h"
#include "utils.h"
#include "constants.h"

#define TASK_QUEUE_SIZE         1000
#define MAX_LOG_LENGTH          1000
#define MAX_RESPONSE_LENGTH     4096

/******************************
 * Structure to save the informtions
 * of a server.
*******************************/
typedef struct server_t {
	/* The cache. */
	struct lru_cache_t *cache;
	/* Pointer to the local data base in which we save
	pairs of next type: doc's name - doc's contnet. */
	struct hashtable_t **local_db;
	/* The queue of requests.*/
	struct queue_t *task_queue;
	/* The id of the server. */
	u_int id;
	/* The hash of the server's id.*/
	u_int hash_id;
} server_t;

/******************************
 * Structure to save the informtions
 * of a request.
*******************************/
typedef struct request_t {
	/* The request's type. */
	request_type type;
	/* The name of the file about
	which we are interesetd. */
	char *doc_name;
	/* The content of the file if we want
	to edit a document. */
	char *doc_content;
} request_t;

/******************************
 * Structure to save the informations
 * of the repsponse of a request.
*******************************/
typedef struct response_t {
	char *server_log;
	char *server_response;
	int server_id;
} response_t;


hashtable_t *ht_redistribute(hashtable_t *old_ht, int new_hmax);

/******************************
 * free_request() - Free the memory of a request.
 *
 * @param r: Request's address.
******************************/
void free_request(void *r);

/******************************
 * key_doc_free_function() - Free the memory of a pair from hashtable,
 * 		in which the key is a simple type of data and the value is of
 *		type doc_t*.
 *
 * @param data: The pair from hashtable.
******************************/
void key_doc_free_function(void *data);

/******************************
 * @brief Duplicate the given request and return that coppy.
*******************************/
request_t *duplicate_request(request_t *req);

/******************************
 * create_response() - Allocate memory for a structure of type
 * 		response and its fields.
 *
 * @return - The created response.
*******************************/
response_t *create_response();

/******************************
* db_resdistribute_docs() - Change the number of buckets from the hashtbale
*		of a database. Also, the documents are redistributed in hashmap.
*
* @param old_db: The database which we must modify.
* @param new_hmax: The new number of buckets. 
*
* @return: The updated hashtable.
*******************************/
hashtable_t *db_redistribute_docs(hashtable_t *old_db, int new_hmax);

/******************************
* @brief Increase the number of buckets from the hashtable of a database
*		and return the updated hashmap. The new number of buckets it's
* 		the closest primer number of the product old number of buckets * 10.
*******************************/
hashtable_t *db_increase_hmax(hashtable_t *db);

/******************************
 * db_add_doc() - Add a document in the local database of a
 *		server.
 *
 * @param s: Server with wich we work.
 * @param file: Document which will be added.
 *		(The doc is put how is received - It's
 *		not put a coppy of it.)
*******************************/
void db_add_doc(server_t *s, doc_t *file);

/******************************
 * db_remove_doc() - Remove a document from the local database of
 *		a server.
 *
 * @param s: Server with wich we work.
 * @param doc_name: Name of the document
 *		which will be removed.
*******************************/
void db_remove_doc(server_t *s, char *doc_name);

/******************************
 * init_doc() - Create and initialize a doc.
 *
 * @param doc_name: The name of the documnet.
 * @param doc_content: The content of the document.
 *
 * @return - The created doc.
*******************************/
doc_t *init_doc(char *doc_name, char *doc_content);

/******************************
 * init_server() - Create and initialize a server.
 *
 * @param server_id: The id of the server.
 *
 * @param cache_size: The maximum number of docs which
 * 		can be saved in cache.
 *
 * @return - The created server.
*******************************/
server_t *init_server(unsigned int server_id, unsigned int cache_size);

/******************************
 * free_resource_of_server() - Deallocate completely the memory used by
 *		a server, excluding the memory of the server's structure.
*******************************/
void free_resources_of_server(server_t *srv);

/******************************
 * free_server() - Deallocate completely the memory used by a server.
 *
 * @param s: Pointer to the server's address with which we work.
*******************************/
void free_server(server_t **s);

/******************************
 * server_edit_document() - Do a edit operation.
 *
 * @param s: Server with wich we work.
 * @param doc_name: The name of the document
 *		which will be edited.
 * @param doc_content: The new content of the doc.
 *
 * @return response_t*: Response of the edit operation.
*******************************/
response_t
*server_edit_document(server_t *s, char *doc_name, char *doc_content);

/******************************
 * server_get_document() - Do a get operation.
 *
 * @param s: Server with wich we work.
 * @param doc_name: The name of the document
 *		whose content we want.
 *
 * @return response_t*: Response of the get operation.
*******************************/
response_t *server_get_document(server_t *s, char *doc_name);

/******************************
 * @brief Resolve all (edit) requests from task queue of
 *		the given server.
*******************************/
void do_tasks_from_queue(server_t *s);

/******************************
 * server_handle_request() - Receives a request from the load balancer
 *      and processes it according to the request type.
 * 
 * @param s: Server which processes the request.
 * @param req: Request to be processed.
 * 
 * @return response_t*: Response of the requested operation.
 * 
 * @brief Based on the type of request, call the appropriate solvers,
 *     and execute the tasks from queue if needed (in this case, after
 *     executing each task, PRINT_RESPONSE is called).
*******************************/
response_t *server_handle_request(server_t *s, request_t *req);

#endif
