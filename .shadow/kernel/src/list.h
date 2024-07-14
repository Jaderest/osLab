#ifndef _LIST_H
#define _LIST_H

struct list_head {
  struct list_head *next, *prev;
};

static inline void INIT_LIST_HEAD(struct list_head *list) {
  list->next = list;
  list->prev = list;
}

static inline void list_add(struct list_head *node, struct list_head *head) {
  node->next = head->next;
  node->prev = head;
  head->next->prev = node;
  head->next = node;
}

static inline void list_append(struct list_head *node, struct list_head *head) {
  struct list_head *tail = head->prev;
  return list_add(node, tail);
}

#endif