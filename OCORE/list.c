/* Felipe Astroza - OCORE
 * Under GPL
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <list.h>

/* Defino primero la funcion static con prefijo _, esta no contiene verificadores, asi se hace mas eficiente el uso por parte
 * de rutinas del mismo objeto
 */
ocore_list *ocore_list_new(void)
{
	ocore_list *new;

	new = calloc(1, sizeof(ocore_list));
	if(!new) {
		perror("calloc");
		exit(-1);
	}

	return new;
}

/* _ocore_list_insert:
 * Inserta el nuevo nodo a continuacion de current
 */
static void _ocore_list_insert(ocore_list *list, ocore_list_node *node)
{
	if(!list->last) {
		list->last = (list->first = node);
		list->current = list->last;
	} else
	if(list->current) {
		if(list->current->next)
			node->next = list->current->next;

		list->current->next = node;

		if(list->current == list->last)
			list->last = node;

		list->current = node;
	}

	list->count++;
}

/* ocore_list_new_node:
 * Reserva la memoria para el nuevo nodo, copia el puntero data, al miembro data de nodo
 * y lo inserta a la lista
 */
int ocore_list_new_node(ocore_list *list, void *data)
{
	ocore_list_node *node;
	if(!list || !data)
		return 0;

	node = calloc(1, sizeof(ocore_list_node));
	if(!node) {
		perror("calloc");
		exit(-1);
	}

	node->data = data;
	_ocore_list_insert(list, node);

	return 1;
}

static void *_ocore_list_goto_first(ocore_list *list)
{
	list->current = list->first;
	return list->current? list->current->data : NULL;
}

/* ocore_list_goto_first:
 * Cambia current al principio de la lista
 */
void *ocore_list_goto_first(ocore_list *list)
{
	if(!list)
		return NULL;

	return _ocore_list_goto_first(list);
}

/* ocore_list_goto_last:
 * Cambia current al final de la lista
 */
void *ocore_list_goto_last(ocore_list *list)
{
	if(!list)
		return NULL;

	if(list->count) {
		list->current = list->last;
		return list->current->data;
	} else
		return NULL;
}

static void *_ocore_list_next(ocore_list *list)
{
	if(list->current->next) {
		list->current = list->current->next;
		return list->current->data;
	} else
		return NULL;
}

/* ocore_list_next:
 * avanza current al siguiente
 */
void *ocore_list_next(ocore_list *list)
{
	if(!list || !list->count || !list->current)
		return NULL;

	return _ocore_list_next(list);
}

/* ocore_list_current: 
 * Retorna el dato de current
 */
void *ocore_list_current(ocore_list *list)
{
	if(!list)
		return NULL;

	if(list->current)
		return list->current->data;

	return NULL;
}

/* ocore_list_count:
 * Retorna el numero de nodos
 */
int ocore_list_count(ocore_list *list)
{
	if(!list)
		return 0;

	return list->count;
}

static ocore_list_node *
_ocore_list_remove(ocore_list *list)
{
	ocore_list_node *old;

	old = list->current;

	if(old == list->first)
		list->first = old->next;
	if(old == list->last)
		list->last = old->next;

	list->current = old->next;

	list->count--;

	if(list->free_func)
		list->free_func(old->data);

	return old;
}

/* ocore_list_remove:
 * Elimina el nodo list->current
 */
int ocore_list_remove(ocore_list *list)
{
	if(!list || !list->current)
		return 0;

	free(_ocore_list_remove(list));
	return 1;
}

/* ocore_list_remove_all: Elimina la lista y sus nodos.
 */
int ocore_list_destroy(ocore_list *list)
{
	if(!list)
		return 0;

	_ocore_list_goto_first(list);
	while(list->current)
		free(_ocore_list_remove(list));

	free(list);
	return 1;
}

/* ocore_list_remove_node(): Elimina un nodo especifico
 * Nota: El comportamiento es impredecible si <opq> no pertenece a la lista.
 */
void ocore_list_remove_node(ocore_list *list, ocore_list_node *node)
{
	assert(list != NULL);
	assert(node != NULL);

	list->current = node;
	free(_ocore_list_remove(list));
}

/* ocore_list_get_current_ptr(): Consigue la direccion del nodo actual.
*/
inline ocore_list_node *ocore_list_get_current_ptr(ocore_list *list)
{
	assert(list != NULL);

	return list->current;
}

inline void ocore_list_set_current(ocore_list *list, ocore_list_node *node)
{
	assert(list != NULL);

	list->current = (ocore_list_node *)node;
}

/* ocore_dlist_new:
 * Retorna puntero de lista doble nueva
 */
ocore_dlist *ocore_dlist_new()
{
	ocore_dlist *list;

	list = calloc(1, sizeof(ocore_dlist));
	if(!list) {
		perror("calloc");
		exit(-1);
	}

	return list;
}

