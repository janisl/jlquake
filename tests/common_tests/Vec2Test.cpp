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

#include "../../source/common/math/Vec2.h"

class idVec2Test : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( idVec2Test );
	CPPUNIT_TEST( TestConstructor );
	CPPUNIT_TEST( TestOperatorIndex );
	CPPUNIT_TEST( TestOperatorPlus );
	CPPUNIT_TEST( TestOperatorFloatMultiplyVec );
	CPPUNIT_TEST_SUITE_END();

public:
	virtual void setUp();

	void TestConstructor();
	void TestOperatorIndex();
	void TestOperatorPlus();
	void TestOperatorFloatMultiplyVec();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( idVec2Test );

void idVec2Test::setUp() {
	idMath::Init();
}

void idVec2Test::TestConstructor() {
	idVec2 v( 1.0f, 2.0f );
	CPPUNIT_ASSERT_EQUAL( 1.0f, v.x );
	CPPUNIT_ASSERT_EQUAL( 2.0f, v.y );
}

void idVec2Test::TestOperatorIndex() {
	idVec2 v;
	v.x = 1.0f;
	v.y = 2.0f;
	CPPUNIT_ASSERT_EQUAL( 1.0f, v[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( 2.0f, v[ 1 ] );
}

void idVec2Test::TestOperatorPlus() {
	idVec2 v1( 23.0f, 76.0f );
	idVec2 v2( 4.0f, 5.0f );
	idVec2 v = v1 + v2;
	CPPUNIT_ASSERT_EQUAL( 27.0f, v.x );
	CPPUNIT_ASSERT_EQUAL( 81.0f, v.y );
}

void idVec2Test::TestOperatorFloatMultiplyVec() {
	idVec2 v1( 1.0f, 2.0f );
	idVec2 v = 3 * v1;
	CPPUNIT_ASSERT_EQUAL( 3.0f, v.x );
	CPPUNIT_ASSERT_EQUAL( 6.0f, v.y );
}
