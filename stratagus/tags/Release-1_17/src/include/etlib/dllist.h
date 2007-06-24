#ifndef ETLIB_DLLIST_H
#define ETLIB_DLLIST_H
/**
**	@author	Copyright by Edgar Toernig.
**	@name	dllist.h		-	Double linked lists.
**
*	$Id$
*/

struct dl_node
{
    struct dl_node *next;
    struct dl_node *prev;
};

struct dl_head
{
    struct dl_node *first;
    struct dl_node *null;
    struct dl_node *last;
};

static inline struct dl_head *
dl_init(struct dl_head *h)
{
    h->first = (struct dl_node *)&h->null;
    h->null = 0;
    h->last = (struct dl_node *)&h->first;
    return h;
}

static inline struct dl_node *
dl_remove(struct dl_node *n)
{
    n->prev->next = n->next;
    n->next->prev = n->prev;
    return n;
}

static inline struct dl_node *
dl_insert_after(struct dl_node *p, struct dl_node *n)
{
    n->next = p->next;
    n->prev = p;
    p->next = n;
    n->next->prev = n;
    return n;
}

	/* A constructor for static list heads. */
#define DL_LIST(id)	struct dl_head id[1] = {{		\
			    (struct dl_node *)&id->null,	\
			    0,					\
			    (struct dl_node *)&id->first	\
			}}

#define dl_empty(h)             ((h)->first->next == 0)
#define dl_insert_before(p, n)  dl_insert_after((p)->prev, (n))
#define dl_insert_first(h, n)   (dl_insert_before((h)->first, (n)))
#define dl_insert_last(h, n)    (dl_insert_after((h)->last, (n)))
#define dl_remove_first(h)      dl_remove((h)->first) // mustn't be empty!
#define dl_remove_last(h)       dl_remove((h)->last) // mustn't be empty!

#endif /* ETLIB_DLLIST_H */