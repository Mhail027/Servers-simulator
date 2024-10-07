// Copyright Necula Mihail 313CAa 2023-2024
#include <string.h>
#include "load_balancer.h"
#include "server.h"

load_balancer_t *init_load_balancer(bool enable_vnodes)
{
	// Allocate memory for the load balancer's structure.
	load_balancer_t *main = (load_balancer_t *)malloc(sizeof(load_balancer_t));
	DIE(main == NULL, "malloc() failed\n");

	// Allocate memory for every complex field from structure.
	main->server = (server_t **)malloc(sizeof(server_t *));
	DIE(main->server == NULL, "malloc() failed\n");

	// Initialize the parameters of the load balancer.
	main->size = 0;
	main->max_size = 1;
	main->replicas = (enable_vnodes == false) ? 1 : 3;
	main->hash_function_servers = hash_uint;
	main->hash_function_docs = hash_string;

	// Return the created load balancer.
	return main;
}


void load_balancer_double_servers(load_balancer_t *main)
{
	main->max_size *= 2;
	main->server = (server_t **)realloc(main->server,
					main->max_size * sizeof(server_t *));
	DIE(main->server == NULL, "malloc() failed\n");
}

void combine_databases(server_t *dst, server_t *src)
{
	for (u_int i = 0; i < (*src->local_db)->hmax; ++i) {
		ll_t *curr_list = (ll_t *)(*src->local_db)->buckets[i];
		ll_node_t *curr_node = curr_list->head;

		while(curr_node) {
			info_t *pair = (info_t *)curr_node->data;
			doc_t *file = *(doc_t **)pair->value;
			doc_t *file_dup = init_doc(file->name, file->content);
			db_add_doc(dst, file_dup);
			curr_node = curr_node->next;
		}
	}
}

u_int lb_add_server_in_array(load_balancer_t *main, server_t *new_s)
{
	// Find the position where should be put the server in
	// the array of servers.
	u_int pos = main->size;
	for (u_int i = 0; i < main->size; ++i) {
		if (main->server[i]->hash_id > new_s->hash_id) {
			pos = i;
			break;
		}
		if (main->server[i]->hash_id ==  new_s->hash_id) {
			if (main->server[i]->id > new_s->id) {
				pos = i;
				break;
			}
		}
	}

	// Shifts all elements from the right to right with +1 position.
	for (u_int i = main->size; i > pos; --i)
		main->server[i] = main->server[i - 1];

	// Now, we can put the server in array.
	main->server[pos] = new_s;

	// Increment the numbers of servers.
	main->size++;

	// Return the position of the new server in the array of servers.
	return pos;
}

server_t
*loader_add_replica(load_balancer_t *main, u_int server_id, u_int cache_size)
{
	// Verify if we must double the size of the array of servers.
	if (main->size == main->max_size)
		load_balancer_double_servers(main);

	// Create the new server / the destination server.
	server_t *dst_srv = init_server(server_id, cache_size);
	dst_srv->hash_id = main->hash_function_servers(&server_id);

	// Add the new server in the array of servers.
	u_int pos_dst = lb_add_server_in_array(main, dst_srv);

	// Find the source server.
	u_int pos_src = (pos_dst + 1) % main->size;
	server_t *src_srv = main->server[pos_src];

	// Empty the task queue of the source server.
	do_tasks_from_queue(src_srv);

	// Verify every file from the source server if must be moved
	// in the new server.

	// Find the server from ring which is before the sorce server,
	// witout to take in consideration the new server.
	u_int pos_prev = (pos_src + main->size - 2) % main->size;
	server_t *prev_srv = main->server[pos_prev];

	for (u_int i = 0; i < (*src_srv->local_db)->hmax; ++i) {
		ll_t *curr_list = (ll_t *)(*src_srv->local_db)->buckets[i];
		ll_node_t * curr_node = curr_list->head;

		while(curr_node) {
			info_t *pair = (info_t *)curr_node->data;
			char *doc_name = (char *)pair->key;
			u_int hash_doc = main->hash_function_docs(doc_name);

			// Verify if the current doc is bewtwen the source
			// and the previous server. (We need to verify this for
			// the load balancers which have more than 1 replica for a server.)
			if (src_srv->hash_id > prev_srv->hash_id)
				if (hash_doc > src_srv->hash_id || hash_doc <= prev_srv->hash_id) {
					curr_node = curr_node->next;
					continue;
				}
			if (src_srv->hash_id < prev_srv->hash_id)
				if (hash_doc > src_srv->hash_id && hash_doc <= prev_srv->hash_id) {
					curr_node = curr_node->next;
					continue;
				}

			// Verify if the current doc must be moved.
			u_int move = 0;
			if (hash_doc < dst_srv->hash_id && dst_srv->hash_id < src_srv->hash_id)
				move = 1;
			if (hash_doc < dst_srv->hash_id && dst_srv->hash_id == src_srv->hash_id
			&& dst_srv->id < src_srv->id)
				move  = 1;
			if (hash_doc > src_srv->hash_id && src_srv->hash_id > dst_srv->hash_id)
				move = 1;
			if (hash_doc > src_srv->hash_id && src_srv->hash_id == dst_srv->hash_id
			&& src_srv->id > dst_srv->id)
				move = 1;
			if (hash_doc > src_srv->hash_id && hash_doc < dst_srv->hash_id)
				move = 1;

			if (move) {
				doc_t *file = *(doc_t **)pair->value;
				// Do a duplicate of the file.
				doc_t *file_dup = init_doc(file->name, file->content);
				// Remove the file from the database of source server.
				curr_node = curr_node->next;
				db_remove_doc(src_srv, file_dup->name);
				// Remove it also from the cache if it's there.
				if (lru_cache_has_key(src_srv->cache, file_dup->name))
					lru_cache_remove(src_srv->cache, file_dup->name);
				// Add the duplicate file in the destination server.
				db_add_doc(dst_srv, file_dup);
			} else {
				curr_node = curr_node->next;
			}
		}
	}

	// Return the new server which was added.
	return dst_srv;
}

