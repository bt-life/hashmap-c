#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <math.h>
#include "hashmap.h"


struct pair {
    struct pair *next;
    unsigned hash_code;
    void *key;
    void *value;
};

struct hashmap {
    struct pair **buckets;
    int num_buckets;
    hash_func_t hash_func;
    cmp_func_t cmp_func;
    unsigned int options;
};

unsigned hashmap_default_str_hash_func(void *key) {
    unsigned hash = 0;
    size_t i, exp;
    char *k = (char*)key;
    for (i = 0, exp = 1; *k; i++, k++) {
        if (0 == i || i == exp) {
            hash = 31 * hash + *k;
            if (i != 0) {
                exp <<= 1;
            }
        }
    }
    return hash;
}
int hashmap_default_str_cmp_func(void *key1, void *key2) {
    return strcmp((char*)key1, (char*)key2);
}

unsigned hashmap_default_int_hash_func(void *key) {
    int hash = v2i(key);
    return hash;
}
int hashmap_default_int_cmp_func(void *key1, void *key2) {
    return v2i(key1) - v2i(key2);
}

static int is_prime(int n)
{
    int i, k;
    k = (int)sqrt((double)n);
    for (i = 2; i <= k; i++) {
        if (n % i == 0) {
            break;
        }
    }
    return i > k;
}

static int get_max_prime(int n)
{
    int i;
    for (i = n; i > 0; i--) {
        if (is_prime(i)) {
            return i;
        }
    }
    return -1;
}

hashmap_t hashmap_new(hash_func_t hash_func, cmp_func_t cmp_func, int size, unsigned opts)
{
    int i;

    if (!hash_func || !cmp_func) {
        return 0;
    }
    if ((hash_func == hashmap_default_str_hash_func && cmp_func == hashmap_default_int_cmp_func) ||
        (hash_func == hashmap_default_int_hash_func && cmp_func == hashmap_default_str_cmp_func)) {
        // hash_func with cmp_func mismatched
        return 0;
    }

    if (size <= 2) {
        size = 137;
    }
    size = get_max_prime(size);  // keep size is prime number to reduce probability of conflict 
    hashmap_t hmap = (hashmap_t)malloc(sizeof(*hmap) + size * sizeof(*hmap->buckets));
    if (!hmap) {
        return hmap;
    }

    hmap->buckets = (struct pair**)(hmap + 1);
    hmap->num_buckets = size;
    hmap->hash_func = hash_func;
    hmap->cmp_func = cmp_func;
    hmap->options = opts;

    for (i = 0; i < size; i++) {
        hmap->buckets[i] = 0;
    }

    return hmap;
}

void hashmap_free(hashmap_t hmap)
{
    if (hmap) {
        hashmap_clear(hmap);
        free((void*)hmap);
    }
}

static void free_pair(hashmap_t hmap, struct pair *pair)
{
    if (!hmap || !pair) {
        return;
    }
    if (hmap->options & HMAP_OPT_FREE_KEY) {
        free((void*)pair->key);
    }
    if (hmap->options & HMAP_OPT_FREE_VALUE) {
        free((void*)pair->value);
    }
    free((void*)pair);
}

static struct pair* find_pair(hashmap_t hmap, void *key, struct pair **prev, unsigned *hash_code)
{
    unsigned _hash_code = hashmap_hashcode(hmap, key) % hmap->num_buckets;
    struct pair *pair = hmap->buckets[_hash_code];

    if (prev) {
        *prev = 0;
    }
    while (pair) {
        if (hmap->cmp_func(pair->key, key) == 0) {
            break;
        }
        if (prev) {
            *prev = pair;
        }
        pair = pair->next;
    }
    if (hash_code) {
        *hash_code = _hash_code;
    }

    return pair;
}

int hashmap_set(hashmap_t hmap, void *key, void *value)
{
    unsigned hash_code;
    struct pair *prev = 0, *pair;

    if (!hmap) {
        return -EINVAL;
    }

    pair = find_pair(hmap, key, &prev, &hash_code);
    if (pair) {
        pair->value = value;
    } else {
        pair = (struct pair *)malloc(sizeof(*pair));
        if (!pair) {
            return -ENOMEM;
        }

        pair->hash_code = hash_code;
        pair->key = key;
        pair->value = value;
        pair->next = 0;

        if (prev) {
            prev->next = pair;
        } else {
            hmap->buckets[hash_code] = pair;
        }
    }

    return 0;
}

