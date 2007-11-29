/* Felipe Astroza 2006
 * Ocore hash.h
 * Under LGPL
 */
#ifndef __OCORE_HASH_H_
#define __OCORE_HASH_H_

typedef struct _ocore_hash_node {
	char *name;
	void *value;
	int name_dup;
	struct _ocore_hash_node *next;
} ocore_hash_node;

typedef void (*ocore_hash_free_func)(void *);

typedef struct {
	ocore_hash_node **table;
	unsigned int size;
	ocore_hash_free_func free_func;
} ocore_hash;

typedef struct {
	unsigned int idx;
	ocore_hash_node *node;
} ocore_hash_position;

#define DEFAULT_HASHSIZE 16

void ocore_hash_init(ocore_hash *handle, unsigned int size, ocore_hash_free_func func);

ocore_hash_node *ocore_hash_add(ocore_hash *handle, const char *name, void *value, int dup);
int ocore_hash_remove(ocore_hash *handle, const char *name);
ocore_hash_node *ocore_hash_change_key(ocore_hash *handle, const char *old_name, const char *new_name);

ocore_hash_node *ocore_hash_list(ocore_hash *handle, ocore_hash_position *pst);

ocore_hash_node *ocore_hash_get_node(ocore_hash *handle, const char *name);
void *ocore_hash_get_value(ocore_hash *handle, const char *name);

void ocore_hash_destroy_all(ocore_hash *handle);
void ocore_hash_free_table(ocore_hash *handle);

#endif
