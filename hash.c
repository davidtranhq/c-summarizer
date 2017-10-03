#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hash.h"

#define HIGHLOAD 0.75
#define LOWLOAD 0.25

typedef struct tableentry /* hashtab entry */
{
    struct tableentry *next;
    char *key;
    void *val;
} tableentry_t;

typedef struct hashtable
{
    datatype_t type;
    size_t size;
    size_t load; /* number of keys filled */
    struct tableentry **tab;
} hashtable_t;

static unsigned hash(char *s);
/*
    hash: returns an unsigned integer using K&R hashing algorithm
    from string s.
*/

static int insert_te(hashtable_t *ht, char *k, void *v);
/*
    insert_te: used by ht_insert() to insert a tableentry_t into
    the hash table. returns 0 on succes and <0 on failure.
*/

static int delete_te(hashtable_t *ht, char *k);
/*
    delete_te: used by ht_delete() to delete the tableentry_t with
    key k from the hash table pointed to by ht. returns 0 on success
    and <0 on failure.
*/

static tableentry_t *lookup(hashtable_t *ht, char *k);
/*
    lookup: returns the tableentry_t with key k from hash table
    *ht. returns NULL if not found.
*/

static int resize(hashtable_t **ht, size_t size);
/*
    resize: resizes the hash table **ht to a size of size. returns 0 on
    success and <0 on failure.
*/

static tableentry_t *alloc_te(char *k, void *v, datatype_t type);
/*
    alloc_te: allocates memory on the heap for a tableentry_t with a key of
    k and a val of v. The data type of the hash table is needed to properly
    allocated the value. returns a pointer to the allocated memory or NULL on 
    failure.
*/

static void free_te_list(tableentry_t *te);
/*
    free_te_list: frees the linked list of tableentry_t starting at *te.
*/

static void free_te(tableentry_t *te);
/*
    free_te: used by free_te_list() to free the tableentry_t's contents.
*/

static int *intdup(int *i);
/*
    intdup: allocates memory on the heap for an integer and initializes it to
    the value of *i. returns a pointer to the allocated memory or NULL on failure.
*/

hashtable_t *ht_create(size_t size, datatype_t type)
{
    hashtable_t *ht = NULL;
    if ((ht = malloc(sizeof(hashtable_t))) == NULL)
        return NULL;
    /* allocate ht's table */
    if ((ht->tab = malloc(sizeof(tableentry_t *) * size)) == NULL)
    {
        free(ht);
        return NULL;
    }
    /* null-initialize table */
    size_t i;
    for (i = 0; i < size; i++)
        ht->tab[i] = NULL;
    ht->size = size;
    ht->type = type;
    ht->load = 0;
    return ht;
}

int ht_insert(hashtable_t **ht, char *k, void *v)
{
    tableentry_t *te;
    /* unique entry */
    if ((te = lookup(*ht, k)) == NULL)
    {
        if (insert_te(*ht, k, v) < 0)
            return -1;
        /* increase table size if load exceeds HIGHLOAD */
        if (++((*ht)->load) > (*ht)->size * HIGHLOAD)
            if (resize(ht, (*ht)->size * 2) < 0)
                return -1;
    }
    /* replace val of previous entry */
    else
    {
        free(te->val);
        switch ((*ht)->type)
        {
            case STRING:
                if ((te->val = strdup(v)) == NULL)
                    return -1;
                break;
            case INTEGER:
                if ((te->val = intdup(v)) == NULL)
                    return -1;
                break;
            default:
                return -1;
        }
    }
    return 0;
}

int ht_delete(hashtable_t **ht, char *k)
{
    if (lookup(*ht, k) == NULL)
        return -1;
    else
    {
        if (delete_te(*ht, k) < 0)
            return -1; 
        /* resize ht if load balance falls below LOWLOAD */
        if (--((*ht)->load) < (*ht)->size * LOWLOAD)
            if (resize(ht, (*ht)->size/2) < 0)
                return -1;
    }
    return 0;
}

