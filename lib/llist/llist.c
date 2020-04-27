
/*
 * llist.c
 */

#include <stdio.h>
#include <stdlib.h>
#include "llist.h"


struct Lnode *addFront(struct List *list, void *data)
{
    struct Lnode *node = (struct Lnode *)malloc(sizeof(struct Lnode));
    if (node == NULL)
	return NULL;

    node->data = data;
    node->next = list->head;
    list->head = node;
    return node;
}

void traverseList(struct List *list, void (*f)(void *))
{
    struct Lnode *node = list->head;
    while (node) {
	f(node->data);
	node = node->next;
    }
}

void flipSignDouble(void *data)
{
    *(double *)data = *(double *)data * -1;
}

int compareDouble(const void *data1, const void *data2)
{
    if (*(double *)data1 == *(double *)data2)
	return 0;
    else 
	return 1;
}

int compareInt(const void *data1, const void *data2)
{
    if (*(int *)data1 == *(int *)data2)
	return 0;
    else 
	return 1;
}

struct Lnode *findNode(struct List *list, const void *dataSought,
	int (*compar)(const void *, const void *))
{
    struct Lnode *node = list->head;
    while (node) {
	if (compar(dataSought, node->data) == 0)
	    return node;
	node = node->next;
    }
    return NULL;
}

void *popFront(struct List *list)
{
    if (isEmptyList(list))
	return NULL;

    struct Lnode *oldHead = list->head;
    list->head = oldHead->next;
    void *data = oldHead->data;
    free(oldHead);
    return data;
}

void removeAllNodes(struct List *list)
{
    while (!isEmptyList(list))
	popFront(list);
}

struct Lnode *addAfter(struct List *list, 
	struct Lnode *prevNode, void *data)
{
    if (prevNode == NULL)
	return addFront(list, data);

    struct Lnode *node = (struct Lnode *)malloc(sizeof(struct Lnode));
    if (node == NULL)
	return NULL;

    node->data = data;
    node->next = prevNode->next;
    prevNode->next = node;
    return node;
}

void reverseList(struct List *list)
{
    struct Lnode *prv = NULL;
    struct Lnode *cur = list->head;
    struct Lnode *nxt;

    while (cur) {
	nxt = cur->next;
	cur->next = prv;
	prv = cur;
	cur = nxt;
    }

    list->head = prv;
}

struct Lnode *addBack(struct List *list, void *data)
{
    // make the new node that will go to the end of list
    struct Lnode *node = (struct Lnode *)malloc(sizeof(struct Lnode));
    if (node == NULL)
	return NULL;
    node->data = data;
    node->next = NULL;

    // if the list is empty, this node is the head
    if (list->head == NULL) {
	list->head = node;
	return node;
    }

    // find the last node
    struct Lnode *end = list->head;
    while (end->next != NULL)
	end = end->next;

    // 'end' is the last node at this point
    end->next = node;
    return node;
}