server_t *create_replica_of_server(server_t *s, u_int id, u_int hash_id)
{
	// Allocate memory fo the replica.
	server_t *replica = (server_t *)malloc(sizeof(server_t));
	DIE(replica == NULL, "malloc() failed\n");

	// Add the resources of the servers in replica.
	replica->cache = s->cache;
	replica->local_db = s->local_db;
	replica->task_queue = s->task_queue;

	// Initialize the parameters of replica.
	replica->id = id;
	replica->hash_id = hash_id;

	return replica;
}


void loader_add_server(load_balancer_t *main, u_int server_id, u_int cache_size)
{
	// 1 replica for server
	if(main->replicas == 1) {
		loader_add_replica(main, server_id, cache_size);
		return;
	}

	// 3 replicas for server

	// Firstly, we add the 3 replicas in the system, independently.
	// (rpl = replica)
	server_t *rpl_1 = loader_add_replica(main, server_id, cache_size);
	server_t *rpl_2 = loader_add_replica(main, 100000 + server_id, cache_size);
	server_t *rpl_3 = loader_add_replica(main, 200000 + server_id, cache_size);

	// After, we combine those replicas to have the resources put at common.

	// Add the docs from the database of the second replica
	// in the database of the first replica
	combine_databases(rpl_1, rpl_2);

	// Free the memory of the second replica's resources.
	free_resources_of_server(rpl_2);

	// Create the second replica how should be (at common with the first one).
	u_int rpl_2_id = 100000 + server_id;
	u_int hash_id = main->hash_function_servers(&rpl_2_id);
	server_t *new_rpl_2 = create_replica_of_server(rpl_1, rpl_2_id, hash_id);

	// Add the fields of the new replica in the old replica.
	*rpl_2 = *new_rpl_2;
	free(new_rpl_2);

	// Add the docs from the database of the third replica
	// in the database of the first and second replicas.
	combine_databases(rpl_1, rpl_3);

	// Free the memory of the third replica resource's.
	free_resources_of_server(rpl_3);

	// Create the third replica how should be (at common with the first one
	// and the scond one).
	u_int rpl_3_id = 200000 + server_id;
	hash_id = main->hash_function_servers(&rpl_3_id);
	server_t *new_rpl_3 = create_replica_of_server(rpl_1, rpl_3_id, hash_id);

	// Add the fields of the new replica in the old replica.
	*rpl_3 = *new_rpl_3;
	free(new_rpl_3);
}

void loader_remove_replica(load_balancer_t *main, u_int server_id)
{
	// Find the position of the source server in the
	// load balancer's array.
	u_int src_pos;
	for (u_int i  = 0; i < main->size; ++i) {
		if (main->server[i]->id == server_id) {
			src_pos = i;
			break;
		}
	}

	// Find the source server.
	server_t *src_srv = main->server[src_pos];

	// Do every request from the tasks queue of source server.
	do_tasks_from_queue(src_srv);

	// Find the position of the destination server in the
	// load balancer's array.
	u_int dst_pos = (src_pos + 1) % main->size;

	// Find the destination server.
	server_t *dst_srv = main->server[dst_pos];

	// Add every doc from source in destination.
	combine_databases(dst_srv, src_srv);

	// Free the memory allocated for the source server.
	free_server(&main->server[src_pos]);

	// Fill the gap from the load balancer's array
	// made by the remove operation.
	for (u_int i = src_pos; i < main->size - 1; ++i)
		main->server[i] = main->server[i + 1];

	// Decrement the number of server from the systme.
	main->size--;
}

