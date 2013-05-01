//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************
//**
//**	Memory allocation
//**
//**	Mostly based on memory allocator of Doom 3.
//**
//**************************************************************************

// HEADER FILES ------------------------------------------------------------

#include "qcommon.h"
#include "Common.h"

// MACROS ------------------------------------------------------------------

#define SMALLID             0x22
#define LARGEID             0x33

// TYPES -------------------------------------------------------------------

enum
{
	ALIGN = 4
};

#define ALIGN_SIZE( bytes )   ( ( ( bytes ) + ALIGN - 1 ) & ~( ALIGN - 1 ) )

#define SMALL_HEADER_SIZE   ALIGN_SIZE( sizeof ( byte ) + sizeof ( byte ) )
#define LARGE_HEADER_SIZE   ALIGN_SIZE( sizeof ( void* ) + sizeof ( byte ) )

#define SMALL_ALIGN( bytes )  ( ALIGN_SIZE( ( bytes ) + SMALL_HEADER_SIZE ) - SMALL_HEADER_SIZE )

struct MemDebug_t {
	const char* FileName;
	int LineNumber;
	int Size;
	MemDebug_t* Prev;
	MemDebug_t* Next;
};

class QMemHeap {
public:
	QMemHeap();

	void* Alloc( size_t Bytes );
	void Free( void* Ptr );

private:
	struct QPage {
		void* Data;
		size_t Size;

		QPage* Prev;
		QPage* Next;
	};

	size_t PageSize;

	void* SmallFirstFree[ 256 / ALIGN + 1 ];
	QPage* SmallPage;
	size_t SmallOffset;
	QPage* SmallUsedPages;

	QPage* LargeFirstUsedPage;

	QPage* AllocPage( size_t Bytes );
	void FreePage( QPage* Page );

	void* SmallAlloc( size_t Bytes );
	void SmallFree( void* Ptr );

