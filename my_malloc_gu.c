
#include <stdio.h>
#include <stdlib.h>
#include "kvstore.h"
#define MEM_PAGE_SIZE 8192



int mp_init(mempool_t *m,int size)
{
	if (!m)return -1;
	if (size < 16)size=16;
	m->block_size = size;

	m->mem = (char*)malloc(MEM_PAGE_SIZE);
	if (!m->mem)return -1;
	m->free_ptr = m->mem;
	m->free_count = MEM_PAGE_SIZE / size;

	char* ptr = m->free_ptr;
	for (int i = 0; i < m->free_count-1; ++i)
	{
		*(char**)ptr = ptr + size;//ÄÚ´æ³Ø¿ÕÏÐÁ´±í¾­µäÐ´·¨
		ptr += size;
	}
	*(char**)ptr = NULL;//Á´±íÎ²²¿ÖÃ¿Õ
	return 0;
}


void mp_dest(mempool_t*m)
{
	if (!m || !m->mem)return;

	free(m->mem);
}

void *mp_alloc(mempool_t *m)
{
#if 1
	if (!m) {
		printf("mp_alloc: m is NULL\n");
		return NULL;
	}
	if (m->free_count == 0) {
		printf("mp_alloc: pool exhausted\n");
		return NULL;
	}
	if (m->free_ptr == NULL) {
		printf("mp_alloc: free_ptr is NULL but free_count=%d\n", m->free_count);
		return NULL;
	}

	void* ptr = m->free_ptr;
	m->free_ptr = *(char**)ptr;
	m->free_count--;
	if (m->free_ptr == NULL) {
		printf("mp_alloc: free_ptr is NULL but free_count=%d\n", m->free_count);
		return NULL;
	}
	
	return ptr;
#else
	if (!m || m->free_count == 0)return NULL;

	void* ptr = m->free_ptr;
	m->free_ptr = *(char**)ptr;
	m->free_count--;
	return ptr;
#endif
}

void mp_free(mempool_t *m,void* ptr)
{
	if (!m || !ptr)return ;
	*(char**)ptr = m->free_ptr;
	m->free_ptr = (char*)ptr;
	m->free_count++;
}
mempool_t m;
