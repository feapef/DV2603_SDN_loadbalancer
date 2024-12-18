#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <netinet/in.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

typedef struct node_s{
    char ip[INET_ADDRSTRLEN];
    float load;
    struct node_s* next;
}node;

typedef struct ll_s{
    node *first;
    int size;
}linked_list;


node *create(char ip[INET_ADDRSTRLEN],float load);
linked_list *init();
void add(linked_list *l,char ip[INET_ADDRSTRLEN],float load);
int rm(linked_list *l, char ip[INET_ADDRSTRLEN]);
float get_load(linked_list *l, char ip[INET_ADDRSTRLEN]);
int set_load(linked_list *l, char ip[INET_ADDRSTRLEN],float load);
void next(linked_list *l);
void display(linked_list *l);

#endif //LINKED_LIST_H
