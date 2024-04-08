/**! @file hashtable_chaining.c
 *
 * @brief Basic implementation of a chained void pointer hashtable
 */
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashtable_chaining.h"

/**!
 * @struct Hashtable entry
 *
 * @brief Interior struct within the hashtable which holds data
 *
 * @var p_next  Pointer to the next entry "bucket"
 * @var p_key   String key of the entry
 * @var p_data  Pointer to data struct
 */
typedef struct entry
{

    struct entry *p_next;
    char         *p_key;
    void         *p_data;

} entry_t;

/**!
 * @struct Hashtable
 *
 * @brief Exterior struct that contains entries
 *
 * @var entry_t **pp_entries    Double pointer of entries
 * @var uint64_t size           Current size of the hashtable
 * @var uint64_t capacity       "Maximum" size of the hashtable
 * @var pthread_mutex_t lock    Mutex lock to prevent race conditions
 * 
 * @note Capacity will never technically be full due to chaining/buckets.
 *       However, a decision should be made on a reasonable size to create.
 */
typedef struct hashtable
{

    entry_t       **pp_entries;
    uint64_t        size;
    uint64_t        capacity;
    pthread_mutex_t lock;

} hashtable_t;

/**!
 * @brief Implementation of a polynomial rolling hash function.
 *
 * @param[in] p_key     Key to hash
 * @param[in] capacity  Capacity of the hashtable
 */
static long long hash_key(const char *p_key, uint64_t ht_capacity);

/**!
 * @brief Opens a file to populate a hashtable
 *
 * @param[in] p_hashtable   Hashtable to populate
 * @param[in] p_filename    Path to file
 *  
*/
static bool open_file(hashtable_t * p_hashtable, const char * p_filename);

hashtable_t *
ht_create (uint64_t capacity)
{
    hashtable_t *p_new_hashtable = NULL;
    if (0 == capacity)
    {
        fprintf(stderr, "Hashtable cannot have 0 capacity.\n");
        goto OUT;
    }

    p_new_hashtable = calloc(1, sizeof(hashtable_t));
    if (NULL == p_new_hashtable)
    {
        perror("calloc for hashtable in ht_create");
        errno = 0;
        goto OUT;
    }

    p_new_hashtable->pp_entries = calloc(capacity, sizeof(entry_t));
    if (NULL == p_new_hashtable)
    {
        perror("calloc for entries in ht_create");
        free(p_new_hashtable);
        errno = 0;
        goto OUT;
    }

    p_new_hashtable->size     = 0;
    p_new_hashtable->capacity = capacity;
    pthread_mutex_init(&(p_new_hashtable->lock), NULL);

    OUT:

    return p_new_hashtable;

} /* ht_create */

void
ht_destroy (hashtable_t *p_hashtable, p_del p_del_func)
{
    if ((NULL == p_hashtable) || (NULL == p_del_func))
    {
        fprintf(stderr, "Invalid argument passed to ht_destroy.\n");
        goto OUT;
    }

    // deny access to a hashtable during a destroy call
    pthread_mutex_lock(&(p_hashtable->lock));
    for (uint64_t idx = 0; idx < p_hashtable->capacity; ++idx)
    {
        entry_t *p_temp_entry = p_hashtable->pp_entries[idx];
        if (NULL != p_temp_entry)
        {
            // free buckets
            entry_t *p_bucket = p_temp_entry;
            while (NULL != p_bucket->p_next)
            {
                p_temp_entry = p_bucket;
                p_bucket     = p_bucket->p_next;
                p_del_func(p_temp_entry->p_data);
                free(p_temp_entry->p_key);
                free(p_temp_entry);
            }

            // free entry
            p_del_func(p_bucket->p_data);
            free(p_temp_entry->p_key);
            free(p_temp_entry);
        }
    }

    // free indexes
    free(p_hashtable->pp_entries);
    // unlock
    pthread_mutex_unlock(&(p_hashtable->lock));
    // destroy the mutex
    pthread_mutex_destroy(&(p_hashtable->lock));
    // finally, destroy the hashtable
    free(p_hashtable);

    OUT:

    return;

} /* ht_destroy */

