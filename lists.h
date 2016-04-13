/*
 * lists.h
 *
 *  Created on: Mar 7, 2012
 *      Author: nikolas
 */

#ifndef LISTS_H_
#define LISTS_H_

#include <stdlib.h>

struct list_node;
typedef struct list_node * list_t;

#define OUT_OF_MEMORY 1

/* List Structure. Next node and data */
struct list_node {
	struct list_node * next;
	void * data;
};

list_t list_nextof(list_t);
list_t list_create();
void list_destroy(list_t *);
int list_empty(list_t);
list_t  list_get_last_node(list_t *);
list_t list_get_first_node(list_t *);
void list_node_get_object(list_t, void *, size_t);
int list_push_front(list_t *, void *, size_t);
int list_push_back(list_t * , void *, size_t);
int list_insert(list_t *, list_t, void *, size_t);
void list_erase(list_t *, list_t);
void list_pop_front(list_t *, void *, size_t);
void list_pop_back(list_t *, void *, size_t);

#endif /* LISTS_H_ */
