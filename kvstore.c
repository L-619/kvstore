        
#include"kvstore.h"


#define KVSTORE_MAX_TOKENS 128

const char* commands[] = {
	"SET","GET","DEL","MOD","COUNT",
	"RSET","RGET","RDEL","RMOD","RCOUNT",
	"HSET","HGET","HDEL","HMOD","HCOUNT",
	NULL
};

enum {
	KVSTORE_CMD_START = 0,
	KVSTORE_CMD_SET = KVSTORE_CMD_START,
	KVSTORE_CMD_GET,
	KVSTORE_CMD_DEL,
	KVSTORE_CMD_MOD,
	KVSTORE_CMD_COUNT,

	KVSTORE_CMD_RSET,
	KVSTORE_CMD_RGET,
	KVSTORE_CMD_RDEL,
	KVSTORE_CMD_RMOD,
	KVSTORE_CMD_RCOUNT,

	KVSTORE_CMD_HSET,
	KVSTORE_CMD_HGET,
	KVSTORE_CMD_HDEL,
	KVSTORE_CMD_HMOD,
	KVSTORE_CMD_HCOUNT,
	KVSTORE_CMD_FORCOUNT = KVSTORE_CMD_HCOUNT,
};

void* kvstore_malloc(size_t size)//内存分配封装
{
#if ENABLE_MEMPOOL
	return mp_alloc(&m);
#else
	return malloc(size);
#endif // 
}
void kvstore_free(void* ptr)//内存释放封装
{
#if ENABLE_MEMPOOL
	return mp_free(&m, ptr);
#else
	return free(ptr); 
#endif // 
	
}


#if ENABLE_ARRAY_KVENGINE //数组存储引擎适配层

int kvstore_array_set(char* key, char* value)
{
	return array_kv_set(&Array, key, value);
}

char* kvstore_array_get(char* key)
{
	return array_kv_get(&Array, key);
}
int  kvstore_array_delete(char* key)
{
	return array_kv_delete(&Array, key);
}
int kvstore_array_modify(char* key, char* value)
{
	return array_kv_modify(&Array, key, value);
}
int kvstore_array_count(void)
{
	return array_kv_count(&Array);
}

#endif



#if ENABLE_RBTREE_KVENGINE //红黑树存储引擎适配层



int kvstore_rbtree_set(char* key, char* value)
{
	return rbtree_kv_set(&Tree, key, value);
}

char* kvstore_rbtree_get(char* key)
{
	return rbtree_kv_get(&Tree, key);
}
int  kvstore_rbtree_delete(char* key)
{
	return rbtree_kv_delete(&Tree, key);
}
int kvstore_rbtree_modify(char* key, char* value)
{
	return rbtree_kv_modify(&Tree, key, value);
}
int kvstore_rbtree_count(void)
{
	return rbtree_kv_count(&Tree);
}


#endif 



#if ENABLE_HASH_KVENGINE //哈希存储引擎适配层

int kvstore_hash_set(char* key, char* value)
{
	return hash_kv_set(&Hash, key, value);
}

char* kvstore_hash_get(char* key)
{
	return hash_kv_get(&Hash, key);
}
int  kvstore_hash_delete(char* key)
{
	return hash_kv_delete(&Hash, key);
}
int kvstore_hash_modify(char* key, char* value)
{
	return hash_kv_modify(&Hash, key, value);
}
int kvstore_hash_count(void)
{
	return hash_kv_count(&Hash);
}

#endif


int kvstore_split_token(char* msg, char** tokens)//拆分字符
{
	if(msg==NULL||tokens==NULL) return -1;
	int idx = 0;
	char* token = strtok(msg, " ");
	while(token!= NULL && idx <KVSTORE_MAX_TOKENS)
	{
		tokens[idx++] = token;
		token = strtok(NULL, " ");
	}
	return idx;
}