void *
ht_find_key (hashtable_t *p_hashtable, const char *p_key)
{
    entry_t *p_temp_entry = NULL;
    if ((NULL == p_hashtable) || (NULL == p_key))
    {
        fprintf(stderr, "Invalid argument passed to ht_find.\n");
        goto OUT;
    }

    // hash the key
    long long ht_index = hash_key(p_key, p_hashtable->capacity);
    if (-1 == ht_index)
    {
        goto OUT;
    }

    p_temp_entry = p_hashtable->pp_entries[ht_index];
    // lock while conducting a search
    pthread_mutex_lock(&(p_hashtable->lock));
    // go through each entry while it is not null and the key doesn't match
    while ((NULL != p_temp_entry) && (0 != strcmp(p_temp_entry->p_key, p_key)))
    {
        p_temp_entry = p_temp_entry->p_next;
    }

    // end of the hashtable
    if (NULL == p_temp_entry)
    {
        pthread_mutex_unlock(&(p_hashtable->lock));
        goto OUT;
    }

    p_temp_entry = p_temp_entry->p_data;
    pthread_mutex_unlock(&(p_hashtable->lock));

    OUT:

    return p_temp_entry;

} /* ht_find_key */

bool
ht_insert (hashtable_t *p_hashtable, const char *p_key, void *p_data)
{
    bool b_result = false;
    if ((NULL == p_hashtable) || (NULL == p_key) || (NULL == p_data))
    {
        fprintf(stderr, "Invalid argument passed to ht_insert.\n");
        goto OUT;
    }

    // hash the key
    long long ht_index = hash_key(p_key, p_hashtable->capacity);
    if (-1 == ht_index)
    {
        goto OUT;
    }

    // check that the key doesn't already exist
    if (NULL != (ht_find_key(p_hashtable, p_key)))
    {
        fprintf(stderr, "Entry exists.\n");
        goto OUT;
    }

    // lock during the writing process
    pthread_mutex_lock(&(p_hashtable->lock));
    entry_t *p_new_entry = calloc(1, sizeof(*p_new_entry));
    if (NULL == p_new_entry)
    {
        perror("calloc for new entry in insert");
        errno = 0;
        pthread_mutex_unlock(&(p_hashtable->lock));
        goto OUT;
    }

    // allocate key length + 1 to null terminate
    p_new_entry->p_key = calloc((strlen(p_key) + 1), sizeof(char));
    if (NULL == p_new_entry->p_key)
    {
        perror("calloc for entry key in insert");
        free(p_new_entry);
        errno = 0;
        pthread_mutex_unlock(&(p_hashtable->lock));
        goto OUT;
    }

    // copy the key and data over
    strcpy(p_new_entry->p_key, p_key);
    p_new_entry->p_data = p_data;

    // add the entry to the hashtable in the corresponding position
    p_new_entry->p_next               = p_hashtable->pp_entries[ht_index];
    p_hashtable->pp_entries[ht_index] = p_new_entry;
    ++p_hashtable->size;
    b_result = true;
    pthread_mutex_unlock(&(p_hashtable->lock));

    OUT:

    return b_result;

} /* ht_insert */

void *
ht_compare (hashtable_t *p_hashtable, p_comp p_user_func)
{
    void *p_match = NULL;
    if ((NULL == p_hashtable) || (p_user_func))
    {
        fprintf(stderr, "Invalid argument passed to ht_match.\n");
        goto OUT;
    }

    pthread_mutex_lock(&(p_hashtable->lock));
    for (uint64_t idx = 0; idx < p_hashtable->capacity; ++idx)
    {
        if (NULL != p_hashtable->pp_entries[idx])
        {
            entry_t *p_temp_entry = p_hashtable->pp_entries[idx];
            while (NULL != p_temp_entry)
            {
                // user function should return a pointer to the entry
                p_match = p_user_func(p_temp_entry->p_data);
                // leaves when the condition is met
                if (NULL != p_match)
                {
                    pthread_mutex_unlock(&(p_hashtable->lock));
                    goto OUT;
                }

                // otherwise, continue to move through the entries
                p_temp_entry = p_temp_entry->p_next;
            }
        }
    }

    pthread_mutex_unlock(&(p_hashtable->lock));

    OUT:

    return p_match;

} /* ht_match*/

