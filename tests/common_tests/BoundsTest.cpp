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

#include <cppunit/extensions/HelperMacros.h>

#include "../../source/common/bv/Bounds.h"
#include "test_utils.h"

class idBoundsTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( idBoundsTest );
	CPPUNIT_TEST( TestConstructor );
	CPPUNIT_TEST( TestOperatorIndex );
	CPPUNIT_TEST_SUITE_END();

public:
	virtual void setUp();

	void TestConstructor();
	void TestOperatorIndex();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( idBoundsTest );

void idBoundsTest::setUp() {
	idMath::Init();
}

void idBoundsTest::TestConstructor() {
	idVec3 v1( -1.0f, -2.0f, -3.0f );
	idVec3 v2( 1.0f, 2.0f, 3.0f );
	idBounds b( v1, v2 );
	CPPUNIT_ASSERT_EQUAL( v1, b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( v2, b[ 1 ] );
}

void idBoundsTest::TestOperatorIndex() {
	idVec3 v1( -1.0f, -2.0f, -3.0f );
	idVec3 v2( 1.0f, 2.0f, 3.0f );
	idBounds b;
	b[ 0 ] = v1;
	b[ 1 ] = v2;
	CPPUNIT_ASSERT_EQUAL( v1, b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( v2, b[ 1 ] );
}
