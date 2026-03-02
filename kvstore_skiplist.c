#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<limits.h>
#include"kvstore.h"

#define MAX_LEVEL 6




Node *createNode(int key, int level) {
	Node* newNode = (Node*)malloc(sizeof(Node));
	newNode->key = key;
	newNode->forward = (Node**)malloc(sizeof(Node*) * (level + 1));
	for (int i = 0; i <= level; i++) {
		newNode->forward[i] = NULL;
	}
	return newNode;
}

int* createSkipList(SkipList* skiplist) {
	if (skiplist == NULL) return -1;
	skiplist->header = createNode(INT_MIN, MAX_LEVEL);
	skiplist->level = 0;
	return 0;
}


int randomLevel() {
	int level = 0;
	while (rand()<RAND_MAX/2 && level < MAX_LEVEL) {
		level++;
	}
	return level;
}

void insert(SkipList* skiplist, int key) {
	Node* current = skiplist->header;
	Node* update[MAX_LEVEL + 1];
	for (int i = skiplist->level; i >= 0; i--) {
		while (current->forward[i] != NULL && current->forward[i]->key < key) {
			current = current->forward[i];
		}
		update[i] = current;
	}
	current = current->forward[0];
	if (current == NULL || current->key != key) {
		int newLevel = randomLevel();
		if (newLevel > skiplist->level) {
			for (int i = skiplist->level + 1; i <= newLevel; i++) {
				update[i] = skiplist->header;
			}
			skiplist->level = newLevel;
		}
		Node* newNode = createNode(key, newLevel);
		for (int i = 0; i <= newLevel; i++) {
			newNode->forward[i] = update[i]->forward[i];
			update[i]->forward[i] = newNode;
		}
	}
}

bool Search(SkipList* skiplist, int key) {
	Node* current = skiplist->header;
	for (int i = skiplist->level; i >= 0; i--) {
		while (current->forward[i] != NULL && current->forward[i]->key < key) {
			current = current->forward[i];
		}
	}
	current = current->forward[0];
	return current != NULL && current->key == key;
}

void deleteNode(SkipList* skiplist, int key) {
	Node* current = skiplist->header;
	Node* update[MAX_LEVEL + 1];
	for (int i = skiplist->level; i >= 0; i--) {
		while (current->forward[i] != NULL && current->forward[i]->key < key) {
			current = current->forward[i];
		}
		update[i] = current;
	}
	current = current->forward[0];
	if (current != NULL && current->key == key) {
		for (int i = 0; i <= skiplist->level; i++) {
			if (update[i]->forward[i] != current) {
				break;
			}
			update[i]->forward[i] = current->forward[i];
		}
		free(current);
		while (skiplist->level > 0 && skiplist->header->forward[skiplist->level] == NULL) {
			skiplist->level--;
		}
	}
}

void printfSkipList(SkipList* skiplist) {
	for (int i = skiplist->level; i >= 0; i--) {
		Node* current = skiplist->header->forward[i];
		printf("Level %d: ", i);
		while (current != NULL) {
			printf("%d ", current->key);
			current = current->forward[i];
		}
		printf("\n");
	}
}

//5+2

skiptable_t Skiptable;




int kvstore_skiplist_create(skiptable_t* table)
{
	return createSkipList(table);
}

int kvstore_skiplist_destroy(skiptable_t* table)
{
	return 0;
}