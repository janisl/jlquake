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

#ifndef __idList__
#define __idList__

#include "../qcommon.h"

template<class type> inline int idListSortCompare( const type* a, const type* b ) {
	return *a - *b;
}

template<class type> inline type* idListNewElement() {
	return new type;
}

template<class type> inline void idSwap( type& a, type& b ) {
	type c = a;
	a = b;
	b = c;
}

//	List template
//	Does not allocate memory until the first item is added.
template<class type> class idList {
public:
	typedef int cmp_t ( const type *, const type* );
	typedef type new_t ( );

	idList( int newGranularity = 16 );
	idList( const idList<type>& other );
	~idList();

	void Clear();											// clear the list
	int Num() const;										// returns number of elements in list
	int NumAllocated() const;								// returns number of elements allocated for
	void SetGranularity( int newGranularity );					// set new granularity
	int GetGranularity() const;								// get the current granularity

	size_t Allocated() const;								// returns total size of allocated memory
	size_t Size() const;									// returns total size of allocated memory including size of list type
	size_t MemoryUsed() const;								// returns size of the used elements in the list

	idList<type>& operator=( const idList<type>& other );
	const type& operator[]( int index ) const;
	type& operator[]( int index );

	void Condense();										// resizes list to exactly the number of elements it contains
	void Resize( int newSize );									// resizes list to the given number of elements
	void SetNum( int newNum, bool resize = true );				// set number of elements in list and resize to exactly this number if necessary
	void AssureSize( int newSize );								// assure list has given number of elements, but leave them uninitialized
	void AssureSize( int newSize, const type& initValue );		// assure list has given number of elements and initialize any new elements
	void AssureSizeAlloc( int newSize, new_t* allocator );		// assure the pointer list has the given number of elements and allocate any new elements

	type* Ptr();											// returns a pointer to the list
	const type* Ptr() const;								// returns a pointer to the list
	type& Alloc();											// returns reference to a new data element at the end of the list
	int Append( const type& obj );								// append element
	int Append( const idList<type>& other );					// append list
	int AddUnique( const type& obj );							// add unique element
	int Insert( const type& obj, int index = 0 );				// insert the element at the given index
	int FindIndex( const type& obj ) const;						// find the index for the given element
	type* Find( type const& obj ) const;						// find pointer to the given element
	int FindNull() const;									// find the index for the first NULL pointer in the list
	int IndexOf( const type* obj ) const;						// returns the index for the pointer to an element in the list
	bool RemoveIndex( int index );								// remove the element at the given index
	bool Remove( const type& obj );								// remove the element
	void Sort( cmp_t* compare = & idListSortCompare<type>);
	void SortSubSection( int startIndex, int endIndex, cmp_t* compare = & idListSortCompare<type>);
	void Swap( idList<type>& other );							// swap the contents of the lists
	void DeleteContents( bool clear );							// delete the contents of the list

private:
	int num;
	int size;
	int granularity;
	type* list;
};

template<class type> inline idList<type>::idList( int newGranularity )
	: num( 0 ),
	size( 0 ),
	granularity( newGranularity ),
	list( NULL ) {
	assert( newGranularity > 0 );
}

template<class type> inline idList<type>::idList( const idList<type>& other )
	: num( 0 ),
	size( 0 ),
	granularity( 16 ),
	list( NULL ) {
	*this = other;
}

template<class type> inline idList<type>::~idList() {
	Clear();
}

//	Frees up the memory allocated by the list.  Assumes that type automatically handles freeing up memory.
template<class type> inline void idList<type>::Clear() {
	if ( list ) {
		delete[] list;
	}
	list = NULL;
	num = 0;
	size = 0;
}

//	Returns the number of elements currently contained in the list.
// Note that this is NOT an indication of the memory allocated.
template<class type> inline int idList<type>::Num() const {
	return num;
}

//	Returns the number of elements currently allocated for.
template<class type> inline int idList<type>::NumAllocated() const {
	return size;
}

//	Sets the base size of the array and resizes the array to match.
template<class type> inline void idList<type>::SetGranularity( int newGranularity ) {
	assert( newGranularity > 0 );
	granularity = newGranularity;

	if ( list ) {
		// resize it to the closest level of granularity
		int newSize = num + granularity - 1;
		newSize -= newSize % granularity;
		if ( newSize != size ) {
			Resize( newSize );
		}
	}
}

//	Get the current granularity.
template<class type> inline int idList<type>::GetGranularity() const {
	return granularity;
}

//	return total memory allocated for the list in bytes, but doesn't take into account additional memory allocated by type
template<class type> inline size_t idList<type>::Allocated() const {
	return size * sizeof ( type );
}

//	return total size of list in bytes, but doesn't take into account additional memory allocated by type
template<class type> inline size_t idList<type>::Size() const {
	return sizeof ( idList<type>) + Allocated();
}

template<class type> inline size_t idList<type>::MemoryUsed() const {
	return num * sizeof ( type );
}

