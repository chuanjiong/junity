/*
 * jlist.h
 *
 * @chuanjiong
 */

#ifndef _J_LIST_H_
#define _J_LIST_H_

//Copy from linux/list.h

struct list_head {
    struct list_head *next, *prev;
};

static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

static inline void __list_add(struct list_head *add,
                              struct list_head *prev,
                              struct list_head *next)
{
    next->prev = add;
    add->next = next;
    add->prev = prev;
    prev->next = add;
}

static inline void list_add(struct list_head *add, struct list_head *head)
{
    __list_add(add, head, head->next);
}

static inline void list_add_tail(struct list_head *add, struct list_head *head)
{
    __list_add(add, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

static inline int list_is_last(const struct list_head *list, const struct list_head *head)
{
    return list->next == head;
}

static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

#define OFFSETOF(TYPE, MEMBER)  ((size_t)&((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({ \
    const typeof(((type *)0)->member) * __mptr = (ptr); \
    (type *)((char *)__mptr - OFFSETOF(type, member)); })

#define list_entry(ptr, type, member)   container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) \
    list_entry((ptr)->prev, type, member)

#define list_next_entry(pos, member) \
    list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_prev_entry(pos, member) \
    list_entry((pos)->member.prev, typeof(*(pos)), member)

#define list_for_each_entry(pos, head, member) \
    for (pos = list_first_entry(head, typeof(*pos), member); \
         &pos->member != (head); \
         pos = list_next_entry(pos, member))

#endif //_J_LIST_H_


