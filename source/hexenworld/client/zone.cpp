// Z_zone.c

#include "quakedef.h"

#define DYNAMIC_SIZE    0x40000

#define ZONEID  0x1d4a11
#define MINFRAGMENT 64

typedef struct memblock_s
{
	int size;				// including the header and possibly tiny fragments
	int tag;				// a tag of 0 is a free block
	int id;					// should be ZONEID
	struct memblock_s* next, * prev;
	int pad;				// pad to 64 bit boundary
} memblock_t;

typedef struct
{
	int size;			// total bytes malloced, including header
	memblock_t blocklist;		// start / end cap for linked list
	memblock_t* rover;
} memzone_t;

/*
==============================================================================

                        ZONE MEMORY ALLOCATION

There is never any space between memblocks, and there will never be two
contiguous free memblocks.

The rover can be left pointing at a non-empty block

The zone calls are pretty much only used for small strings and structures,
all big things are allocated on the hunk.
==============================================================================
*/

memzone_t* mainzone;

void Z_ClearZone(memzone_t* zone, int size);


/*
========================
Z_ClearZone
========================
*/
void Z_ClearZone(memzone_t* zone, int size)
{
	memblock_t* block;

// set the entire zone to one free block

	zone->blocklist.next = zone->blocklist.prev = block =
													  (memblock_t*)((byte*)zone + sizeof(memzone_t));
	zone->blocklist.tag = 1;	// in use block
	zone->blocklist.id = 0;
	zone->blocklist.size = 0;
	zone->rover = block;

	block->prev = block->next = &zone->blocklist;
	block->tag = 0;			// free block
	block->id = ZONEID;
	block->size = size - sizeof(memzone_t);
}


/*
========================
Z_Free
========================
*/
void Z_Free(void* ptr)
{
	memblock_t* block, * other;

	if (!ptr)
	{
		common->FatalError("Z_Free: NULL pointer");
	}

	block = (memblock_t*)((byte*)ptr - sizeof(memblock_t));
	if (block->id != ZONEID)
	{
		common->FatalError("Z_Free: freed a pointer without ZONEID");
	}
	if (block->tag == 0)
	{
		common->FatalError("Z_Free: freed a freed pointer");
	}

	block->tag = 0;		// mark as free

	other = block->prev;
	if (!other->tag)
	{	// merge with previous free block
		other->size += block->size;
		other->next = block->next;
		other->next->prev = other;
		if (block == mainzone->rover)
		{
			mainzone->rover = other;
		}
		block = other;
	}

	other = block->next;
	if (!other->tag)
	{	// merge the next free block onto the end
		block->size += other->size;
		block->next = other->next;
		block->next->prev = block;
		if (other == mainzone->rover)
		{
			mainzone->rover = block;
		}
	}
}


/*
========================
Z_Malloc
========================
*/
void* Z_Malloc(int size)
{
	void* buf;

	Z_CheckHeap();	// DEBUG
	buf = Z_TagMalloc(size, 1);
	if (!buf)
	{
		common->FatalError("Z_Malloc: failed on allocation of %i bytes",size);
	}
	Com_Memset(buf, 0, size);

	return buf;
}

void* Z_TagMalloc(int size, int tag)
{
	int extra;
	memblock_t* start, * rover, * newb, * base;

	if (!tag)
	{
		common->FatalError("Z_TagMalloc: tried to use a 0 tag");
	}

//
// scan through the block list looking for the first free block
// of sufficient size
//
	size += sizeof(memblock_t);	// account for size of block header
	size += 4;					// space for memory trash tester
	size = (size + 7) & ~7;		// align to 8-byte boundary

	base = rover = mainzone->rover;
	start = base->prev;

	do
	{
		if (rover == start)	// scaned all the way around the list
		{
			return NULL;
		}
		if (rover->tag)
		{
			base = rover = rover->next;
		}
		else
		{
			rover = rover->next;
		}
	}
	while (base->tag || base->size < size);

//
// found a block big enough
//
	extra = base->size - size;
	if (extra >  MINFRAGMENT)
	{	// there will be a free fragment after the allocated block
		newb = (memblock_t*)((byte*)base + size);
		newb->size = extra;
		newb->tag = 0;			// free block
		newb->prev = base;
		newb->id = ZONEID;
		newb->next = base->next;
		newb->next->prev = newb;
		base->next = newb;
		base->size = size;
	}

	base->tag = tag;				// no longer a free block

	mainzone->rover = base->next;	// next allocation will start looking here

	base->id = ZONEID;

// marker for memory trash testing
	*(int*)((byte*)base + base->size - 4) = ZONEID;

	return (void*)((byte*)base + sizeof(memblock_t));
}