//	Copies the contents and size attributes of another list.
template<class type> inline idList<type>& idList<type>::operator=( const idList<type>& other ) {
	Clear();

	num = other.num;
	size = other.size;
	granularity = other.granularity;

	if ( size ) {
		list = new type[ size ];
		for ( int i = 0; i < num; i++ ) {
			list[ i ] = other.list[ i ];
		}
	}

	return *this;
}

//	Access operator.  Index must be within range or an assert will be issued in debug builds.
// Release builds do no range checking.
template<class type> inline const type& idList<type>::operator[]( int index ) const {
	assert( index >= 0 );
	assert( index < num );

	return list[ index ];
}

//	Access operator.  Index must be within range or an assert will be issued in debug builds.
// Release builds do no range checking.
template<class type> inline type& idList<type>::operator[]( int index ) {
	assert( index >= 0 );
	assert( index < num );

	return list[ index ];
}

//	Resizes the array to exactly the number of elements it contains or frees up memory if empty.
template<class type> inline void idList<type>::Condense() {
	if ( list ) {
		if ( num ) {
			Resize( num );
		} else {
			Clear();
		}
	}
}

//	Allocates memory for the amount of elements requested while keeping the contents intact.
// Contents are copied using their = operator so that data is correnctly instantiated.
template<class type> inline void idList<type>::Resize( int newSize ) {
	assert( newSize >= 0 );

	// free up the list if no data is being reserved
	if ( newSize <= 0 ) {
		Clear();
		return;
	}

	if ( newSize == size ) {
		// not changing the size, so just exit
		return;
	}

	type* temp = list;
	size = newSize;
	if ( size < num ) {
		num = size;
	}

	// copy the old list into our new one
	list = new type[ size ];
	for ( int i = 0; i < num; i++ ) {
		list[ i ] = temp[ i ];
	}

	// delete the old list if it exists
	if ( temp ) {
		delete[] temp;
	}
}

//	Resize to the exact size specified irregardless of granularity
template<class type> inline void idList<type>::SetNum( int newNum, bool resize ) {
	assert( newNum >= 0 );
	if ( resize || newNum > size ) {
		Resize( newNum );
	}
	num = newNum;
}

//	Makes sure the list has at least the given number of elements.
template<class type> inline void idList<type>::AssureSize( int newSize ) {
	int newNum = newSize;

	if ( newSize > size ) {
		assert( granularity > 0 );

		newSize += granularity - 1;
		newSize -= newSize % granularity;
		Resize( newSize );
	}

	num = newNum;
}

//	Makes sure the list has at least the given number of elements and initialize any elements not yet initialized.
template<class type> inline void idList<type>::AssureSize( int newSize, const type& initValue ) {
	int newNum = newSize;

	if ( newSize > size ) {
		assert( granularity > 0 );

		newSize += granularity - 1;
		newSize -= newSize % granularity;
		num = size;
		Resize( newSize );

		for ( int i = num; i < newSize; i++ ) {
			list[ i ] = initValue;
		}
	}

	num = newNum;
}

//	Makes sure the list has at least the given number of elements and allocates any elements using the allocator.
//	NOTE: This function can only be called on lists containing pointers. Calling it
// on non-pointer lists will cause a compiler error.
template<class type> inline void idList<type>::AssureSizeAlloc( int newSize, new_t* allocator ) {
	int newNum = newSize;

	if ( newSize > size ) {
		assert( granularity > 0 );

		newSize += granularity - 1;
		newSize -= newSize % granularity;
		num = size;
		Resize( newSize );

		for ( int i = num; i < newSize; i++ ) {
			list[ i ] = ( *allocator )( );
		}
	}

	num = newNum;
}

//	Returns a pointer to the begining of the array.  Useful for iterating through the list in loops.
//	Note: may return NULL if the list is empty.
//	FIXME: Create an iterator template for this kind of thing.
template<class type> inline type* idList<type>::Ptr() {
	return list;
}

template<class type> inline const type* idList<type>::Ptr() const {
	return list;
}

//	Returns a reference to a new data element at the end of the list.
template<class type> inline type& idList<type>::Alloc() {
	if ( !list ) {
		Resize( granularity );
	}

	if ( num == size ) {
		Resize( size + granularity );
	}

	return list[ num++ ];
}

//	Increases the size of the list by one element and copies the supplied data into it.
//	Returns the index of the new element.
template<class type> inline int idList<type>::Append( type const& obj ) {
	if ( !list ) {
		Resize( granularity );
	}

	if ( num == size ) {
		assert( granularity > 0 );
		int newsize = size + granularity;
		Resize( newsize - newsize % granularity );
	}

	list[ num ] = obj;
	num++;

	return num - 1;
}

//	adds the other list to this one
//	Returns the size of the new combined list
template<class type> inline int idList<type>::Append( const idList<type>& other ) {
	int n = other.Num();
	for ( int i = 0; i < n; i++ ) {
		Append( other[ i ] );
	}

	return Num();
}

//	Increases the size of the list by at leat one element if necessary
// and inserts the supplied data into it.
//	Returns the index of the new element.
template<class type> inline int idList<type>::Insert( type const& obj, int index ) {
	if ( !list ) {
		Resize( granularity );
	}

	if ( num == size ) {
		assert( granularity > 0 );
		int newSize = size + granularity;
		Resize( newSize - newSize % granularity );
	}

	if ( index < 0 ) {
		index = 0;
	} else if ( index > num ) {
		index = num;
	}
	for ( int i = num; i > index; --i ) {
		list[ i ] = list[ i - 1 ];
	}
	num++;
	list[ index ] = obj;
	return index;
}

