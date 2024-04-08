/**! @file hashtable_chaining.h
 *
 * @brief Basic implementation of a chained void pointer hashtable
 */
#ifndef HASHTABLE_CHAINING_H
#define HASHTABLE_CHAINING_H

#include <stdbool.h>
#include <stdint.h>

typedef struct hashtable hashtable_t;
// function pointer to return a pointer during comparison
typedef void * (*p_comp)(void *);
// function pointer for deleting
typedef void (*p_del)(void *);

/**!
 * @brief Creates a hashtable with N indexes
 *
 * @param[in] capacity  Size of the hashtable
 *
 * @return SUCCESS: hashtable_t \
 * @return FAILURE: NULL
 *
 * @note Size cannot be 0. Size should also be a prime number.
 *
 */
hashtable_t *ht_create(uint64_t capacity);

/**!
 * @brief Destroys a hashtable
 *
 * @param[in] p_hashtable   Hashtable to be destroyed
 * @param[in] p_del_func    Function to delete data types within the hashtable
 *
 */
void ht_destroy(hashtable_t *p_hashtable, p_del p_del_func);

/**!
 * @brief Finds an entry in a hashtable based on the key
 *
 * @param[in] p_hashtable   Hashtable to search
 * @param[in] p_key         Key for an entry
 *
 * @return SUCCESS: Pointer to matching entry \
 * @return FAILURE: NULL
 */
void *ht_find_key(hashtable_t *p_hashtable, const char *p_key);

/**!
 * @brief Iterates through each non-null index and bucket in a hashtable and
 * performs a user-provided function.
 *
 * @param[in] p_hashtable   Hashtable to search
 * @param[in] p_user_func   User-provided function
 *
 * @return SUCCESS: Pointer to matching entry \
 * @return FAILURE: NULL
 *
 * @note Common use case would be a compare function. This would be some means
 * other than the key the entry was hashed against.
 *
 */
void *ht_compare(hashtable_t *p_hashtable, p_comp p_user_func);

/**!
 * @brief Adds an entry to a hashtable
 *
 * @param[in] p_hashtable   Hashtable to add an entry
 * @param[in] p_key         Key for an entry
 * @param[in] p_data        Data of an entry
 *
 */
bool ht_insert(hashtable_t *p_hashtable, const char *p_key, void *p_data);

/**!
 * @brief Removes an entry from a hashtable
 *
 * @param[in] p_hashtable   Hashtable from which an entry will be removed
 * @param[in] p_key         Key for an entry
 */
bool ht_delete(hashtable_t *p_hashtable, const char *p_key, p_del p_del_func);

/**!
 * @brief Prints all keys within a hashtable
 * 
 * @param[in] p_hashtable   Hashtable to print
 * 
*/
void ht_print(hashtable_t * p_hashtable);

/**!
 * @brief Populates a hashtable from a file (deserialize)
 * 
 * @param[in] p_hashtable   Hashtable to populate
 * @param[in] p_filename    Path to file
 * 
*/
bool ht_populate(hashtable_t *p_hashtable, const char *p_filename);

/**!
 * @brief Write an entire hashtable to file (serialize)
 * 
 * @param[in] p_hashtable   Hashtable to populate
 * @param[in] p_filename    Path to file
 * 
*/
bool ht_write(hashtable_t *p_hashtable, const char *p_filename);

#endif /* HASHTABLE_CHAINING_H */