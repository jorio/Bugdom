#pragma once

typedef struct Pool Pool;

// Creates an index pool on the heap.
// capacity: number of indices to manage.
// All indices in the pool are free to use after initialization.
Pool* Pool_New(int capacity);

// Disposes of an index pool.
void Pool_Free(Pool* pool);

// Returns the amount of indices currently in use.
int Pool_Size(const Pool* pool);

// Returns true if no indices are in use.
int Pool_Empty(const Pool* pool);

// Grabs a free index, marks it as used, and returns it.
// Returns -1 if there are no free indices left.
int Pool_AllocateIndex(Pool* pool);

// Frees up an index for use again.
void Pool_ReleaseIndex(Pool* pool, int index);

// Frees up all indices for use again.
void Pool_Reset(Pool* pool);

// Returns the first index in use, or -1 if all indices are free.
int Pool_First(const Pool* pool);

// Returns the last index in use, or -1 if all indices are free.
int Pool_Last(const Pool* pool);

// Returns the predecessor of an index, or -1 if that was the first index.
// You may call this on free or in-use indices:
// if the index is in use, Pool_Prev returns the previous index in use;
// if the index is free, Pool_Prev returns the previous free index.
int Pool_Prev(const Pool* pool, int index);

// Returns the successor of an index, or -1 if that was the last index.
// You may call this on free or in-use indices:
// if the index is in use, Pool_Next returns the next index in use;
// if the index is free, Pool_Next returns the next free index.
int Pool_Next(const Pool* pool, int index);

// Returns true if the given index is in use.
int Pool_IsUsed(const Pool* pool, int index);

// Aborts the program if the pool is in an inconsistent state.
void Pool_TestConsistency(const Pool* pool);

