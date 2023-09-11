/*===--------------------------------------------------------------------------------------------===
 * map.c
 *
 * Created by Amy Parent <amy@amyparent.com>
 * Copyright (c) 2023 Amy Parent. All rights reserved
 *
 * Licensed under the MIT License
 *===--------------------------------------------------------------------------------------------===
*/
#include "obj_impl.h"
#include "../value_impl.h"
#include <string.h>

#define MAP_MAX_LOAD            (0.75)

warp_map_t *warp_map_new(warp_vm_t *vm) {
    warp_map_t *map = ALLOCATE_OBJ(vm, warp_map_t, WARP_OBJ_MAP);
    
    map->count = 0;
    map->load = 0;
    map->capacity = 0;
    map->entries = NULL;
    
    return map;
}


void warp_map_free(warp_vm_t *vm, warp_map_t *map) {
    FREE_ARRAY(vm, map->entries, entry_t, map->capacity);
    FREE(vm, map, warp_map_t);
}

static inline bool is_valid_key_type(warp_value_t key) {
    return WARP_IS_NUM(key) || WARP_IS_BOOL(key) || WARP_IS_STR(key);
}

static inline uint32_t hash_bits(uint64_t hash) {
    // From Wren's hashBits() which in turn cites:
    // From v8's ComputeLongHash() which in turn cites:
    // Thomas Wang, Integer Hash Functions.
    // http://www.concentric.net/~Ttwang/tech/inthash.htm
    hash = ~hash + (hash << 18);  // hash = (hash << 18) - hash - 1;
    hash = hash ^ (hash >> 31);
    hash = hash * 21;  // hash = (hash + (hash << 2)) + (hash << 4);
    hash = hash ^ (hash >> 11);
    hash = hash + (hash << 6);
    hash = hash ^ (hash >> 22);
    return (uint32_t)(hash & 0x3fffffff);
}

static inline uint32_t hash_num(double num) {
    typedef union {
        double      f64;
        uint64_t    u64;
    } numbits_t;
    
    numbits_t bits = {.f64 = num};
    return hash_bits(bits.u64);
}

static inline uint32_t hash(warp_value_t key) {
    if(WARP_IS_NUM(key)) {
        return hash_num(WARP_AS_NUM(key));
    } else if(WARP_IS_BOOL(key)) {
        return hash_bits(WARP_AS_BOOL(key));
    } else if(WARP_IS_STR(key)) {
        return WARP_AS_STR(key)->hash;
    }
    UNREACHABLE();
    return 0;
}

static entry_t *find_entry(entry_t *entries, warp_uint_t capacity, warp_value_t key) {
    warp_uint_t idx = hash(key) % capacity;
    entry_t *tombstone = NULL;
    for(;;) {
        entry_t *entry = &entries[idx];
        
        if(WARP_IS_NIL(entry->key)) {
            if(WARP_IS_NIL(entry->value)) {
                return tombstone ? tombstone : entry;
            } else {
                if(!tombstone) {
                    tombstone = entry;
                }
            }
        } else if(value_equals(key, entry->key)) {
            return entry;
        }
        idx = (idx + 1) % capacity;
    }
}

static void map_adjust_cap(warp_vm_t *vm, warp_map_t *map, warp_uint_t capacity) {
    entry_t *entries = ALLOCATE_ARRAY(vm, entry_t, capacity);
    for(warp_uint_t i = 0; i < capacity; ++i) {
        entries[i].key = WARP_NIL_VAL;
        entries[i].value = WARP_NIL_VAL;
    }
    
    for(warp_uint_t i = 0; i < map->capacity; ++i) {
        entry_t *entry = &map->entries[i];
        if(WARP_IS_NIL(entry->key)) continue;
        
        entry_t *dest = find_entry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
    }
    
    FREE_ARRAY(vm, map->entries, entry_t, map->capacity);
    map->load = map->count;
    map->capacity = capacity;
    map->entries = entries;
}

bool warp_map_set(warp_vm_t *vm, warp_map_t *map, warp_value_t key, warp_value_t val) {
    CHECK(is_valid_key_type(key));
    
    if(map->load + 1 > map->capacity * MAP_MAX_LOAD) {
        warp_uint_t new_cap = GROW_CAPACITY(map->capacity);
        map_adjust_cap(vm, map, new_cap);
    }
    
    entry_t *entry = find_entry(map->entries, map->capacity, key);
    CHECK(entry != NULL);
    
    bool existing = true;
    if(WARP_IS_NIL(entry->key)) {
        map->count += 1;
        map->load += 1;
        existing = false;
    }
    entry->key = key;
    entry->value = val;
    return existing;
}

bool warp_map_delete(warp_map_t *map, warp_value_t key, warp_value_t *out) {
    CHECK(is_valid_key_type(key));
    if(map->count == 0) return false;
    
    entry_t *entry = find_entry(map->entries, map->capacity, key);
    if(WARP_IS_NIL(entry->key)) return false;
    
    if(out) *out = entry->value;
    entry->key = WARP_NIL_VAL;
    entry->value = WARP_BOOL_VAL(true);
    map->count -= 1;
    return true;
}

warp_str_t *warp_map_find_str(warp_map_t *map, const char *str, warp_uint_t length, uint32_t hash) {
    if(map->count == 0) return NULL;
    
    warp_uint_t idx = hash % map->capacity;
    for(;;) {
        entry_t *entry = &map->entries[idx];
        if(WARP_IS_NIL(entry->key)) {
            if(WARP_IS_NIL(entry->value)) {
                return NULL;
            }
        } else if(WARP_IS_STR(entry->key)) {
            warp_str_t *key = WARP_AS_STR(entry->key);
            if(key->hash == hash && key->length == length && memcmp(str, key->data, length) == 0) {
                return WARP_AS_STR(entry->key);
            }
        }
        idx = (idx + 1) % map->capacity;
    }
}

bool warp_map_get(warp_map_t *map, warp_value_t key, warp_value_t *out) {
    CHECK(is_valid_key_type(key));
    if(map->count == 0) return false;
    
    entry_t *entry = find_entry(map->entries, map->capacity, key);
    if(WARP_IS_NIL(entry->key)) return false;
    *out = entry->value;
    return true;
}
