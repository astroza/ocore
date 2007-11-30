/* Felipe Astroza 2006
 * Ocore list.h
 * Under LGPL
 */
#ifndef __OCORE_LIST_H_
#define __OCORE_LIST_H_

typedef void (*ocore_list_free_cb)(void *);

struct _ocore_list_node
{
	void *data;
	struct _ocore_list_node *next;
};
typedef struct _ocore_list_node ocore_list_node;

 struct _ocore_dlist_node
{
	ocore_list_node complement;
	struct _ocore_dlist_node *prev;
};
typedef struct _ocore_dlist_node ocore_dlist_node;

struct _ocore_list 
{
	ocore_list_node *first;
	ocore_list_node *last;
	ocore_list_node *current;
	ocore_list_free_cb free_func;

	int count;
};

struct _ocore_dlist
{
	struct _ocore_list complement;
	int insert_as_previous;
};

typedef struct _ocore_list ocore_list;
typedef struct _ocore_dlist ocore_dlist;

#define OCORE_LNODE(a) ((ocore_list_node *)(a))
#define OCORE_DLNODE(a) ((ocore_dlist_node *)(a))
#define OCORE_LIST(a) ((ocore_list *)(a))

ocore_list *ocore_list_new(void);
int ocore_list_new_node(ocore_list *list, void *data);
void *ocore_list_goto_first(ocore_list *list);
void *ocore_list_goto_last(ocore_list *list);
void *ocore_list_next(ocore_list *list);
void *ocore_list_current(ocore_list *list);
int ocore_list_count(ocore_list *list);
int ocore_list_remove(ocore_list *list);
int ocore_list_destroy(ocore_list *list);
void ocore_list_remove_node(ocore_list *list, ocore_list_node *);
ocore_list_node *ocore_list_get_current_ptr(ocore_list *list);
void ocore_list_set_current(ocore_list *list, ocore_list_node *node);
ocore_dlist *ocore_dlist_new();
int ocore_dlist_new_node(ocore_dlist *list, void *data);
void *ocore_dlist_prev(ocore_dlist *list);
int ocore_dlist_remove(ocore_dlist *list);
void ocore_dlist_remove_node(ocore_dlist *list, ocore_dlist_node *node);
void ocore_dlist_insert_as(ocore_dlist *list, int w);
void ocore_list_set_free_func(ocore_list *list, ocore_list_free_cb func);

#endif