void *ht_get(hashtable_t *ht, char *k)
{
    tableentry_t *te;
    if ((te = lookup(ht, k)) == NULL)
        return NULL;
    return te->val;
}

void ht_free(hashtable_t *ht)
{
    size_t i;
    if (ht)
    {
        for (i = 0; i < ht->size; i++)
            if (ht->tab[i] != NULL)
                free_te_list(ht->tab[i]);
        free(ht);
    }
}

static unsigned hash(char *s)
{
    unsigned hashval;
    for (hashval = 0; *s != '\0'; s++)
        hashval = *s + 31 * hashval;
    return hashval;
}

static int insert_te(hashtable_t *ht, char *k, void *v)
{
    tableentry_t *te;
    if ((te = alloc_te(k, v, ht->type)) == NULL)
        return -1;
    unsigned hashval = hash(k) % ht->size;
    /* insert at beginning of linked list */
    te->next = ht->tab[hashval]; 
    ht->tab[hashval] = te;
    return 0;
}

static int delete_te(hashtable_t *ht, char *k)
{
    tableentry_t *te, *prev;
    unsigned hashval = hash(k) % ht->size;
    te = ht->tab[hashval];
    /* point head to next element if deleting head */
    if (strcmp(te->key, k) == 0)
    {
        ht->tab[hashval] = te->next;
        free_te(te);
        return 0;
    }
    /* otherwise look through, keeping track of prev to reassign its ->next */
    for (; te != NULL; te = te->next)
    {
        if (strcmp(te->key, k) == 0)
        {
            prev->next = te->next;
            free_te(te);
            return 0;
        }
        prev = te;
    }
    return -1; /* not found */
}

static tableentry_t *lookup(hashtable_t *ht, char *k)
{
    tableentry_t *te;
    /* step through linked list */
    for (te = ht->tab[hash(k) % ht->size]; te != NULL; te = te->next)
        if (strcmp(te->key, k) == 0)
            return te; /* found */
    return NULL; /* not found */
}

static int resize(hashtable_t **ht, size_t size)
{
    hashtable_t *nht; /* new hashtable */
    nht = ht_create(size, (*ht)->type);
    /* rehash */
    size_t i;
    tableentry_t *te;
    /* loop through hashtable */
    for (i = 0; i < (*ht)->size; i++)
        /* loop through linked list */
        for (te = (*ht)->tab[i]; te != NULL; te = te->next)
            /* insert & rehash old vals into new ht */
            if (ht_insert(&nht, te->key, te->val) < 0)
                return -1;
    ht_free(*ht);
    *ht = nht;
    return 0;
}

static tableentry_t *alloc_te(char *k, void *v, datatype_t type)
{
    tableentry_t *te = NULL;
    int status = 0;
    /* alloc struct */
    if ((te = malloc(sizeof(*te))) == NULL)
        status = -1;
    /* alloc key */
    if ((te->key = strdup(k)) == NULL)
        status = -1;
    /* alloc value */
    int *d;
    char *s;
    switch (type)
    {
        case STRING:
            s = (char *) v;
            if ((te->val = strdup(s)) == NULL)
                status = -1;
            break;
        case INTEGER:
            d = (int *) v;
            if ((te->val = intdup(d)) == NULL)
                status = -1;
            break;
        default:
            status = -1;
    }
    if (status < 0)
    {
        free_te_list(te);
        return NULL;
    }
    te->next = NULL;
    return te;
}

static void free_te_list(tableentry_t *te)
{
    tableentry_t *next;
    while (te != NULL)
    {
        next = te->next;
        free_te(te);
        te = next;
    }
}

static void free_te(tableentry_t *te)
{
    free(te->key);
    free(te->val);
    free(te);
}

static int *intdup(int *i)
{
    int *new;
    if ((new = malloc(sizeof(int))) == NULL)
        return NULL;
    *new = *i;
    return new;
}