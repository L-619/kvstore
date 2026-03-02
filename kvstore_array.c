#include "kvstore.h"



array_t Array;


//数组的初始化create
int kvstore_array_create(array_t *arr)
{
	if (!arr) return -1;
	arr->array_table = malloc(sizeof(struct kvs_array_item) * KVS_ARRAY_SIZE);
	if (!arr->array_table)
	{
		return -1;
	}
	memset(arr->array_table, 0, sizeof(struct kvs_array_item) * KVS_ARRAY_SIZE);
	arr->array_idx = 0;
	return 0;
}
//数组的销毁destory
void kvstore_array_destroy(array_t*arr)//数组的销毁
{
	if (!arr) return ;
	if (!arr->array_table)
	{
		free(arr->array_table);
	}
}

int array_kv_set(array_t* arr,char *key,char *value)//数组的set操作
{
	if (arr==NULL||key == NULL || value == NULL) return -1;
	if (arr->array_idx == KVS_ARRAY_SIZE)
		return -1;
	char* kcopy =(char *) malloc(strlen(key)+1);
	if (kcopy == NULL) return -1;
	char* vcopy = (char*)malloc(strlen(value) + 1);
	if(vcopy == NULL)
	{
		free((void*)kcopy);
		return -1;
	}
	strcpy(kcopy, key);
	strcpy(vcopy, value);

	int i = 0;
	for (i = 0; i < arr->array_idx; ++i)
	{
		if (arr->array_table[i].key == NULL)
		{
			arr->array_table[arr->array_idx].key = kcopy;
			arr->array_table[arr->array_idx].value = vcopy;
			arr->array_idx++;
			return 0;
		}
	}
	if (i < KVS_ARRAY_SIZE && i == arr->array_idx)
	{
		arr->array_table[arr->array_idx].key = kcopy;
		arr->array_table[arr->array_idx].value = vcopy;
		arr->array_idx++;
	}
	
	return 0;
}



char * array_kv_get(array_t* arr, char* key)//数组的get操作
{
	if(arr==NULL||key==NULL)
		return NULL;
	for (int i = 0; i < arr->array_idx; ++i)
	{
		if(arr->array_table[i].key==NULL)
			continue;
		if (strcmp(arr->array_table[i].key, key) == 0)
		{
			return arr->array_table[i].value;
		}
	}
	return NULL;
}
int  array_kv_delete(array_t* arr, char* key)//数组的delete操作
{
	if(arr==NULL||key==NULL)
		return -1;	
	int i = 0;
	for (i = 0; i < arr->array_idx; ++i)
	{
		if (strcmp(arr->array_table[i].key, key) == 0)
		{
			free((void*)arr->array_table[i].key);
			free((void*)arr->array_table[i].value);
			arr->array_table[i].key = NULL;
			arr->array_table[i].value = NULL;
			arr->array_idx--;
			return 0;
		}
	}
	return i;
}
int array_kv_modify(array_t* arr, char* key, char* value)//数组的mod操作
{
	if (arr == NULL||key == NULL || value == NULL) return -1;
	int i = 0;

	for (i = 0; i < arr->array_idx; ++i)
	{
		if (strcmp(key,arr->array_table[i].key) == 0)
		{
			free((void*)arr->array_table[i].value);
			arr->array_table[i].value = NULL;

			char* vcopy = (char*)malloc(strlen(value) + 1);
			strcpy(vcopy, value);
			arr->array_table[i].value = vcopy;
			return 0;
		}
	}
	return i;
}
int array_kv_count(array_t* arr)//数组的count操作
{
	if (!arr)return -1;
	return arr->array_idx;
}