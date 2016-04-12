/*
 * lists.c
 *
 *  Created on: Mar 7, 2012
 *      Author: nikolas
 */

#include "lists.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>

/* Returns the next node of a list */
list_t list_nextof(list_t node)
{
	return node->next;
}

/* Creates list */
list_t list_create()
{
	return NULL;
}

/* Destroys a list */
void list_destroy(list_t * list)
{
	list_t next, tmp = *list;
	for (tmp = *list; tmp != NULL; tmp = next)
	{
		next = list_nextof(tmp);
		free(tmp->data);
		free(tmp);
	}
	*list = NULL;
}

/* Makes a list empty */
int list_empty(list_t list)
{
	return (list == NULL);
}

/* GetsList first node*/
list_t list_get_first_node(list_t * list){
	return *list;
}

list_t list_get_last_node(list_t * list){
	list_t tmp;
	if(list_empty(*list))
		return NULL;
	for (tmp = *list; tmp->next != NULL; tmp = list_nextof(tmp));
	return tmp;
}

void list_node_get_object(list_t node, void * obj, size_t obj_size)
{
	assert( !list_empty(node) );
	memcpy(obj, node->data, obj_size);
}

int list_push_front(list_t * list, void * obj, size_t obj_size)
{
	list_t nnode, next = *list;

	nnode = malloc( sizeof(struct list_node) );
	if(nnode == NULL)
		return OUT_OF_MEMORY;

	nnode->data = malloc( obj_size );
	if(nnode->data == NULL)
		return OUT_OF_MEMORY;

	nnode->next = next;
	memcpy(nnode->data, obj, obj_size);
	*list = nnode;
	return 0;
}

int list_push_back(list_t * list, void * obj, size_t obj_size)
{
	list_t tmp;
	if(list_empty(*list))
		return list_push_front(list, obj, obj_size);

	for (tmp = *list; tmp->next != NULL; tmp = list_nextof(tmp)){}

	return list_push_front(&(tmp->next), obj, obj_size);
}

int list_insert(list_t * list, list_t prev, void * obj, size_t obj_size)
{
	if( list_empty(*list) || prev == NULL )
	{
		return list_push_front(list, obj, obj_size);
	}
	
	return list_push_front(&(prev->next), obj, obj_size);
}

void list_erase(list_t * list, list_t prev)
{
	list_t tmp; 
	assert(! list_empty(*list) );

	if (prev == NULL)
	{
		list_t next = list_nextof(*list);

		free ((*list)->data);
		free (*list);
		*list = next;
		return;
	}

	tmp = list_nextof(prev);
	if (tmp)
	{
		prev->next = list_nextof(tmp);
		free (tmp->data);
		free (tmp);
	}
}

void list_pop_front(list_t * list, void * obj, size_t obj_size)
{
	assert(! list_empty(*list) );

	list_node_get_object (*list, obj, obj_size);
	list_erase(list, NULL);
}

void list_pop_back(list_t * list, void * obj, size_t obj_size)
{
	list_t before_last = NULL, last;
	assert( !list_empty(*list) );

	for (last = *list; last->next != NULL; last = list_nextof(last))
	{
		before_last = last;
	}

	list_node_get_object (last, obj, obj_size);
	list_erase(list, before_last);
}
