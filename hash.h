/*
 * hash.h
 *
 *  Created on: Mar 7, 2012
 *      Author: nikolas
 */

#ifndef HASH_H_
#define HASH_H_

#include <stdlib.h>
#include "lists.h"

#define OUT_OF_MEMORY 1

typedef struct hash{
	int		size;	/* hash table size */
	list_t * 	table;	/* hash table contains linked lists to handle collisions */
}hash;

int hash_init(hash *, int);
void hash_destroy(hash *);
int getHashIndex(int, const hash *);
void add_in_hash(hash *, list_t , int);
void remove_from_hash(hash *, int, list_t);
list_t * get_from_hash_value(const hash *, int);

#endif /* HASH_H_ */