int kvstore_parser_protocol(struct conn_item* item,char** tokens,int count)//解析协议
{
	if (item == NULL || tokens[0] == NULL || count <= 0) return -1;
	int cmd = KVSTORE_CMD_START;
	for(cmd=KVSTORE_CMD_START;cmd<=KVSTORE_CMD_FORCOUNT;cmd++)
	{
		if(strcmp(tokens[0],commands[cmd])==0)
		{
			break;
		}
	}
	char* msg = item->wbuffer;//响应消息缓冲区（回复给客户端的消息）
	memset(msg, 0, BUFFER_SIZE);//清空缓冲区
		switch (cmd)
		{
		case KVSTORE_CMD_SET: {
			int res = kvstore_array_set(tokens[1], tokens[2]);
			if (res == 0)
			{
				printf("set command\n");
				snprintf(msg, BUFFER_SIZE, "SUCCESS\r\n");
			}
			else
			{
				snprintf(msg, BUFFER_SIZE, "FAILED\r\n");
			}
			break;
		}
		case KVSTORE_CMD_GET:{
			char* value = kvstore_array_get(tokens[1]);
			if (value != NULL)
			{
				snprintf(msg, BUFFER_SIZE, "SUCCESS:%s\r\n", value);
			}
			else
			{
				snprintf(msg, BUFFER_SIZE, "NO EXIST\r\n");
			}
			break;
		}
		case KVSTORE_CMD_MOD: {
			printf("mod command\n");
			int res = kvstore_array_modify(tokens[1],tokens[2]);
			if (res < 0)
			{
				snprintf(msg, BUFFER_SIZE, "%s\r\n", "ERROR");
			}
			else if (res == 0)
			{
				snprintf(msg, BUFFER_SIZE, "%s\r\n", "SUCCESS");
			}
			else
			{
				snprintf(msg, BUFFER_SIZE, "NO EXIST\r\n");
			}
			break;
		}
		case KVSTORE_CMD_DEL: {
			printf("del command\n");
			int res = kvstore_array_delete(tokens[1]);
			if (res < 0)
			{
				snprintf(msg, BUFFER_SIZE,"%s\r\n","ERROR");
			}
			else if(res==0)
			{
				snprintf(msg, BUFFER_SIZE,"%s\r\n","SUCCESS");
			}
			else
			{
				snprintf(msg, BUFFER_SIZE,"NO EXIST\r\n");
			}
			break;
		}
		case KVSTORE_CMD_COUNT: {
			int count = kvstore_array_count();
			if (count < 0)
			{
				snprintf(msg, BUFFER_SIZE, "%s\r\n", "ERROR");
			}
			else
			{
				snprintf(msg, BUFFER_SIZE, "%d\r\n", count);
			}
				break;
		}
		case KVSTORE_CMD_RSET: {
			int res = kvstore_rbtree_set(tokens[1], tokens[2]);
			if (res == 0)
			{
				printf("set command\r\n");
				snprintf(msg, BUFFER_SIZE, "SUCCESS\r\n");
			}
			else
			{
				snprintf(msg, BUFFER_SIZE, "FAILED\r\n");
			}
			break;
		}
		case KVSTORE_CMD_RGET: {
			char* value = kvstore_rbtree_get(tokens[1]);
			if (value != NULL)
			{
				snprintf(msg, BUFFER_SIZE, "SUCCESS:%s\r\n", value);
			}
			else
			{
				snprintf(msg, BUFFER_SIZE, "NO EXIST\r\n");
			}
			break;
			break;
		}
		case KVSTORE_CMD_RDEL: {
			int res = kvstore_rbtree_delete(tokens[1]);
			if (res < 0)
			{
				snprintf(msg, BUFFER_SIZE, "%s\r\n", "ERROR");
			}
			else if (res == 0)
			{
				snprintf(msg, BUFFER_SIZE, "%s\r\n", "SUCCESS");
			}
			else
			{
				snprintf(msg, BUFFER_SIZE, "NO EXIST\r\n");
			}
			break;
		}
		case KVSTORE_CMD_RMOD: {
			int res = kvstore_rbtree_modify(tokens[1], tokens[2]);
			if (res < 0)
			{
				snprintf(msg, BUFFER_SIZE, "%s\r\n", "ERROR");
			}
			else if (res == 0)
			{
				snprintf(msg, BUFFER_SIZE, "%s\r\n", "SUCCESS");
			}
			else
			{
				snprintf(msg, BUFFER_SIZE, "NO EXIST\r\n");
			}
			break;
		}
		case KVSTORE_CMD_RCOUNT: {
			int count = kvstore_rbtree_count();
			if (count < 0)
			{
				snprintf(msg, BUFFER_SIZE, "%s\r\n", "ERROR");
			}
			else
			{
				snprintf(msg, BUFFER_SIZE, "%d\r\n", count);
			}
			break;
		}
		case KVSTORE_CMD_HSET: {
			int res = kvstore_hash_set(tokens[1], tokens[2]);
			if (res == 0)
			{
				printf("set command\n");
				snprintf(msg, BUFFER_SIZE, "SUCCESS\r\n");
			}
			else
			{
				snprintf(msg, BUFFER_SIZE, "FAILED\r\n");
			}
			break;
		}
		case KVSTORE_CMD_HGET: {
			char* value = kvstore_hash_get(tokens[1]);
			if (value != NULL)
			{
				snprintf(msg, BUFFER_SIZE, "SUCCESS:%s\r\n", value);
			}
			else
			{
				snprintf(msg, BUFFER_SIZE, "NO EXIST\r\n");
			}
			break;
		}
		case KVSTORE_CMD_HMOD: {
			printf("mod command\n");
			int res = kvstore_hash_modify(tokens[1], tokens[2]);
			if (res < 0)
			{
				snprintf(msg, BUFFER_SIZE, "%s\r\n", "ERROR");
			}
			else if (res == 0)
			{
				snprintf(msg, BUFFER_SIZE, "%s\r\n", "SUCCESS");
			}
			else
			{
				snprintf(msg, BUFFER_SIZE, "NO EXIST\r\n");
			}
			break;
		}
		case KVSTORE_CMD_HDEL: {
			printf("del command\n");
			int res = kvstore_hash_delete(tokens[1]);
			if (res < 0)
			{
				snprintf(msg, BUFFER_SIZE, "%s\r\n", "ERROR");
			}
			else if (res == 0)
			{
				snprintf(msg, BUFFER_SIZE, "%s\r\n", "SUCCESS");
			}
			else
			{
				snprintf(msg, BUFFER_SIZE, "NO EXIST\r\n");
			}
			break;
		}
		case KVSTORE_CMD_HCOUNT: {
			int count = kvstore_hash_count();
			if (count < 0)
			{
				snprintf(msg, BUFFER_SIZE, "%s\r\n", "ERROR");
			}
			else
			{
				snprintf(msg, BUFFER_SIZE, "%d\r\n", count);
			}
			break;
		}
			default: {
				printf("unknown command\n");
				assert(0);
			}
				
		}
		return 0;
}

