#ifndef DICT_H
#define DICT_H

#include "utils/typedefs.h" // For `buffer_size_t`
#include "utils/bitarray.h" // For `BitArray`

/*
Expected places where this will be used:
- Clip path comparison in audio.c
- Albedo texture path comparison for materials in level_config.c
- Subvar name cmp in uniform_buffer.c

- So at the moment, the only expected key will be strings (but generalize later as needed)
- For generalizing, pass in an enum that defines the key type - standard key types will go to
	builtin optimized hashing functions, and other ones will go to custom hashing functions
	(perhaps those other ones will call the optimized hashing functions, and then combine the results
	in some way)

TODO:
- Support a better probing method
- Support resizing in-place
- Put the min and max load factors in the `constants` struct
- Find a way to assert in a relatively static manner that infinite recursion will not happen for `insert_into_dict`,
	and that the probing will not loop forever either. For the recursion, perhaps if there's more than 1 infinite loop,
	alert that there's a problem? Or only recur for 1 level?

- Make some macro that initializes an array of structs into a hash table,
	passed in without a separate initialization of that struct (a `sizeof`
	size inference can be used for that)

- Support hashing for u8, u16, u32, u64, i8, i16, i32, i64, string, and custom .
	Pass in an enum for the key type; builtins will have predefined hashing functions,
	and other will have a hashing function passed in.

- For the different key types, perhaps use a union

- Share logic between the getting and setting of keys
- Perhaps compare keys only by their hash
- And perhaps allocate more or less, depending on the sizes of the keys or values
- With that idea, support any type for dicts, not just the thin existing selection of types
- Still provide some premade hash functions though
*/

static const float
	min_load_factor = 0.4f,
	max_load_factor = 0.75f;

typedef uint64_t dict_hash_t;

//////////

enum {num_dict_var_types = 2};

typedef union {
	uint32_t unsigned_int;
	const char* string;
} DictVar;

typedef enum {
	DV_UnsignedInt,
	DV_String
} DictVarType;

//////////

typedef struct {
	DictVar key, value;
} DictEntry;

typedef struct {
	// Note: I can remove the value type if I remove printing
	DictVarType key_type, value_type;
	BitArray containment_states;
	DictEntry* entries;
	buffer_size_t num_entry_slots, num_entries;
} Dict;

// Excluded: make_format_string, fail_for_invalid_key_type, get_key_index, keys_are_equal, init_dict_with_load_factor, get_ptr_to_value_in_dict

//////////

#define DICT_FOR_EACH(dict, item_name, ...) do {\
	const BitArray containment_states = (dict) -> containment_states;\
	DictEntry* const entries = (dict) -> entries;\
	const buffer_size_t num_entry_slots = (dict) -> num_entry_slots;\
	for (buffer_size_t i = 0; i < num_entry_slots; i++) {\
		if (!bitarray_bit_is_set(containment_states, i)) continue;\
		DictEntry* const item_name = entries + i;\
		__VA_ARGS__\
	}\
} while (false)

// TODO: is the value type actually needed?
Dict init_dict(const buffer_size_t expected_num_entries,
	const DictVarType key_type, const DictVarType value_type);

void deinit_dict(const Dict* const dict);
void clear_dict(Dict* const dict);

//////////

#define typed_insert_into_dict(dict, literal_key, literal_value, key_field, value_field)\
	insert_into_dict(dict, (DictVar) {.key_field = literal_key}, (DictVar) {.value_field = literal_value})

// TODO: how to typecheck this? Perhaps switch on the dict types, and strcmp the key and value fields
#define typed_read_from_dict(dict, literal_key, key_field, value_field)\
	read_from_dict(dict, (DictVar) {.key_field = literal_key}).value_field

#define typed_key_exists_in_dict(dict, literal_key, key_field)\
	key_exists_in_dict(dict, (DictVar) {.key_field = literal_key})

void insert_into_dict(Dict* const dict, const DictVar key, const DictVar value);
DictVar read_from_dict(const Dict* const dict, const DictVar key);
bool key_exists_in_dict(const Dict* const dict, const DictVar key);

#endif