void* hashmap_get(hashmap_t hmap, void *key)
{
    struct pair *pair = hmap ? find_pair(hmap, key, 0, 0) : 0;
    return pair ? pair->value : 0;
}

int hashmap_remove(hashmap_t hmap, void *key)
{
    unsigned hash_code;
    struct pair *prev = 0, *pair;

    if (!hmap) {
        return -EINVAL;
    }

    pair = find_pair(hmap, key, &prev, &hash_code);
    if (!pair) {
        return -1;
    }

    if (prev) {
        prev->next = pair->next;
    } else {
        hmap->buckets[hash_code] = pair->next;
    }
    hmap->buckets[hash_code] = 0;

    free_pair(hmap, pair);

    return 0;
}

int hashmap_clear(hashmap_t hmap)
{
    int i;

    if (!hmap) {
        return -EINVAL;
    }

    for (i = 0; i < hmap->num_buckets; i++) {
        struct pair *pair = hmap->buckets[i], *next = 0;
        for (; pair; pair = next) {
            next = pair->next;
            free_pair(hmap, pair);
        }
        hmap->buckets[i] = 0;
    }

    return 0;
}

unsigned hashmap_hashcode(hashmap_t hmap, void *key)
{
    unsigned hash_code = 0;

    if (!hmap) {
        return hash_code;
    }

    hash_code = hmap->hash_func(key);
    if (hash_code < 0) {
        hash_code = -hash_code;
    }
    return hash_code;
}


//#if 1
#ifdef __UNIT_TEST__
#include <assert.h>
int main(int argc, char *argv[]) {
    int i;
    int n = argc > 1 ? atoi(argv[1]) : 100;
    hashmap_t hmap;
    
    printf("Testing for integer key\n");
    hmap = hashmap_new(hashmap_default_int_hash_func, hashmap_default_int_cmp_func, 7, 0);

    hashmap_set(hmap, i2v(1), i2v(1));
    printf("initial value: %d\n", v2i(hashmap_get(hmap, i2v(1))));
    hashmap_set(hmap, i2v(1), i2v(2));
    printf("value after changing: %d\n", v2i(hashmap_get(hmap, i2v(1))));
    printf("remove key(%d) from hmap: %s\n", 1, hashmap_remove(hmap, i2v(1)) == 0 ? "removed" : "not removed");
    printf("value after remove: %d\n", v2i(hashmap_get(hmap, i2v(1))));

    for (i = 1; i < n; i++) {
        int v = i * i;
        hashmap_set(hmap, i2v(i), i2v(v));
    }
    printf("setted %d values\n", n);
    for (i = 1; i < n; i++) {
        void *v = hashmap_get(hmap, i2v(i));
        if (v) {
            int val = v2i(v);
            assert(val == i * i);
            printf("%d:%d\t", i, val);
        }
    }
    printf("\ngetted %d values\n\n", n);

    hashmap_clear(hmap);
    hashmap_free(hmap);

    printf("Testing for string key\n");
    hmap = hashmap_new(hashmap_default_str_hash_func, hashmap_default_str_cmp_func,
        137, HMAP_OPT_FREE_KEY | HMAP_OPT_FREE_VALUE);

    for (i = 1; i < n; i++) {
        char *key, *val;
        key = (char*)malloc(13);
        val = (char*)malloc(13);
        sprintf(key, "str%d", i);
        sprintf(val, "str%d", i * i);
        hashmap_set(hmap, key, val);
    }
    printf("setted %d values\n", n);
    for (i = 1; i < n; i++) {
        char key[13], val[13];
        sprintf(key, "str%d", i);
        sprintf(val, "str%d", i * i);
        void *v = hashmap_get(hmap, key);
        if (v) {
            char *_v = (char*)v;
            assert(strcmp(val, _v) == 0);
            printf("%s:%s\t", key, val);
        }
    }
    printf("\ngetted %d values\n", n);

    hashmap_free(hmap);

    printf("\ndone!\n");
}
#endif
