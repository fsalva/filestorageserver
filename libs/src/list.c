#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../list.h"

//display the list
void printList(elem_t * head) {
    elem_t *ptr = head;
    fprintf(stderr, "\n[ ");
	
    //start from the beginning
    while(ptr != NULL) {
      fprintf(stderr, "(%d,%d) ",ptr->key,ptr->data);
      ptr = ptr->next;
    }
	
   printf(" ]");
}

//insert link at the first location 
void push(elem_t ** head, int key, int data) {
   //create a link
    elem_t *link = ( elem_t*) malloc(sizeof(elem_t));
	//
   link->key = key;
   link->data = data;
	
   //point it to old first elem
   link->next = * head;
	
   //point first to new first elem
   * head = link;
}

//delete first item
 elem_t* pop(elem_t ** head) {

   //save reference to first link
    elem_t *tempLink = * head;
	
   //mark next to first link as first 
   * head = (*head)->next;
	
   //return the deleted link
   return tempLink;
}

//is list empty
bool isEmpty(elem_t *head) {
   return head == NULL;
}

int length(elem_t *head) {
   int length = 0;
   elem_t *current;
	
   for(current = head; current != NULL; current = current->next) {
      length++;
   }
	
   return length;
}

//find a link with given key
 elem_t* find(elem_t ** head, int key) {

   //start frm the first link 
    elem_t* current = * head;

   //if list is empty
   if(* head == NULL) {
      return NULL;
   }

   //navigate through list
   while(current->key != key) {
	
      //if it is last elem_t
      if(current->next == NULL) {
         return NULL;
      } else {
         //go to next link
         current = current->next;
      }
   }      
	
   //if data found, return the current Link
   return current;
}

//delete a link with given key
 elem_t* delete(elem_t ** head, int key) {

   //start from the first link
    elem_t* current = * head;
    elem_t* previous = NULL;
	
   //if list is empty
   if(* head == NULL) {
      return NULL;
   }

   //navigate through list
   while(current->key != key) {

      //if it is last elem_t
      if(current->next == NULL) {
         return NULL;
      } else {
         //store reference to current link
         previous = current;
         //move to next link
         current = current->next;
      }
   }

   //found a match, update the link
   if(current == *head) {
      //change first to point to next link
      * head = (* head)->next;
   } else {
      //bypass the current link
      previous->next = current->next;
   }    
	
   return current;
}