static void _ocore_dlist_insert(ocore_dlist *list, ocore_dlist_node *node)
{
	ocore_dlist_node *aux = OCORE_DLNODE(OCORE_LIST(list)->current);

	/* Inserta el nodo como lista normal, luego copia el current
	 * anterior en el prev del nuevo nodo. Luego si el nuevo nodo tiene un next, mueve su direccion
	 * a next->prev
	 */
	_ocore_list_insert(OCORE_LIST(list), OCORE_LNODE(node));
	node->prev = aux;

	if( (aux = OCORE_DLNODE(OCORE_LNODE(node)->next)))
		aux->prev = node;

	/* Una manera facil de implementar esta caracteristica */
	if(list->insert_as_previous && node->prev) { /* inserta como anterior a current */
		void *data;
		ocore_list_node *prev;

		data = OCORE_LNODE(node)->data;
		prev = OCORE_LNODE(node->prev);
		OCORE_LNODE(node)->data = prev->data;
		prev->data = data;
		ocore_dlist_prev(list);
	}
}

/* ocore_dlist_new_node:
 * Agrega nodo a ¿continuacion? de current
 */
int ocore_dlist_new_node(ocore_dlist *list, void *data)
{
	ocore_dlist_node *node;

	if(!list || !data)
		return 0;

	node = calloc(1, sizeof(ocore_dlist_node));
	if(!node) {
		perror("calloc");
		exit(-1);
	}

	OCORE_LNODE(node)->data = data;
	_ocore_dlist_insert(list, node);

	return 1;
}

static void *_ocore_dlist_prev(ocore_dlist *list)
{
	ocore_dlist_node *node = OCORE_DLNODE(OCORE_LIST(list)->current);

	if(OCORE_LNODE(node->prev)) {
		OCORE_LIST(list)->current = OCORE_LNODE(node->prev);
		return OCORE_LIST(list)->current->data;
	}

	return NULL;
}

/* ocore_dlist_prev:
 * Salta al previo de current
 */
void *ocore_dlist_prev(ocore_dlist *list)
{
	if(!list || !OCORE_LIST(list)->current)
		return NULL;

	return _ocore_dlist_prev(list);
}

static ocore_dlist_node *
_ocore_dlist_remove(ocore_dlist *list)
{
	ocore_dlist_node *old;

	old = (ocore_dlist_node *)OCORE_LIST(list)->current;

	/* Primero cambia los punteros prev de su siguiente, y next de su antecesor, para
	 * desaparecer de la lista. La direccion de current por defecto siempre es el anterior
	 * al eliminado, pero si no existe, deja el siguiente.
	 */
	if(OCORE_LNODE(old)->next)
		OCORE_DLNODE(OCORE_LNODE(old)->next)->prev = old->prev;
	if(old->prev) {
		OCORE_LNODE(old->prev)->next = OCORE_LNODE(old)->next;
		OCORE_LIST(list)->current = (ocore_list_node *)old->prev;
	} else
		OCORE_LIST(list)->current = OCORE_LNODE(old)->next;


	if(old == OCORE_DLNODE(OCORE_LIST(list)->first))
		OCORE_LIST(list)->first = OCORE_LNODE(old)->next;

	if(old == OCORE_DLNODE(OCORE_LIST(list)->last))
		OCORE_LIST(list)->last = OCORE_LNODE(old->prev);

	OCORE_LIST(list)->count--;
	if(OCORE_LIST(list)->free_func)
		OCORE_LIST(list)->free_func(OCORE_LNODE(old)->data);

	return old;
}

int ocore_dlist_remove(ocore_dlist *list)
{
	assert(list);

	if(!OCORE_LIST(list)->current)
		return 0;

	/* Que persona mas segura de su codigo no? */
	free(_ocore_dlist_remove(list));
	return 1;
}

/* ocore_dlist_remove_node(): Vease ocore_list_remove_node(). 
 */
void ocore_dlist_remove_node(ocore_dlist *list, ocore_dlist_node *node)
{
	assert(list != NULL);
	assert(node != NULL);

	OCORE_LIST(list)->current = (ocore_list_node *)node;

	free(_ocore_dlist_remove(list));
}

/* ocore_dlist_insert_as: Permite indicar hacia que lado (next or prev) de current
 * insertar el nuevo nodo
 */
void ocore_dlist_insert_as(ocore_dlist *list, int w)
{
	/* >= 1: previous
	 *  = 0: next (default)
	 */
	if(list)
		list->insert_as_previous = w;
}
/* ocore_dlist_move: Translada el nodo src->current a dst.
 */
int ocore_dlist_move(ocore_dlist *src, ocore_dlist *dst)
{
	ocore_dlist_node *node;

	assert(src != NULL);
	assert(dst != NULL);

	if(!OCORE_LIST(src)->current)
		return 0;

	node = _ocore_dlist_remove(src);
	_ocore_dlist_insert(dst, node);

	return 1;
}

/* ocore_list_set_free_func: Ajusta free_func
 */
void ocore_list_set_free_func(ocore_list *list, ocore_list_free_cb func)
{
	if(list)
		list->free_func = func;
}