/*
========================
Z_Print
========================
*/
void Z_Print(memzone_t* zone)
{
	memblock_t* block;

	common->Printf("zone size: %i  location: %p\n",mainzone->size,mainzone);

	for (block = zone->blocklist.next;; block = block->next)
	{
		common->Printf("block:%p    size:%7i    tag:%3i\n",
			block, block->size, block->tag);

		if (block->next == &zone->blocklist)
		{
			break;			// all blocks have been hit
		}
		if ((byte*)block + block->size != (byte*)block->next)
		{
			common->Printf("ERROR: block size does not touch the next block\n");
		}
		if (block->next->prev != block)
		{
			common->Printf("ERROR: next block doesn't have proper back link\n");
		}
		if (!block->tag && !block->next->tag)
		{
			common->Printf("ERROR: two consecutive free blocks\n");
		}
	}
}


/*
========================
Z_CheckHeap
========================
*/
void Z_CheckHeap(void)
{
	memblock_t* block;

	for (block = mainzone->blocklist.next;; block = block->next)
	{
		if (block->next == &mainzone->blocklist)
		{
			break;			// all blocks have been hit
		}
		if ((byte*)block + block->size != (byte*)block->next)
		{
			common->FatalError("Z_CheckHeap: block size does not touch the next block\n");
		}
		if (block->next->prev != block)
		{
			common->FatalError("Z_CheckHeap: next block doesn't have proper back link\n");
		}
		if (!block->tag && !block->next->tag)
		{
			common->FatalError("Z_CheckHeap: two consecutive free blocks\n");
		}
	}
}

//============================================================================

#define HUNK_SENTINAL   0x1df001ed

typedef struct
{
	int sentinal;
	int size;			// including sizeof(hunk_t), -1 = not allocated
	char name[8];
} hunk_t;

byte* hunk_base;
int hunk_size;

int hunk_low_used;
int hunk_high_used;

qboolean hunk_tempactive;
int hunk_tempmark;

void R_FreeTextures(void);

/*
==============
Hunk_Check

Run consistancy and sentinal trahing checks
==============
*/
void Hunk_Check(void)
{
	hunk_t* h;

	for (h = (hunk_t*)hunk_base; (byte*)h != hunk_base + hunk_low_used; )
	{
		if (h->sentinal != HUNK_SENTINAL)
		{
			common->FatalError("Hunk_Check: trahsed sentinal");
		}
		if (h->size < 16 || h->size + (byte*)h - hunk_base > hunk_size)
		{
			common->FatalError("Hunk_Check: bad size");
		}
		h = (hunk_t*)((byte*)h + h->size);
	}
}

/*
==============
Hunk_Print

If "all" is specified, every single allocation is printed.
Otherwise, allocations with the same name will be totaled up before printing.
==============
*/
void Hunk_Print(qboolean all)
{
	hunk_t* h, * next, * endlow, * starthigh, * endhigh;
	int count, sum;
	int totalblocks;
	char name[9];

	name[8] = 0;
	count = 0;
	sum = 0;
	totalblocks = 0;

	h = (hunk_t*)hunk_base;
	endlow = (hunk_t*)(hunk_base + hunk_low_used);
	starthigh = (hunk_t*)(hunk_base + hunk_size - hunk_high_used);
	endhigh = (hunk_t*)(hunk_base + hunk_size);

	common->Printf("          :%8i total hunk size\n", hunk_size);
	common->Printf("-------------------------\n");

	while (1)
	{
		//
		// skip to the high hunk if done with low hunk
		//
		if (h == endlow)
		{
			common->Printf("-------------------------\n");
			common->Printf("          :%8i REMAINING\n", hunk_size - hunk_low_used - hunk_high_used);
			common->Printf("-------------------------\n");
			h = starthigh;
		}

		//
		// if totally done, break
		//
		if (h == endhigh)
		{
			break;
		}

		//
		// run consistancy checks
		//
		if (h->sentinal != HUNK_SENTINAL)
		{
			common->FatalError("Hunk_Check: trahsed sentinal");
		}
		if (h->size < 16 || h->size + (byte*)h - hunk_base > hunk_size)
		{
			common->FatalError("Hunk_Check: bad size");
		}

		next = (hunk_t*)((byte*)h + h->size);
		count++;
		totalblocks++;
		sum += h->size;

		//
		// print the single block
		//
		Com_Memcpy(name, h->name, 8);
		if (all)
		{
			common->Printf("%8p :%8i %8s\n",h, h->size, name);
		}

		//
		// print the total
		//
		if (next == endlow || next == endhigh ||
			String::NCmp(h->name, next->name, 8))
		{
			if (!all)
			{
				common->Printf("          :%8i %8s (TOTAL)\n",sum, name);
			}
			count = 0;
			sum = 0;
		}

		h = next;
	}

	common->Printf("-------------------------\n");
	common->Printf("%8i total blocks\n", totalblocks);

}