void loader_remove_server(load_balancer_t *main, u_int server_id)
{
	// 1 replica for server
	if (main->replicas == 1) {
		loader_remove_replica(main, server_id);
		return;
	}

	// 3 replicas for server

	// Find the replicas of the server.
	// (rpl  = replica)
	u_int mom_srv_id = server_id % 100000;
	u_int pos_rpl_1, pos_rpl_2, pos_rpl_3;
	server_t *rpl_1 = NULL, *rpl_2 = NULL, *rpl_3 = NULL;
	for (u_int i  = 0; i  < main->size; ++i) {
		if (main->server[i]->id == mom_srv_id) {
			rpl_1 = main->server[i];
			pos_rpl_1 = i;
		}
		if (main->server[i]->id == 100000 + mom_srv_id) {
			rpl_2 = main->server[i];
			pos_rpl_2 = i;
		}
		if (main->server[i]->id == 200000 + mom_srv_id) {
			rpl_3 = main->server[i];
			pos_rpl_3 = i;
		}
		if (rpl_1 && rpl_2 && rpl_3)
			break;
	}

	// Create a tempoarry load balancer when we put those 3 replicas,
	// independently (without to have the resources at common).
	// In this system, every replica becomes a independent server.
	load_balancer_t *lb_tmp = init_load_balancer(false);

	// Add first replica.
	lb_tmp->server[0] = rpl_1;
	lb_tmp->size = 1;
	// Add second replica.
	loader_add_server(lb_tmp, rpl_2->id, 1);
	free(rpl_2);
	// Add third replica.
	loader_add_server(lb_tmp, rpl_3->id, 1);
	free(rpl_3);

	// Change the replicas from the main load balancer
	// with the servers from the temporary load balancer.
	for (u_int i  = 0; i  < 3; ++i) {
		if (lb_tmp->server[i]->id == mom_srv_id)
			main->server[pos_rpl_1] = lb_tmp->server[i];
		if (lb_tmp->server[i]->id == 100000 + mom_srv_id)
			main->server[pos_rpl_2] = lb_tmp->server[i];
		if (lb_tmp->server[i]->id == 200000 + mom_srv_id)
			main->server[pos_rpl_3] = lb_tmp->server[i];
	}

	// Remove the servers which now have 1 replica and are
	// coresponding to the replicas which we transformed.
	loader_remove_replica(main, mom_srv_id);
	loader_remove_replica(main, 100000 + mom_srv_id);
	loader_remove_replica(main, 200000 + mom_srv_id);

	// Free the unnecesary memory.
	free(lb_tmp->server);
	free(lb_tmp);
}

response_t *loader_forward_request(load_balancer_t *main, request_t *req)
{
	// Find the hash of the dos's name.
	u_int hash_doc = main->hash_function_docs(req->doc_name);

	// Find the server to which the request must be sent.
	u_int pos = 0;
	for (u_int i = 0; i < main->size; ++i)
		if (main->server[i]->hash_id > hash_doc) {
			pos = i;
			break;
		}

	// Send the request further.
	server_t *srv = main->server[pos];
	response_t *rsp = server_handle_request(srv, req);

	// Return the response of the request.
	return rsp;
}

void free_load_balancer(load_balancer_t **main)
{
	// Get the load_balancer's address.
	load_balancer_t *lb = *main;

	// Free the memory of every server.
	u_int *visited = (u_int *)calloc(99999, sizeof(u_int));
	DIE(visited == NULL, "calloc() failed\n");
	for (u_int i = 0; i < lb->size; ++i) {
		u_int mom_srv_id = lb->server[i]->id % 100000;
		if (!visited[mom_srv_id])
			free_server(&lb->server[i]);
		else
			free(lb->server[i]);
		visited[mom_srv_id] = 1;
	}
	free(visited);

	// Free the array of server's memory.
	free(lb->server);

	// Free the memory of the load balancer structure.
	free(lb);

	// Lose the address of the load balancer, which
	// doesn't exist anymore.
	*main = NULL;
}

