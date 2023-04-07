#include "utils/dict.h"
#include "utils/alloc.h" // For `alloc`, and `dealloc`
#include "utils/failure.h" // For `FAIL`

//////////

static const char dict_var_format_chars[num_dict_var_types] = {'u', 's'};

#define FORMAT_SEARCH_CHAR '_'
#define FORMAT_SEARCH_CHAR_AS_STRING "_"

// Returns the found location, so that the format string can be built further from there
static char* make_format_string(char* const format_string, const DictVarType type) {
	char* const pos = strchr(format_string, FORMAT_SEARCH_CHAR);

	if (pos == NULL) FAIL(CreateFormatString,
		"Could not make a format string, since "
		"the search char '%c' was not found", FORMAT_SEARCH_CHAR
	);

	*pos = dict_var_format_chars[type];
	return pos;
}

//////////

// TODO: remove
/*
void print_dict(const Dict* const dict) {
	puts("{");

	const buffer_size_t num_entry_slots = dict -> num_entry_slots;
	const BitArray containment_states = dict -> containment_states;
	const DictEntry* const entries = dict -> entries;

	char format_string[] = "\t%" FORMAT_SEARCH_CHAR_AS_STRING ": %" FORMAT_SEARCH_CHAR_AS_STRING "\n";
	make_format_string(make_format_string(format_string, dict -> key_type), dict -> value_type);

	//////////

	for (buffer_size_t i = 0; i < num_entry_slots; i++) {
		if (bitarray_bit_is_set(containment_states, i)) {
			const DictEntry entry = entries[i];
			printf(format_string, entry.key, entry.value);
		}
	}

	puts("}");
}
*/

////////// Operations on keys

static buffer_size_t get_key_index(const Dict* const dict, DictVar key) {
	dict_hash_t hash;

	switch (dict -> key_type) {
		case DV_UnsignedInt:
			hash = key.unsigned_int;
			hash = ((hash >> 16u) ^ hash) * 0x45d9f3bu;
			hash = ((hash >> 16u) ^ hash) * 0x45d9f3bu;
			hash = (hash >> 16u) ^ hash;
			break;

		// http://www.cse.yorku.ca/~oz/hash.html
		case DV_String: {
			hash = 5381u;
			uint8_t c;

			while ((c = (uint8_t) *(key.string++)))
				hash = ((hash << 5) + hash) + c;
			
			break;
		}
	}

	return hash % dict -> num_entry_slots;
}

static bool keys_are_equal(const DictVar key_1, const DictVar key_2, const DictVarType type) {
	switch (type) {
		case DV_UnsignedInt: return key_1.unsigned_int == key_2.unsigned_int;
		case DV_String: return !strcmp(key_1.string, key_2.string);
	}
}

////////// Some utils, and init and deinit

static Dict init_dict_with_load_factor(const buffer_size_t expected_num_entries,
	const float load_factor, const DictVarType key_type, const DictVarType value_type) {

	// Once the dict's number of entries equals `expected_num_entries`, the load factor will equal `load_factor`.
	const buffer_size_t initial_num_entry_slots = (buffer_size_t) (expected_num_entries / load_factor);

	return (Dict) {
		key_type, value_type,
		init_bitarray(initial_num_entry_slots),
		alloc(initial_num_entry_slots, sizeof(DictEntry)),
		initial_num_entry_slots, 0
	};
}

Dict init_dict(const buffer_size_t expected_num_entries,
	const DictVarType key_type, const DictVarType value_type) {

	/* For an entry count of `expected_num_entries`, the load factor will equal `max_load_factor`,
	and then, the dict will be resized to have a load factor of `min_load_factor`. */
	return init_dict_with_load_factor(expected_num_entries, max_load_factor, key_type, value_type);
}

void deinit_dict(const Dict* const dict) {
	deinit_bitarray(dict -> containment_states);
	dealloc(dict -> entries);
}

////////// Insertion, getting, and existence checking

void insert_into_dict(Dict* const dict, const DictVar key, const DictVar value) {
	const DictVarType key_type = dict -> key_type;
	DictEntry* entries = dict -> entries;
	BitArray containment_states = dict -> containment_states;

	////////// Resizing the dict, if necessary

	const buffer_size_t
		num_entries = dict -> num_entries,
		old_num_entry_slots = dict -> num_entry_slots;

	const float load_factor = (float) num_entries / old_num_entry_slots;

	if (load_factor > max_load_factor) {
		Dict new_dict = init_dict_with_load_factor(num_entries,
			min_load_factor, key_type, dict -> value_type);

		for (buffer_size_t i = 0; i < old_num_entry_slots; i++) {
			if (bitarray_bit_is_set(containment_states, i)) {
				// This recursive call will not recurse forever, because
				// the new load factor will not exceed the max load factor.
				const DictEntry entry = entries[i];
				insert_into_dict(&new_dict, entry.key, entry.value);
			}
		}

		deinit_dict(dict); // Deallocing the old one
		*dict = new_dict; // Copying over the new one
		entries = dict -> entries; // Resetting some local vars
		containment_states = dict -> containment_states;
	}

	////////// Inserting into the dict

	buffer_size_t index = get_key_index(dict, key);

	// Loop while entries are occupied
	while (bitarray_bit_is_set(containment_states, index)) {
		// Return if the keys match (this implies that the entry is already occupied)
		if (keys_are_equal(key, entries[index].key, key_type)) {
			entries[index].value = value;
			return;
		}

		if (++index == dict -> num_entry_slots) index = 0;

		/* If the index equals the original index by here, there will be an infinite loop.
		This never happens because: Since the max load factor will always be under 1, there will
		always be some number of available entry slots in the hash table. Thus, an entry slot will 
		always be found. Once the load factor equals the max load factor, the hash table will be resized,
		which will give even more available slots. */
	}

	set_bit_in_bitarray(containment_states, index);
	entries[index] = (DictEntry) {key, value};
	dict -> num_entries++;
}

//////////

static const DictVar* get_ptr_to_value_in_dict(const Dict* const dict, const DictVar key) {
	buffer_size_t index = get_key_index(dict, key);
	const buffer_size_t orig_index = index, num_entry_slots = dict -> num_entry_slots;

	const DictVarType key_type = dict -> key_type;
	const BitArray containment_states = dict -> containment_states;
	const DictEntry* const entries = dict -> entries;

	while (true) { // Stop if entry is valid and matching key found
		if (bitarray_bit_is_set(containment_states, index) && keys_are_equal(key, entries[index].key, key_type))
			return &entries[index].value;

		if (++index == num_entry_slots) index = 0; // Wrapping around the index
		if (index == orig_index) return NULL;
	}
}

DictVar read_from_dict(const Dict* const dict, const DictVar key) {
	const DictVar* const value = get_ptr_to_value_in_dict(dict, key);

	if (value == NULL) {
		char format_string[] = "Could not find the key '% " FORMAT_SEARCH_CHAR_AS_STRING "' in the dict";
		make_format_string(format_string, dict -> key_type);
		FAIL(ReadFromDict, format_string, key);
	}

	return *value;
}

bool key_exists_in_dict(const Dict* const dict, const DictVar key) {
	return get_ptr_to_value_in_dict(dict, key) != NULL;
}
