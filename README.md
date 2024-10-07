Name: Necula Mihail <br>
Group: 313CAa

## Readme Homework 2

### 1. The description of the program ###

We have 3 main files: "lru_cache.c", "server.c" and "load_balancer.c".

***A. LRU_CACHE.C***

Defines 2 structures:

- doc_t ---> in which we save 2 strings (the name and the content of a document)
- lru_cache_t ---> which has 4 fields: max_size (the maximum number of docs <br>
which can be stored in cache), size (the current number of docs which are <br>
stored in cache), list_docs (a circular doubly linking list wich saves the addresses <br>
of the docs from cache), ht_docs (a hahstable where are stored pairs of next type: <br>
name of doc - node's address from the list that saves the address of the doc)

In the list, the docs are saved in the order of the time when they were use last <br>
time, from the least to the most recently. So first node send us at the least recent <br>
document. When the cache is full, will eliminate the document which is saved in the head <br>
of the list.

To access a random document from the cache will use the hashable because <br>
is more time-efficient.

This file defines the next functions:

- init_lru_cache() - allocate memory for a cache
- lru_cache_is_full() - verify if is reached the capacity of the cache / if can't be added <br>
docs without to eliminate others
- free_lru_cache() - deallocate the memory of a cache
- lru_cache_key() - verify if a doc is in a cache, using the hashtable and the name of the doc
- lru_cache_put() - put a document in a cache
- lru_cache_get() - return the address of a doc from a cache after what is provided with <br>
the name of the doc <br>
- lru_cache_remove() - remove a doc from a cache

***B. SERVER.C***

Defines 3 structures:

- request_t ---> which has 3 fields: type (0 means EDIT request, while 1 means GET request), <br>
doc_name (string that saves the name of the document in which we are interested), doc_content <br>
(if we have an EDIT request, we must know the content that will be written.).
- server_t ---> which has 5 fields: cache (which is of type lru_cache_t), local_db (which is <br>
implemented using an hashtable which saves pairs of next type: doc_name - address of the doc), <br>
task_queue (a queue in which are stored the edit requests; we don't do those requests instantly <br>
after we receive them; we wait until the user gives us a get request to do the edits; so when have a <br>
get request, we empty this queue) , id, hash_id (we'll explain this field later - when we get at load <br>
balancer)
- response_t ---> in which save 2 strings (the log and the response of a server after receiving a request) <br>
and 1 int (the id of the server which worked with the request)

This file defines the next functions:

- free_request() - deallocate the memory of a cache
- key_doc_free_function() - used by hashmap (database) to deallocate the memory of a pair
- duplicate_request() - make a duplicate of the given request
- create_response() - allocate memory for a response
- init_doc() - create and initialize a doc using the given parameters
- db_redistribute_docs() - change the number of buckets from the ht of a db and redistribute <br>
the docs in the new ht
- db_increase_hmax() - increase the number of buckets from a database's hashtable
- db_add_doc() - add a doc in the local database of a server
- db_remove_doc() - remove a doc from the local database of a server
- init_server() - allocate and initialize a server using the given parameters
- free_resources_of_server() - deallocate the memory of a server, excepting its structure
- free_server() - deallocate completely the memory of a server
- server_edit_document() - do a EDIT request and return a response
- server_get_document() - do a GET request and return a response
- do_tasks_from_queue() - empty the task queue of a server and print every response given by <br>
the execution of a request
- server_handle_request() - handle a request using the given server and the previous functions

***C. LOAD_BALANCER.C***

Define 1 structure:

- load_balancer_t - which has 6 fields: server (array with addresses of servers), size (the number <br>
of servers from array), max_size (the maximum number of servers which can be stored), replicas <br>
(number of replicas for every server), hash_function_servers (pointer to the function which hash the id <br>
of a server; using the hashed IDs, will place the servers in a hash ring), hash_function_docs <br>
(pointer to the function which hash the name of a doc; using the hashed names, will place the docs, <br>
together with the servers, in hash ring).

The servers from array will be sorted after the hashed IDs. So, will be in the order from the ring.

This file defines the next functions:

- init_load_balancer() - create and initialize a load balancer with the given parameters.
- init_load_balancer_double_servers() - double number of servers which can be stored in a load balancer <br>
- combine_databases() - receive 2 servers and add the docs from the data base of one of them in the data <bs>
of the other server
- lb_add_server_in_array - add a server in the array of a load balancer
- loader_add_replica() - add the replica of a server in a load balancer; this function considers that the <br>
server has an unique replica
- create_replica_of_server() - create a replica for a server
- loader_add_server() - add a server in a load balancer (with all its replicas)
- loader_remove_replica() - remove a replica of a server from a load balancer; this function considers that the <br>
server has an unique replica
- loader_remove_server() - remove a server from a load balancer (with all its replicas)
- loader_forward_request() - receive a request, decide for which server is it and send it to that server
- free_load_balancer() - deallocate completely the memory of a load balancer

***D. THE GENERAL FLOW***


1. init_load_balancer() <br>

2. add server, then we have: <br>
1 replica: loader_add_server() -> loader_add_replica() <br>
3 replicas: loader_add_server() -> loader_add_replica() 3 times -> combine_databses() 2 times <br>
-> create_replica_of_server() 2 timmes <br>

3. remove server, then we have: <br>
1 replica: loader_remove_server() -> loader_remove_replica() <br>
3 replicas: loader_remove_server() -> init_load_balancer() -> add manualy first replica <br>
-> loader_add_replica() 2 times -> combine the load balancers which we have <br>
-> loader_remove_replica() 3 times <br>

4. edit request, than we have: <br>
loader_forward_request() -> server_handle_request() <br>

5. get request, than we have: <br>
loader_forward_request() -> server_handle_request() -> do_tasks_from_queue() <br>
-> server_edit_document -> db_add_doc() and lru_cache_put() -> server_get_document() <br>

6. free_load_balancer()

### 2. Comments about the homework ###

***Do I think that I could have done a better implementation?***

Yes. I could have done a resizbale hashmap and for cache. Also, I could have done a structure where to save the <br>
informations about a replica, with the fields: mom_server, id, hashed_id.