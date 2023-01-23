// POOL.C
// (C)2023 Iliyas Jorio

#include "game.h"

struct PoolList
{
	int head;
	int tail;
};

struct PoolNode
{
	int prev;
	int next;
};

struct Pool
{
	int capacity;
	int allocated;
	struct PoolList freeList;
	struct PoolList usedList;
	struct PoolNode* nodes;
	bool* isUsed;
};

static void PoolList_Unlink(struct PoolList *list, struct PoolNode *nodes, int index)
{
	GAME_ASSERT(index >= 0);

	struct PoolNode old = nodes[index];

	if (old.next >= 0)
		nodes[old.next].prev = old.prev;

	if (old.prev >= 0)
		nodes[old.prev].next = old.next;

	if (list->head == index)
		list->head = old.next;

	if (list->tail == index)
		list->tail = old.prev;

	nodes[index].prev = -1;
	nodes[index].next = -1;
}

static void PoolList_Insert(struct PoolList *list, struct PoolNode *nodes, int newIndex, int after)
{
	if (after < 0)
	{
		GAME_ASSERT(list->head < 0);				// if `after` negative, assume list empty
		GAME_ASSERT(list->tail < 0);
		GAME_ASSERT(nodes[newIndex].prev < 0);		// node must be sole item in list
		GAME_ASSERT(nodes[newIndex].next < 0);

		list->head = newIndex;
		list->tail = newIndex;
	}
	else
	{
		GAME_ASSERT(list->head >= 0);				// list cannot be empty
		GAME_ASSERT(list->tail >= 0);

		int oldNext = nodes[after].next;

		nodes[after].next = newIndex;

		nodes[newIndex].prev = after;
		nodes[newIndex].next = oldNext;

		if (oldNext >= 0)
		{
			GAME_ASSERT(list->tail != after);
			nodes[oldNext].prev = newIndex;
		}

		if (list->tail == after)
		{
			GAME_ASSERT(oldNext < 0);
			list->tail = newIndex;
		}
	}
}

Pool* Pool_New(int capacity)
{
	Pool* pool = (Pool*) NewPtrClear(sizeof(struct Pool));

	pool->capacity = capacity;
	pool->nodes = (struct PoolNode*) NewPtrClear(capacity * sizeof(struct PoolNode));
	pool->isUsed = (bool*) NewPtrClear(capacity * sizeof(bool));

	// Make all indices available
	Pool_Reset(pool);

	return pool;
}

void Pool_Free(Pool* pool)
{
	if (pool)
	{
		DisposePtr((Ptr) pool->nodes);
		DisposePtr((Ptr) pool->isUsed);
		DisposePtr((Ptr) pool);
	}
}

void Pool_Reset(Pool* pool)
{
	const int n = pool->capacity;

	pool->allocated = 0;

	for (int i = 0; i < n; i++)
	{
		pool->nodes[i].prev = i - 1;
		pool->nodes[i].next = i + 1;
		pool->isUsed[i] = 0;
	}

	pool->nodes[n - 1].next = -1;		// last node has no successor

	pool->freeList.head = 0;			// all nodes start free
	pool->freeList.tail = n - 1;

	pool->usedList.head = -1;			// nothing in use yet
	pool->usedList.tail = -1;

	Pool_TestConsistency(pool);
}

int Pool_Size(const Pool* pool)
{
	return pool->allocated;
}

int Pool_Empty(const Pool* pool)
{
	return 0 == pool->allocated;
}

int Pool_First(const Pool* pool)
{
	return pool->usedList.head;
}

int Pool_Last(const Pool* pool)
{
	return pool->usedList.tail;
}

int Pool_Prev(const Pool* pool, int index)
{
	GAME_ASSERT(index >= 0);
	GAME_ASSERT(index < pool->capacity);
	return pool->nodes[index].prev;
}

int Pool_Next(const Pool* pool, int index)
{
	GAME_ASSERT(index >= 0);
	GAME_ASSERT(index < pool->capacity);
	return pool->nodes[index].next;
}

int Pool_IsUsed(const Pool* pool, int index)
{
	return pool->isUsed[index];
}

int Pool_AllocateIndex(Pool* pool)
{
	if (pool->allocated >= pool->capacity)
	{
		return -1;
	}

	// Pop head of "free" list
	int newIndex = pool->freeList.head;
	GAME_ASSERT(newIndex >= 0);

	PoolList_Unlink(&pool->freeList, pool->nodes, newIndex);
	PoolList_Insert(&pool->usedList, pool->nodes, newIndex, pool->usedList.tail);

	GAME_ASSERT(pool->usedList.tail == newIndex);
	GAME_ASSERT(!pool->isUsed[newIndex]);

	pool->isUsed[newIndex] = true;
	pool->allocated++;

#if _DEBUG
	Pool_TestConsistency(pool);
#endif

	return newIndex;
}

void Pool_ReleaseIndex(Pool* pool, int index)
{
	GAME_ASSERT(index >= 0 && index < pool->capacity);
	GAME_ASSERT(pool->allocated > 0);
	GAME_ASSERT(pool->isUsed[index]);

	PoolList_Unlink(&pool->usedList, pool->nodes, index);
	PoolList_Insert(&pool->freeList, pool->nodes, index, pool->freeList.tail);

	pool->isUsed[index] = false;
	pool->allocated--;

	GAME_ASSERT(pool->freeList.tail == index);

#if _DEBUG
	Pool_TestConsistency(pool);
#endif
}

void Pool_TestConsistency(const Pool* pool)
{
	Byte* markers = (Byte*) NewPtrClear(pool->capacity);
	int n;

	// Walk free list
	for (int i = pool->freeList.head; i >= 0; i = pool->nodes[i].next)
	{
		GAME_ASSERT(!markers[i]);
		GAME_ASSERT(!pool->isUsed[i]);
		markers[i] = 1;
	}

	// Walk free list backwards
	for (int i = pool->freeList.tail; i >= 0; i = pool->nodes[i].prev)
	{
		GAME_ASSERT(markers[i] == 1);
	}

	// Walk used list
	n = 0;
	for (int i = pool->usedList.head; i >= 0; i = pool->nodes[i].next)
	{
		GAME_ASSERT(!markers[i]);
		GAME_ASSERT(pool->isUsed[i]);
		markers[i] = 2;
		n++;
	}
	GAME_ASSERT(n == pool->allocated);

	// Walk used list backwards
	n = 0;
	for (int i = pool->usedList.tail; i >= 0; i = pool->nodes[i].prev)
	{
		GAME_ASSERT(markers[i] == 2);
		n++;
	}
	GAME_ASSERT(n == pool->allocated);

	// Make sure we didn't miss any nodes
	for (int i = 0; i < pool->capacity; i++)
	{
		GAME_ASSERT(markers[i] != 0);
	}

	DisposePtr((Ptr) markers);
}
