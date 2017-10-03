/* a hash-table implementation in c */

/*
    hashing algorithm: hashval = *s + 31 * hashval
    resolves collisions using linked lists
*/

#ifndef HASH
#define HASH

typedef struct hashtable hashtable_t;

typedef enum datatype {STRING, INTEGER} datatype_t;

hashtable_t *ht_create(size_t size, datatype_t type);
/*
    ht_create: allocates hashtable_t on the heap with a size of size and a 
    type of type. returns a pointer to the allocated memory or NULL on
    failure. remember to use ht_free() when finished.
    
*/

int ht_insert(hashtable_t **ht, char *k, void *v);
/*
    ht_insert: inserts a key-val pair with key k and val v in the hash table
    double pointed by ht. double pointer used to modify pointer to ht in case
    hash table is resized. void * of v is casted to the type of the hash
    table. returns 0 on success and <0 on failure.
*/

int ht_delete(hashtable_t **ht, char *k);
/*
    ht_delete: deletes the key-val pair with key k. double pointer is used to
    modify pointer to ht if hash table is resized. returns 0 on success and <0
    on failure.
*/

void *ht_get(hashtable_t *ht, char *k);
/*
    ht_get: returns a pointer to the key-val pair's value whose key is k.
*/

void ht_free(hashtable_t *ht);
/*
    ht_free: frees the hash table pointed to by ht.
*/

#endif