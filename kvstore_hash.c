#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<assert.h>
#include"kvstore.h"


#define MAX_TABLE_SIZE 1024

//哈希节点定义
typedef struct hashnode_s {
	char *key;
	char *value;

	struct hashnode_s* next;
}hashnode_t;

//哈希表定义
typedef struct hashtable_s {
	hashnode_t** nodes;

	int max_slots;
	int count;

}hashtable_t;

hashtable_t Hash; 

//哈希映射函数
static int _hash(char* key, int size) {
	if (!key)return -1;
	int sum = 0;
	int  i = 0;
	while (key[i] != 0)
	{
		sum += key[i];
		++i;
	}
	return sum % size;
}

//根据给定的key，value创建哈希节点
hashnode_t* _create_node(char* key, char* value)
{
	hashnode_t* node = (hashnode_t*)kvstore_malloc(sizeof(hashnode_t));
	if (!node)return NULL;
	
	node->key = kvstore_malloc(strlen(key) + 1);
	if (!node->key)
	{
		kvstore_free(node);
		return NULL;
	}
	strcpy(node->key, key);

	node->value = kvstore_malloc(strlen(value) + 1);
	if(!node->value)
	{
		kvstore_free(node->key);
		kvstore_free(node);
		return NULL;
	}
	strcpy(node->value, value);

	node->next = NULL;
	return node;
}

//哈希表的初始化
int init_hahstable(hashtable_t* hash)
{
	if (!hash)return -1;
	hash->nodes = (hashnode_t**)kvstore_malloc(sizeof(hashnode_t) * MAX_TABLE_SIZE);
	if (!hash->nodes)return -1;

	hash->max_slots = MAX_TABLE_SIZE;
	hash->count = 0;

	
	
	return 0;
}
//哈希表的销毁
void dest_hashtable(hashtable_t* hash)
{
	if (!hash)return;
	int i = 0;
	for (i = 0; i < hash->max_slots; ++i)
	{
		hashnode_t* node = hash->nodes[i];
		while (node != NULL)
		{
			hashnode_t* tmp = node;
			node = node->next;
			hash->nodes[i] = node;

			kvstore_free(tmp);
		}
	}
	kvstore_free(hash->nodes);
}

//哈希表的插入
int put_kv_hashtable(hashtable_t* hash, char* key, char* value)
{
	if (!key || !hash || !value)return -1;
	int idx = _hash(key, MAX_TABLE_SIZE);

	
	hashnode_t* node = hash->nodes[idx];

	//若该key值已插入，return，否则执行插入操作(idx处首插)
#if 1
	while (node != NULL)
	{
		if (strcmp(node->key, key) == 0)
		{
			
			return 1;
		}
		node = node->next;
	}
#endif
	hashnode_t* newnode = _create_node(key, value);
	newnode->next = hash->nodes[idx];
	hash->nodes[idx] = newnode;

	hash->count++;



	return 0;
}

//哈希查找
char* get_kv_hashtable(hashtable_t* hash, char* key)
{
	if (!hash || !key)return NULL;
	int idx = _hash(key, MAX_TABLE_SIZE);

	

	hashnode_t* node = hash->nodes[idx];
	while (node != NULL)
	{
		if(strcmp(node->key, key) == 0)
		{
			
			return node->value;
		}
		node = node->next;
	}


	return NULL;
}
//哈希获取count
int count_kv_hashtable(hashtable_t* hash)
{
	return hash->count;
}
//哈希删key
int delete_kv_hashtable(hashtable_t* hash, char* key)
{
	if (!hash || !key)return -2;
	int idx = _hash(key, MAX_TABLE_SIZE);

	
	hashnode_t* head = hash->nodes[idx];
	if (head == NULL)return -1;//无待删除key
	if (strcmp(head->key, key) == 0)
	{
		hashnode_t* tmp = head->next;
		hash->nodes[idx] = tmp;
		if (head->key) {
			kvstore_free(head->key);	
		}
		if(head->value){
			kvstore_free(head->value);
		}
		kvstore_free(head);
		hash->count--;
		
		return 0;
	}
	

	hashnode_t* cur = head;
	while (cur->next != NULL)
	{
		if (strcmp(head->key, key) == 0)
		{
			break;
		}
		cur = cur->next;
	}
	if (cur->next == NULL)
	{
		
		return -1;
	}

	hashnode_t* tmp = cur->next;
	cur->next = tmp->next;
	if (tmp->key) {
		kvstore_free(tmp->key);
	}
	if (tmp->value) {
		kvstore_free(tmp->value);
	}
	kvstore_free(tmp);
	hash->count--;
	
	
	return 0;
}

//检查是否存在key
int exist_kv_hashtabl(hashtable_t* hash, char* key)
{
	char* value = get_kv_hashtable(hash, key);
	if (value)return 1;
	else return 0;
}



//5+2接口

int kvstore_hash_create(hashtable_t* hash)
{
	return init_hahstable(hash);
}
void kvstore_hash_destroy(hashtable_t* hash)
{
	return dest_hashtable(hash);
}
int hash_kv_set(hashtable_t* hash, char* key, char* value)
{
	return put_kv_hashtable(hash, key, value);
}
char* hash_kv_get(hashtable_t* hash, char* key)
{
	return get_kv_hashtable(hash, key);
}
int  hash_kv_delete(hashtable_t* hash, char* key)
{
	return delete_kv_hashtable(hash, key);
}
int hash_kv_modify(hashtable_t* hash, char* key, char* value)
{
	if (!hash || !key||!value)return -1;
	int idx = _hash(key, MAX_TABLE_SIZE);



	hashnode_t* node = hash->nodes[idx];
	while (node != NULL)
	{
		if(strcmp(node->key, key) == 0)
		{
			kvstore_free(node->value);

			node->value = kvstore_malloc(strlen(value) + 1);
			if (node->value)
			{
				strcpy(node->value, value);
			}
			else
			{
				assert(0);
			}
		}
		node = node->next;
	}
	

	return 0;
}
int hash_kv_count(hashtable_t* hash)
{
	return count_kv_hashtable(hash);
}

