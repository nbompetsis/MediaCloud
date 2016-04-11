/*
 * hash.c
 *
 *  Created on: Mar 7, 2012
 *      Author: nikolas
 */

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "hash.h"
#include "lists.h"

/* Initialize hash table */
int hash_init(hash * hashPtr, int hsize){
	int i;
	hashPtr->size = hsize;
	if((hashPtr->table = malloc(hsize * sizeof(list_t))) == NULL)
		return OUT_OF_MEMORY;
	for(i = 0; i < hsize; i++)
		hashPtr->table[i] = list_create();
	return 0;
}

/* Destroy hash table */
void hash_destroy(hash *hashPtr){
	int i;
	for(i = 0; i < hashPtr->size; i++)
		list_destroy(&hashPtr->table[i]);
	free(hashPtr->table);
}

/* Return hash index*/
int getHashIndex(int num, const hash * H){
	return num % H->size;
}

/* add node in hash[index].collisionlist */
void add_in_hash(hash * H, list_t node, int index){
	/* printf("pointer = %d\n", node); */
	list_push_back(&H->table[index], &node, sizeof(list_t));
}

/* remove from hash[index].collisionlist node after "bef" */
void remove_from_hash(hash * H, int index, list_t bef){
	list_erase(&H->table[index], bef);
}

list_t * get_from_hash_value(const hash * H, int index){
	return &(H->table[index]);
}