/*
===================
Hunk_AllocName
===================
*/
void* Hunk_AllocName(int size, const char* name)
{
	hunk_t* h;

#ifdef PARANOID
	Hunk_Check();
#endif

	if (size < 0)
	{
		common->FatalError("Hunk_Alloc: bad size: %i", size);
	}

	size = sizeof(hunk_t) + ((size + 15) & ~15);

	if (hunk_size - hunk_low_used - hunk_high_used < size)
//		common->FatalError ("Hunk_Alloc: failed on %i bytes",size);
#ifdef _WIN32
	{ common->FatalError("Not enough RAM allocated.  Try starting using \"-heapsize 16000\" on the HexenWorld command line."); }
#else
	{ common->FatalError("Not enough RAM allocated.  Try starting using \"-mem 16\" on the HexenWorld command line."); }
#endif

	h = (hunk_t*)(hunk_base + hunk_low_used);
	hunk_low_used += size;

	Com_Memset(h, 0, size);

	h->size = size;
	h->sentinal = HUNK_SENTINAL;
	String::NCpy(h->name, name, 8);

	return (void*)(h + 1);
}

/*
===================
Hunk_Alloc
===================
*/
void* Hunk_Alloc(int size)
{
	return Hunk_AllocName(size, "unknown");
}

int Hunk_LowMark(void)
{
	return hunk_low_used;
}

void Hunk_FreeToLowMark(int mark)
{
	if (mark < 0 || mark > hunk_low_used)
	{
		common->FatalError("Hunk_FreeToLowMark: bad mark %i", mark);
	}
	Com_Memset(hunk_base + mark, 0, hunk_low_used - mark);
	hunk_low_used = mark;
}

int Hunk_HighMark(void)
{
	if (hunk_tempactive)
	{
		hunk_tempactive = false;
		Hunk_FreeToHighMark(hunk_tempmark);
	}

	return hunk_high_used;
}

void Hunk_FreeToHighMark(int mark)
{
	if (hunk_tempactive)
	{
		hunk_tempactive = false;
		Hunk_FreeToHighMark(hunk_tempmark);
	}
	if (mark < 0 || mark > hunk_high_used)
	{
		common->FatalError("Hunk_FreeToHighMark: bad mark %i", mark);
	}
	Com_Memset(hunk_base + hunk_size - hunk_high_used, 0, hunk_high_used - mark);
	hunk_high_used = mark;
}


/*
===================
Hunk_HighAllocName
===================
*/
void* Hunk_HighAllocName(int size, const char* name)
{
	hunk_t* h;

	if (size < 0)
	{
		common->FatalError("Hunk_HighAllocName: bad size: %i", size);
	}

	if (hunk_tempactive)
	{
		Hunk_FreeToHighMark(hunk_tempmark);
		hunk_tempactive = false;
	}

#ifdef PARANOID
	Hunk_Check();
#endif

	size = sizeof(hunk_t) + ((size + 15) & ~15);

	if (hunk_size - hunk_low_used - hunk_high_used < size)
	{
		common->Printf("Hunk_HighAlloc: failed on %i bytes\n",size);
		return NULL;
	}

	hunk_high_used += size;

	h = (hunk_t*)(hunk_base + hunk_size - hunk_high_used);

	Com_Memset(h, 0, size);
	h->size = size;
	h->sentinal = HUNK_SENTINAL;
	String::NCpy(h->name, name, 8);

	return (void*)(h + 1);
}


/*
=================
Hunk_TempAlloc

Return space from the top of the hunk
=================
*/
void* Hunk_TempAlloc(int size)
{
	void* buf;

	size = (size + 15) & ~15;

	if (hunk_tempactive)
	{
		Hunk_FreeToHighMark(hunk_tempmark);
		hunk_tempactive = false;
	}

	hunk_tempmark = Hunk_HighMark();

	buf = Hunk_HighAllocName(size, "temp");

	hunk_tempactive = true;

	return buf;
}

//============================================================================


/*
========================
Memory_Init
========================
*/
void Memory_Init(void* buf, int size)
{
	int p;
	int zonesize = DYNAMIC_SIZE;

	hunk_base = (byte*)buf;
	hunk_size = size;
	hunk_low_used = 0;
	hunk_high_used = 0;

	p = COM_CheckParm("-zone");
	if (p)
	{
		if (p < COM_Argc() - 1)
		{
			zonesize = String::Atoi(COM_Argv(p + 1)) * 1024;
		}
		else
		{
			common->FatalError("Memory_Init: you must specify a size in KB after -zone");
		}
	}
	mainzone = (memzone_t*)Hunk_AllocName(zonesize, "zone");
	Z_ClearZone(mainzone, zonesize);
}