int kvstore_request(struct conn_item* item)//处理请求
{
	char* msg = item->rbuffer;
	char* tokens[KVSTORE_MAX_TOKENS] = { 0 };
	int count=kvstore_split_token(msg, tokens);
	
	int idx = 0;
	for (idx = 0; idx < count; idx++)
	{
		printf("token[%d]:%s\n", idx, tokens[idx]);
	}
	kvstore_parser_protocol(item, tokens, count);

	return 0;
}
//初始化kv引擎
int init_kvengine(void)
{
#if ENABLE_ARRAY_KVENGINE
	printf("init array begin\n");
	kvstore_array_create(&Array);
#endif

#if ENABLE_RBTREE_KVENGINE
	printf("init rbtree begin\n");
	kvstore_rbtree_create(&Tree);
#endif
#if ENABLE_HASH_KVENGINE
	printf("init hash begin\n");
	kvstore_hash_create(&Hash);
#endif
}
//退出
int exit_kvengine(void)
{
#if ENABLE_ARRAY_KVENGINE
	kvstore_array_destroy(&Array);
#endif

#if ENABLE_RBTREE_KVENGINE

	kvstore_rbtree_destroy(&Tree);
#endif
#if ENABLE_HASH_KVENGINE
	kvstore_hash_destroy(&Hash);
#endif
}


void init_ctx(void)
{
#if ENABLE_MEMPOOL
	mp_init(&m,256);
#endif
	return;
}

int main()
{
	init_ctx();
	init_kvengine();
#if (ENABLE_NETWORE_SELECT==NETWORK_EPOLL)
	epoll_entry();
#elif(ENABLE_NETWORE_SELECT==NETWORK_NTYCO)
	
	ntyco_entry();
#elif(ENABLE_NETWORE_SELECT==NETWORK_IOURING)
#endif

	exit_kvengine();
	return 0;
}