	void* LargeAlloc( size_t Bytes );
	void LargeFree( void* Ptr );
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

#ifdef MEM_DEBUG
static void Mem_MemDebugDump();
#endif

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static QMemHeap MainHeap;
#ifdef MEM_DEBUG
static MemDebug_t* MemDebug;
#endif

// CODE --------------------------------------------------------------------

//==========================================================================
//
//	QMemHeap::Init
//
//==========================================================================

QMemHeap::QMemHeap()
	: PageSize( 0 ),
	SmallPage( NULL ),
	SmallOffset( 0 ),
	SmallUsedPages( NULL ),
	LargeFirstUsedPage( NULL ) {
}

//==========================================================================
//
//	QMemHeap::Alloc
//
//==========================================================================

void* QMemHeap::Alloc( size_t Bytes ) {
	if ( Bytes < 256 ) {
		return SmallAlloc( Bytes );
	}
	return LargeAlloc( Bytes );
}

//==========================================================================
//
//	QMemHeap::Free
//
//==========================================================================

void QMemHeap::Free( void* Ptr ) {
	switch ( ( ( byte* )Ptr )[ -1 ] ) {
	case SMALLID:
		SmallFree( Ptr );
		break;
	case LARGEID:
		LargeFree( Ptr );
		break;
	default:
		common->FatalError( "Invalid memory block" );
	}
}

//==========================================================================
//
//	QMemHeap::AllocPage
//
//==========================================================================

QMemHeap::QPage* QMemHeap::AllocPage( size_t Bytes ) {
	size_t Size = Bytes + sizeof ( QPage );
	QPage* P = ( QPage* )::malloc( Size );
	if ( !P ) {
		//common->FatalError("Failed to allocate %d bytes", Bytes);
		common->FatalError( "Failed to allocate page" );
	}
	P->Data = P + 1;
	P->Size = Bytes;
	P->Prev = NULL;
	P->Next = NULL;
	return P;
}

//==========================================================================
//
//	QMemHeap::FreePage
//
//==========================================================================

void QMemHeap::FreePage( QMemHeap::QPage* Page ) {
	if ( !Page ) {
		common->FatalError( "Tried to free NULL page" );
	}
	::free( Page );
}

//==========================================================================
//
//	QMemHeap::SmallAlloc
//
//==========================================================================

void* QMemHeap::SmallAlloc( size_t Bytes ) {
	//	We need enough memory for the free list.
	if ( Bytes < sizeof ( void* ) ) {
		Bytes = sizeof ( void* );
	}

	if ( !PageSize ) {
		PageSize = 65536 - sizeof ( QPage );

		Com_Memset( SmallFirstFree, 0, sizeof ( SmallFirstFree ) );
		SmallPage = AllocPage( PageSize );
		SmallOffset = 0;
		SmallUsedPages = NULL;
	}

	//	Align the size.
	Bytes = SMALL_ALIGN( Bytes );
	byte* SmallBlock = ( byte* )SmallFirstFree[ Bytes / ALIGN ];
	if ( SmallBlock ) {
		byte* Ptr = SmallBlock + SMALL_HEADER_SIZE;
		SmallFirstFree[ Bytes / ALIGN ] = *( void** )Ptr;
		Ptr[ -1 ] = SMALLID;
		return Ptr;
	}

	size_t BytesLeft = PageSize - SmallOffset;
	if ( BytesLeft < Bytes + SMALL_HEADER_SIZE ) {
		//	Add current page to the used ones.
		SmallPage->Next = SmallUsedPages;
		SmallUsedPages = SmallPage;
		SmallPage = AllocPage( PageSize );
		SmallOffset = 0;
	}

	SmallBlock = ( byte* )SmallPage->Data + SmallOffset;
	byte* Ptr = SmallBlock + SMALL_HEADER_SIZE;
	SmallBlock[ 0 ] = ( byte )( Bytes / ALIGN );
	Ptr[ -1 ] = SMALLID;
	SmallOffset += Bytes + SMALL_HEADER_SIZE;
	return Ptr;
}

//==========================================================================
//
//	QMemHeap::SmallFree
//
//==========================================================================

void QMemHeap::SmallFree( void* Ptr ) {
	( ( byte* )Ptr )[ -1 ] = 0;

	byte* Block = ( byte* )Ptr - SMALL_HEADER_SIZE;
	size_t Idx = *Block;
	if ( Idx > 256 / ALIGN ) {
		common->FatalError( "Invalid small memory block size" );
	}

	*( ( void** )Ptr ) = SmallFirstFree[ Idx ];
	SmallFirstFree[ Idx ] = Block;
}

//==========================================================================
//
//	QMemHeap::LargeAlloc
//
//==========================================================================

void* QMemHeap::LargeAlloc( size_t Bytes ) {
	QPage* P = AllocPage( Bytes + LARGE_HEADER_SIZE );

	byte* Ptr = ( byte* )P->Data + LARGE_HEADER_SIZE;
	*( void** )P->Data = P;
	Ptr[ -1 ] = LARGEID;

	//	Link to 'large used page list'
	P->Prev = NULL;
	P->Next = LargeFirstUsedPage;
	if ( P->Next ) {
		P->Next->Prev = P;
	}
	LargeFirstUsedPage = P;

	return Ptr;
}

//==========================================================================
//
//	QMemHeap::LargeFree
//
//==========================================================================

void QMemHeap::LargeFree( void* Ptr ) {
	( ( byte* )Ptr )[ -1 ] = 0;

	//	Get page pointer
	QPage* P = ( QPage* )( *( ( void** )( ( ( byte* )Ptr ) - LARGE_HEADER_SIZE ) ) );

	//	Unlink from doubly linked list
	if ( P->Prev ) {
		P->Prev->Next = P->Next;
	}
	if ( P->Next ) {
		P->Next->Prev = P->Prev;
	}
	if ( P == LargeFirstUsedPage ) {
		LargeFirstUsedPage = P->Next;
	}
	P->Next = P->Prev = NULL;

	FreePage( P );
}

//==========================================================================
//
//  Mem_Shutdown
//
//==========================================================================

void Mem_Shutdown() {
#ifdef MEM_DEBUG
	Mem_MemDebugDump();
#endif
}

#ifdef MEM_DEBUG

#undef Mem_Alloc
#undef Mem_ClearedAlloc
#undef Mem_Free

//==========================================================================
//
//	Mem_Alloc
//
//==========================================================================

void* Mem_Alloc( int size, const char* FileName, int LineNumber ) {
	if ( !size ) {
		return NULL;
	}

	void* ptr = MainHeap.Alloc( size + sizeof ( MemDebug_t ) );
	if ( !ptr ) {
		//	It should always return valid pointer.
		common->FatalError( "MainHeap.Alloc failed" );
	}

	MemDebug_t* m = ( MemDebug_t* )ptr;
	m->FileName = FileName;
	m->LineNumber = LineNumber;
	m->Size = size;
	m->Next = MemDebug;
	if ( MemDebug ) {
		MemDebug->Prev = m;
	}
	MemDebug = m;

	return ( byte* )ptr + sizeof ( MemDebug_t );
}

//==========================================================================
//
//  Mem_ClearedAlloc
//
//==========================================================================

void* Mem_ClearedAlloc( int size, const char* FileName, int LineNumber ) {
	void* P = Mem_Alloc( size, FileName, LineNumber );
	Com_Memset( P, 0, size );
	return P;
}

//==========================================================================
//
//	Mem_Free
//
//==========================================================================

void Mem_Free( void* ptr, const char* FileName, int LineNumber ) {
	if ( !ptr ) {
		return;
	}

	//	Unlink debug info.
	MemDebug_t* m = ( MemDebug_t* )( ( char* )ptr - sizeof ( MemDebug_t ) );
	if ( m->Next ) {
		m->Next->Prev = m->Prev;
	}
	if ( m == MemDebug ) {
		MemDebug = m->Next;
	} else {
		m->Prev->Next = m->Next;
	}

	MainHeap.Free( ( char* )ptr - sizeof ( MemDebug_t ) );
}

//==========================================================================
//
//	Mem_MemDebugDump
//
//==========================================================================

static void Mem_MemDebugDump() {
	int NumBlocks = 0;
	for ( MemDebug_t* m = MemDebug; m; m = m->Next ) {
		common->Printf( "block %p size %8d at %s:%d\n", m + 1, m->Size,
			m->FileName, m->LineNumber );
		NumBlocks++;
	}
	common->Printf( "%d blocks allocated\n", NumBlocks );
}

#else

//==========================================================================
//
//	Mem_Alloc
//
//==========================================================================

void* Mem_Alloc( int size ) {
	if ( !size ) {
		return NULL;
	}

	void* ptr = MainHeap.Alloc( size );
	if ( !ptr ) {
		//	It should always return valid pointer.
		common->FatalError( "MainHeap.Alloc failed" );
	}
	return ptr;
}

//==========================================================================
//
//  Mem_ClearedAlloc
//
//==========================================================================

void* Mem_ClearedAlloc( int size ) {
	void* P = Mem_Alloc( size );
	Com_Memset( P, 0, size );
	return P;
}

//==========================================================================
//
//	Mem_Free
//
//==========================================================================

void Mem_Free( void* ptr ) {
	if ( !ptr ) {
		return;
	}

	MainHeap.Free( ptr );
}

#endif