bool
ht_delete (hashtable_t *p_hashtable, const char *p_key, p_del p_del_func)
{
    bool b_result = false;
    if ((NULL == p_hashtable) || (NULL == p_key))
    {
        fprintf(stderr, "Invalid argument passed to ht_find.\n");
        goto OUT;
    }

    // hash the key
    long long ht_index = hash_key(p_key, p_hashtable->capacity);
    if (-1 == ht_index)
    {
        goto OUT;
    }

    entry_t *p_temp_entry = p_hashtable->pp_entries[ht_index];
    entry_t *p_prev_entry = NULL;
    pthread_mutex_lock(&(p_hashtable->lock));
    // go through each entry while it is not null and the key doesn't match
    while ((NULL != p_temp_entry) && (0 != strcmp(p_temp_entry->p_key, p_key)))
    {
        // keep track of entries once the desired node is found
        p_prev_entry = p_temp_entry;
        p_temp_entry = p_temp_entry->p_next;
    }

    // end of the hashtable
    if (NULL == p_temp_entry)
    {
        pthread_mutex_unlock(&(p_hashtable->lock));
        goto OUT;
    }

    // deleting the beginning of the entries
    if (NULL == p_prev_entry)
    {
        p_hashtable->pp_entries[ht_index] = p_temp_entry->p_next;
    }
    else
    {
        // deleting from anywhere other than the beginning
        p_prev_entry = p_temp_entry->p_next;
    }

    p_del_func(p_temp_entry->p_data);
    free(p_temp_entry);
    b_result = true;
    pthread_mutex_unlock(&(p_hashtable->lock));

    OUT:

    return b_result;

} /* ht_delete */

void
ht_print(hashtable_t * p_hashtable)
{
    if (NULL == p_hashtable)
    {
        fprintf(stderr, "Invalid argument passed to ht_print.\n");
        goto OUT;
    }

    puts("Beginning of table.");
    for (uint64_t idx = 0; idx < p_hashtable->capacity; ++idx)
    {
        if (NULL != p_hashtable->pp_entries[idx])
        {
            printf("Index: %u.\n", idx);
            entry_t * p_temp_entry = p_hashtable->pp_entries[idx];
            while (NULL != p_temp_entry)
            {
                printf("Key: %s.\n", p_temp_entry->p_key);
                p_temp_entry = p_temp_entry->p_next;
            }
        }
    }

    puts("End of table.");

    OUT:

    return;
} /* ht_print */

static long long
hash_key (const char *p_key, uint64_t ht_capacity)
{
    long long hash = -1;
    if ((NULL == p_key) || (0 == ht_capacity))
    {
        fprintf(stderr, "Invalid argument passed to hash_key.\n");
        goto OUT;
    }

    /*
        set prime for the following in regards to string type:
        lowercase-only: 31
        uppercase and lowercase: 53
        ASCII-printable: 97
        else: 29791
    */
    const int choice = 97;
    /*
        an arbitrarily large prime number should be selected for large. the
        probability of two random strings colliding will be 1/large.
    */
    const int large      = 1e9 + 9;
    long long power      = 1;
    size_t    key_length = strlen(p_key);
    for (size_t idx = 0; idx < key_length; ++idx)
    {
        hash  = (hash + (p_key[idx] - 'a' + 1) * power) % large;
        power = (power * choice) % large;
    }

    // ensure the hash fits inside a hashtable index
    hash %= ht_capacity;

    OUT:

    return hash;

} /* hash_key */

static bool open_file(hashtable_t * p_hashtable, const char * p_filename)
{
    bool b_result = false;
    if ((NULL == p_hashtable) || (NULL == p_filename))
    {
        fprintf(stderr, "Invalid argument passed to open_file.\n");
        goto OUT;
    }



    OUT:

    return b_result;

} /* open_file */