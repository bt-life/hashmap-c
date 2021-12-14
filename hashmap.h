#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#ifdef __cplusplus
extern "C" {
#endif

// void* to int
#define v2i(v) ((int)(uintptr_t)(v))
// int* to void*
#define i2v(i) ((void*)(uintptr_t)(i))

// free key and value memory by hashmap_t
#define HMAP_OPT_FREE_KEY   (1 << 0)
#define HMAP_OPT_FREE_VALUE (1 << 1)

typedef struct hashmap *hashmap_t;
typedef unsigned (*hash_func_t)(void *key);
typedef int (*cmp_func_t)(void *key1, void *key2);

unsigned hashmap_default_str_hash_func(void *key);
int hashmap_default_str_cmp_func(void *key1, void *key2);
unsigned hashmap_default_int_hash_func(void *key);
int hashmap_default_int_cmp_func(void *key1, void *key2);

hashmap_t hashmap_new(hash_func_t hash_func, cmp_func_t cmp_func, 
    int size /*= 137*/, unsigned opts /*= 0*/);
void hashmap_free(hashmap_t hmap);
int hashmap_set(hashmap_t hmap, void *key, void *value);
void* hashmap_get(hashmap_t hmap, void *key);
int hashmap_remove(hashmap_t hmap, void *key);
int hashmap_clear(hashmap_t hmap);
unsigned hashmap_hashcode(hashmap_t hmap, void *key);


#ifdef __cplusplus
}
#endif

#endif
