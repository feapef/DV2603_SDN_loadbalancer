#include <netinet/in.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* LINKED LIST
 * 
 * Implementation of the data structure linked list 
 * for the management of the list of web servers
 * */

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


node *create(char ip[INET_ADDRSTRLEN],float load){
    node *n=malloc(sizeof(node));
    assert(n!=NULL);
    strcpy(n->ip,ip);
    n->load=load;
    n->next=NULL;
    return n;
}

linked_list *init(){
    linked_list *l=malloc(sizeof(linked_list));
    assert(l!=NULL);
    l->size=0;
	l->first=NULL;
    return l;
}

void add(linked_list *l,char ip[INET_ADDRSTRLEN],float load){
    node* n=create(ip,load);
	n->next=l->first;
	l->first=n;
	l->size++;
}
int rm(linked_list *l, char ip[INET_ADDRSTRLEN]){
    // remove one element from the linked list
    // return 0 when the element is removed
    // return -1 if the element is not present in the list
    // return -2 if list is empty
	if (l->size==0){
		return -2;
	}
    node *current=l->first;
    node *tmp;
    if (strcmp(current->ip,ip)==0){
        // first element to remove
        l->first=current->next;
        free(current);
        l->size--;
        return 0;
    }
    for(int i=0;i<l->size-1;i++){
        if(strcmp(current->next->ip,ip)==0){
	    tmp=current->next;
            current->next=current->next->next;
            free(tmp);
            l->size--;
            return 0;
        }
        else 
            current=current->next;
    }
    // ip not matched
    return -1;
}
float get_load(linked_list *l, char ip[INET_ADDRSTRLEN]){
    // Return the load value linked to a given IP
    // Return -1 if the element is not present in the list 
    node* current=l->first;
    for(int i=0;i<l->size;i++){
        if(strcmp(current->ip,ip)==0){
            return current->load;
        }
        else 
            current=current->next;
    }
    // ip not matched
    return -1;
}

int set_load(linked_list *l, char ip[INET_ADDRSTRLEN],float load){
    // Return 0  when the load is modified
    // Return -1 if the element is not present in the list 
    node* current=l->first;
    for(int i=0;i<l->size;i++){
        if(strcmp(current->ip,ip)==0){
            current->load=load;
            return 0;
        }
        else 
            current=current->next;
    }
    // ip not matched
    return -1;
}

void display(linked_list *l){
    node* current=l->first;
    for(int i=0;i<l->size;i++){
        printf("[%s : %f]->\n",current->ip,current->load);
        current=current->next;
    }
    printf("[X]\n");

}