//	Adds the data to the list if it doesn't already exist.  Returns the index of the data in the list.
template<class type> inline int idList<type>::AddUnique( type const& obj ) {
	int index = FindIndex( obj );
	if ( index < 0 ) {
		index = Append( obj );
	}

	return index;
}

//	Searches for the specified data in the list and returns it's index.  Returns -1 if the data is not found.
template<class type> inline int idList<type>::FindIndex( type const& obj ) const {
	for ( int i = 0; i < num; i++ ) {
		if ( list[ i ] == obj ) {
			return i;
		}
	}

	// Not found
	return -1;
}

//	Searches for the specified data in the list and returns it's address. Returns NULL if the data is not found.
template<class type> inline type* idList<type>::Find( type const& obj ) const {
	int i = FindIndex( obj );
	if ( i >= 0 ) {
		return &list[ i ];
	}

	return NULL;
}

//	Searches for a NULL pointer in the list.  Returns -1 if NULL is not found.
//	NOTE: This function can only be called on lists containing pointers. Calling it
// on non-pointer lists will cause a compiler error.
template<class type> inline int idList<type>::FindNull() const {
	for ( int i = 0; i < num; i++ ) {
		if ( list[ i ] == NULL ) {
			return i;
		}
	}

	// Not found
	return -1;
}

//	Takes a pointer to an element in the list and returns the index of the element.
//	This is NOT a guarantee that the object is really in the list.
//	Function will assert in debug builds if pointer is outside the bounds of the list,
// but remains silent in release builds.
template<class type> inline int idList<type>::IndexOf( type const* objptr ) const {
	int index = objptr - list;

	assert( index >= 0 );
	assert( index < num );

	return index;
}

//	Removes the element at the specified index and moves all data following the element down to fill in the gap.
// The number of elements in the list is reduced by one.  Returns false if the index is outside the bounds of the list.
// Note that the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
template<class type> inline bool idList<type>::RemoveIndex( int index ) {
	assert( list != NULL );
	assert( index >= 0 );
	assert( index < num );

	if ( ( index < 0 ) || ( index >= num ) ) {
		return false;
	}

	num--;
	for ( int i = index; i < num; i++ ) {
		list[ i ] = list[ i + 1 ];
	}

	return true;
}

//	Removes the element if it is found within the list and moves all data following the element down to fill in the gap.
// The number of elements in the list is reduced by one.  Returns false if the data is not found in the list.  Note that
// the element is not destroyed, so any memory used by it may not be freed until the destruction of the list.
template<class type> inline bool idList<type>::Remove( type const& obj ) {
	int index = FindIndex( obj );
	if ( index >= 0 ) {
		return RemoveIndex( index );
	}

	return false;
}

//	Performs a qsort on the list using the supplied comparison function.  Note that the data is merely moved around the
// list, so any pointers to data within the list may no longer be valid.
template<class type> inline void idList<type>::Sort( cmp_t* compare ) {
	if ( !list ) {
		return;
	}
	typedef int cmp_c ( const void*, const void* );

	cmp_c* vCompare = reinterpret_cast<cmp_c*>( compare );
	qsort( list, static_cast<size_t>( num ), sizeof ( type ), vCompare );
}

//	Sorts a subsection of the list.
template<class type> inline void idList<type>::SortSubSection( int startIndex, int endIndex, cmp_t* compare ) {
	if ( !list ) {
		return;
	}
	if ( startIndex < 0 ) {
		startIndex = 0;
	}
	if ( endIndex >= num ) {
		endIndex = num - 1;
	}
	if ( startIndex >= endIndex ) {
		return;
	}
	typedef int cmp_c ( const void*, const void* );

	cmp_c* vCompare = reinterpret_cast<cmp_c*>( compare );
	qsort( &list[ startIndex ], static_cast<size_t>( endIndex - startIndex + 1 ), sizeof ( type ), vCompare );
}

//	Swaps the contents of two lists
template<class type> inline void idList<type>::Swap( idList<type>& other ) {
	idSwap( num, other.num );
	idSwap( size, other.size );
	idSwap( granularity, other.granularity );
	idSwap( list, other.list );
}

//	Calls the destructor of all elements in the list.  Conditionally frees up memory used by the list.
//	Note that this only works on lists containing pointers to objects and will cause a compiler error
// if called with non-pointers.  Since the list was not responsible for allocating the object, it has
// no information on whether the object still exists or not, so care must be taken to ensure that
// the pointers are still valid when this function is called.  Function will set all pointers in the
// list to NULL.
template<class type> inline void idList<type>::DeleteContents( bool clear ) {
	for ( int i = 0; i < num; i++ ) {
		delete list[ i ];
		list[ i ] = NULL;
	}

	if ( clear ) {
		Clear();
	} else {
		memset( list, 0, size * sizeof ( type ) );
	}
}

#endif
