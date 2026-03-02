#ifndef _KVSTORE_H_
#define _KVSTORE_H_
#include<string.h>
#include<assert.h>
#include<stdlib.h>
#include<stddef.h>
#include<stdio.h>
#define BUFFER_SIZE 128



typedef int (*RCALLBACK)(int fd);//

struct conn_item
{
	int fd;
	union {
		RCALLBACK accept_callback;
		RCALLBACK recv_callback;
	}recv_t;
	RCALLBACK send_callback;

	char rbuffer[BUFFER_SIZE];
	int rlen;
	char wbuffer[BUFFER_SIZE];
	int wlen;
	char source[BUFFER_SIZE];

};
int ntyco_entry(void);//ntyco网络框架入口
int epoll_entry(void);//epoll网络框架入口
int kvstore_request(struct conn_item* item);//处理请求



#define ENABLE_MEMPOOL 0
//malloc封装
void* kvstore_malloc(size_t size);

//释放内存封装(free)
void kvstore_free(void* ptr);


//内存池相关定义和函数声明
#if ENABLE_MEMPOOL

typedef struct mempool_s {
	int block_size;//每个小内存块的大小
	int free_count;//剩余可用的小内存块数量

	char* free_ptr;//指向第一个可用的小内存块
	char* mem;//分配的大内存块，一个PAGE
}mempool_t;
int mp_init(mempool_t* m, int size);
void mp_dest(mempool_t* m);
void* mp_alloc(mempool_t* m);
void mp_free(mempool_t* m, void* ptr);

extern mempool_t m;

#endif




#define NETWORK_EPOLL      0
#define NETWORK_NTYCO      1
#define NETWORK_IOURING    2

//存储类型选择
//kv开启存储数组
#define ENABLE_ARRAY_KVENGINE 1
//kv开启存储红黑树
#define ENABLE_RBTREE_KVENGINE 1
//kv开启存储跳表
#define ENABLE_SKIPLIST_KVENGINE 0
//kv开启存储哈希表
#define ENABLE_HASH_KVENGINE 1

//选择网络框架
#define ENABLE_NETWORE_SELECT  NETWORK_EPOLL





//数组存储引擎
#if ENABLE_ARRAY_KVENGINE
struct kvs_array_item {
	char* key;
	char* value;
};
#define KVS_ARRAY_SIZE 1024//定义存数组的大小

typedef struct array_s {   
	struct kvs_array_item* array_table;
	int array_idx;
}array_t;
extern array_t Array;//数组存储引擎的全局变量

int kvstore_array_create(array_t* arr);//数组的初始化 
void kvstore_array_destroy(array_t* arr);//数组的销毁
int array_kv_set(array_t* arr,char* key, char* value);//数组的set操作
char* array_kv_get(array_t* arr, char* key);//数组的get操作
int  array_kv_delete(array_t* arr, char* key);//数组的delete操作
int array_kv_modify(array_t* arr,char* key, char* value);//数组的mod操作
int array_kv_count(array_t* arr);//数组的count操作

#endif


//红黑树存储引擎
#if ENABLE_RBTREE_KVENGINE


#define RED      1
#define BLACK    2

#define ENABLE_KEY_CHAR 1

#define MAX_KEY_LEN 256
#define MAX_VALUE_LEN 1024

#if ENABLE_KEY_CHAR 
typedef char* KEY_TYPE;//
#else 
typedef int KEY_TYPE;
#endif

typedef struct _rbtree_node {
	unsigned char color;
	struct _rbtree_node* left;
	struct _rbtree_node* right;
	struct _rbtree_node* parent;

	KEY_TYPE key;
	void* value;
} rbtree_node;


typedef struct _rbtree {
	rbtree_node* root;
	rbtree_node* nil;
	int count;
}rbtree;


typedef struct _rbtree rbtree_t;
extern rbtree_t Tree;


int kvstore_rbtree_create(rbtree_t* tree);
void kvstore_rbtree_destroy(rbtree_t* tree);

int rbtree_kv_set(rbtree_t* tree, char* key, char* value);

char* rbtree_kv_get(rbtree_t* tree, char* key);
int  rbtree_kv_delete(rbtree_t* tree, char* key);
int rbtree_kv_modify(rbtree_t* tree, char* key, char* value);

int rbtree_kv_count(rbtree_t* tree);//红黑树的count操作

#endif

//跳表存储引擎
#if ENABLE_SKIPLIST_KVENGINE


typedef struct Node {
	int key;
	struct Node** forward;
}Node;


typedef struct SkipList {
	Node* header;
	int level;
}SkipList;
typedef struct SkipList skiptable_t;

extern skiptable_t Skiptable;



#endif

//哈希表存储引擎
#if ENABLE_HASH_KVENGINE

typedef struct hashtable_s hashtable_t;
extern hashtable_t Hash;


int kvstore_hash_create(hashtable_t* hash);
void kvstore_hash_destroy(hashtable_t* hash);

int hash_kv_set(hashtable_t* hash, char* key, char* value);
char* hash_kv_get(hashtable_t* hash, char* key);
int  hash_kv_delete(hashtable_t* hash, char* key);
int hash_kv_modify(hashtable_t* hash, char* key, char* value);
int hash_kv_count(hashtable_t* hash);


#endif








#endif