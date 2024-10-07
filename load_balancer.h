// Copyright Necula Mihail 313CAa 2023-2024
#ifndef LOAD_BALANCER_H
#define LOAD_BALANCER_H

#include "server.h"
#include "hash_map.h"

#define MAX_SERVERS 99999

/******************************
 * Structure to save the informations of a load balancer.
*******************************/
typedef struct load_balancer_t {
	/* Array of servers. */
	server_t **server;
	/* Number of servers. */
	u_int size;
	/* Maximum number of servers which can be stored
	in load balancer. */
	u_int max_size;
	/* Nummber of raplicas for a server. */
	int replicas;
	/* Pointer to a function which hash the id of a server.*/
	unsigned int (*hash_function_servers)(void *);
	/* Pointer to a function which hash the name of a doc.*/
	unsigned int (*hash_function_docs)(void *);
} load_balancer_t;

/******************************
 * init_load_balancer() - Create and initialize a load balancer.
 *
 * @param enalbe_vnode: true  -> every server has 3 replicas
 *                      false -> every server has 1 replica
 *
 * @return - The created load balancer.
*******************************/
load_balancer_t *init_load_balancer(bool enable_vnodes);

/******************************
 * @brief Double the number of servers which can be stored in
 *      the given load balancer.
*******************************/
void load_balancer_double_servers(load_balancer_t *main);

/******************************
 *	combine_databases() - Add the docs from the source server
 *		in the destination server. The files are added just in
 *		the databse, not and in the cache. The files from
 *		the source server are not removed.
 *
 * @param dst: Destination server.
 * @param src: Source server.
*******************************/
void combine_databases(server_t *dst, server_t *src);

/******************************
* lb_add_server_in_array() - Add a new server in the array of
*       a load balancer, without to redistribute the docs.
*
* @param main: The load balancer with which we work.
* @param new_s: The new server.
*
* @return - The position of the new server in array.
*******************************/
u_int lb_add_server_in_array(load_balancer_t *main, server_t *new_s);

/******************************
* loader_add_replica() - Create and add a new replica of a server in a 
*       load balancer and redistribute the docs from the load balancer. 
*       (This function considers that the given server has a unique
*       replica. The servers which are already in the system can have 
*       more replicas.)
*
* @param main: Load balancer with which we work.
* @param server_id: ID of the new replica / server.
* @param cache_size: Maximum number of documents which
*       can be stored in the cache of the new replica.
*
* @return - The new replica.
*******************************/
server_t *loader_add_replica(load_balancer_t* main, u_int server_id,
						   u_int cache_size);

/******************************
 * create_replica_of_server() - Create a replica for a server.
 *
 * @param s: The mom server.
 * @param id: ID of the replica.
 * @param hash_id: The hash of the replica's ID.
 *
 * @return - The created replica.
*******************************/
server_t *create_replica_of_server(server_t *s, u_int id, u_int hash_id);

/******************************
 * loader_add_server() - Adds a new server to the system.
 * 
 * @param main: Load balancer which distributes the work.
 * @param server_id: ID of the new server.
 * @param cache_size: Capacity of the new server's cache.
*******************************/
void
loader_add_server(load_balancer_t *main, u_int server_id, u_int cache_size);

/******************************
* loader_remove_replica() - Remove a replica of a server from a load
*       balancer and redistribute the docs from the load balancer. 
*       (This function considers that the given server has a unique
*       replica. The others servers which are in the system can have 
*       more replicas.)
*
* @param main: Load balancer with which we work.
* @param server_id: ID of the replica / server
*******************************/
void loader_remove_replica(load_balancer_t *main, u_int server_id);

/******************************
 * loader_remove_server() Removes a server from the system.
 * 
 * @param main: Load balancer which distributes the work.
 * @param server_id: ID of the server to be removed.
 * 
 * @brief Removes the server (and its replicas) from the hash ring
 * and distribute ALL documents stored on the removed server
 * to the "neighboring" servers.
 * 
 * Additionally, all the tasks stored in the removed server's queue
 * areexecuted before moving the documents.
*******************************/
void loader_remove_server(load_balancer_t *main, u_int server_id);

/******************************
 * loader_forward_request() - Forwards a request to the appropriate server.
 * 
 * @param main: Load balancer which distributes the work.
 * @param req: Request to be forwarded (relevant fields from the request are
 *        dynamically allocated, but the caller have to free them).
 * 
 * @return response_t* - Contains the response received from the server.
*******************************/
response_t *loader_forward_request(load_balancer_t *main, request_t *req);

/******************************
 * free_load_balancer() - Deallocate completely the memory used by a
 *		load balancer.
 *
 * @param main: Pointer to the load balancer's address with
 *		which we work.
*******************************/
void free_load_balancer(load_balancer_t **main);

#endif
