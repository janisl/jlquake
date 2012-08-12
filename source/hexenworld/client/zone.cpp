// Z_zone.c

#include "quakedef.h"

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
	hunk_base = (byte*)buf;
	hunk_size = size;
	hunk_low_used = 0;
	hunk_high_used = 0;
}
