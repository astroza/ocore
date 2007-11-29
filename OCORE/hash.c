/* Felipe Astroza 2006
 * Ocore hash.c
 * Under GPL
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <hash.h>

static unsigned int hash_func(const char *name)
{
	const char *ptr = name;
	unsigned int h = 0;

	while(*ptr)
		h = *ptr++ + (h << 5) - h;

	return h;
}

inline static ocore_hash_node *
_ocore_hash_get_bucket(ocore_hash *handle, const char *name, int alloc, unsigned int *idx_out)
{
	unsigned int idx = hash_func(name) % handle->size;

	if(idx_out)
		*idx_out = idx;

	if(alloc && !handle->table[idx]) {
		handle->table[idx] = malloc(sizeof(ocore_hash_node));
		if(!handle->table[idx]) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		memset(handle->table[idx], 0, sizeof(ocore_hash_node));
	}

        return handle->table[idx];
}

/* ocore_hash_init: Permite asignar la memoria requerida por la tabla
 * y ajustar algunas opciones (size, free_value).
 */
void ocore_hash_init(ocore_hash *handle, unsigned int size, ocore_hash_free_func func)
{
	unsigned int _size = size? size : DEFAULT_HASHSIZE;

	if(handle) {
		handle->table = calloc(_size, sizeof(ocore_hash_node *));
		if(!handle->table) {
			perror("calloc");
			exit(EXIT_FAILURE);
		}
		handle->size = _size;
		handle->free_func = func;
	}
}

/* ocore_hash_add(): Agrega un nuevo nodo a la hash table
 */
ocore_hash_node *
ocore_hash_add(ocore_hash *handle, const char *name, void *value, int dup)
{
	ocore_hash_node *node;

	if(!handle || !name) 
		return NULL;

	for(node = _ocore_hash_get_bucket(handle, name, 1, NULL); node; node = node->next) {
		if(!node->name)
			break;

		if(strcasecmp(name, node->name) == 0)
			return NULL;

		if(!node->next) {
			node->next = malloc(sizeof(ocore_hash_node));
			if(!node->next) {
				perror("malloc");
				exit(EXIT_FAILURE);
			}
			node = node->next;
			break;
		}
	}

	node->name = dup? strdup(name) : (char *)name;
	node->name_dup = dup;
	node->value = value;
	node->next = NULL;

	return node;
}

static ocore_hash_node *
_ocore_hash_get_node(ocore_hash *handle, const char *name)
{
	ocore_hash_node *node;

	for(node = _ocore_hash_get_bucket(handle, name, 0, NULL); node; node = node->next)
		if(strcasecmp(name, node->name) == 0)
			return node;

	return NULL;
}

/* ocore_hash_get_node(): Retorna nodo buscado.
 */
ocore_hash_node *
ocore_hash_get_node(ocore_hash *handle, const char *name)
{
	if(!handle || !name)
		return NULL;

	return _ocore_hash_get_node(handle, name);
}

/* ocore_hash_get_value(): Retorna el valor del nodo buscado
 */
void *ocore_hash_get_value(ocore_hash *handle, const char *name)
{
	ocore_hash_node *node;

	if(!handle || !name)
		return NULL;

	node = _ocore_hash_get_node(handle, name);
	return node? node->value : NULL;
}

static ocore_hash_node *
_ocore_hash_extract(ocore_hash *handle, const char *name)
{
	ocore_hash_node *node, *prev = NULL;
	unsigned int idx;
	/* Rutina de busqueda y extraccion de nodo */
	for(node = _ocore_hash_get_bucket(handle, name, 0, &idx); node; node = node->next) {
		if(strcasecmp(name, node->name) == 0)
			break;

		prev = node;
	}

	if(!node)
		return NULL;
	if(prev)
		prev->next = node->next;

	if(handle->table[idx] == node)
		handle->table[idx] = node->next;

	return node;
}

/* ocore_hash_remove(): Elimina nodo de la hash table.
 */
int ocore_hash_remove(ocore_hash *handle, const char *name)
{
	ocore_hash_node *node;

	if(!handle || !name)
		return 0;

	node = _ocore_hash_extract(handle, name);
	if(!node)
		return 0;

	if(handle->free_func && node->value)
		handle->free_func(node->value);

	if(node->name_dup)
		free(node->name);

	free(node);
	return 1;
}

/* ocore_hash_change_key(): Reubica el nodo con su nueva llave.
 */
ocore_hash_node *
ocore_hash_change_key(ocore_hash *handle, const char *old_name, const char *new_name)
{
	ocore_hash_node *node, *aux;
	unsigned int idx;

	if(!handle || !old_name || !new_name)
		return NULL;

	for(node = _ocore_hash_get_bucket(handle, new_name, 0, &idx); node; node = node->next) {
		if(strcasecmp(new_name, node->name) == 0)
			return NULL;

		if(!node->next)
			break;
	}

	aux = _ocore_hash_extract(handle, old_name);
	if(!aux)
		return NULL;

	if(node)
		node->next = aux;
	else 
		handle->table[idx] = aux;

	aux->next = NULL;

	/* Finalmente el nuevo nombre */
	if(aux->name_dup) {
		free(aux->name);
		aux->name = strdup(new_name);
	} else
		aux->name = (char *)new_name;

	return aux;
}

/* ocore_hash_list(): Permite recorrer la tabla con sus nodos.
 * ocore_hash_position es una estructura que contiene la ubicacion
 * actual en la tabla.
 */
ocore_hash_node *ocore_hash_list(ocore_hash *handle, ocore_hash_position *pst)
{
	if(!handle || !pst || pst->idx >= handle->size)
		return NULL;

	if(pst->node)
		pst->node = pst->node->next;
	else
		pst->node = handle->table[pst->idx];

	if(!pst->node) {
		while(++pst->idx < handle->size) {
			pst->node = handle->table[pst->idx];
			if(pst->node)
				break;
		}
	}

	return pst->node;
}

void _ocore_hash_destroy_all(ocore_hash *handle)
{
	ocore_hash_node *node, *next;
	unsigned int idx = 0;

	/* Manera rapida de acabar con todos los nodos hohoho.. */
	while(idx < handle->size) {
		node = handle->table[idx];
		while(node != NULL) {
			next = node->next;
			if(handle->free_func && node->value)
				handle->free_func(node->value);

			if(node->name_dup)
				free(node->name);

			free(node);
			node = next;
		}

		handle->table[idx++] = NULL;
	}
}

/* ocore_hash_destroy_all(): Libera todas las entradas de la tabla.
 */
void ocore_hash_destroy_all(ocore_hash *handle)
{
	if(handle && handle->table)
		_ocore_hash_destroy_all(handle);
}

/* ocore_hash_free_table(): Libera todas las entradas y la tabla.
*/
void ocore_hash_free_table(ocore_hash *handle)
{
	if(handle && handle->table) {
		_ocore_hash_destroy_all(handle);
		free(handle->table);
	}
}
