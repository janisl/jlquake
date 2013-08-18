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
	CPPUNIT_TEST( TestOperatorPlusVec3 );
	CPPUNIT_TEST( TestClear );
	CPPUNIT_TEST( TestZero );
	CPPUNIT_TEST( TestAddPointMinX );
	CPPUNIT_TEST( TestAddPointMinY );
	CPPUNIT_TEST( TestAddPointMinZ );
	CPPUNIT_TEST( TestAddPointMaxX );
	CPPUNIT_TEST( TestAddPointMaxY );
	CPPUNIT_TEST( TestAddPointMaxZ );
	CPPUNIT_TEST( TestAddPointInside );
	CPPUNIT_TEST( TestAddPointCleared );
	CPPUNIT_TEST( TestAddBoundsMinX );
	CPPUNIT_TEST( TestAddBoundsMinY );
	CPPUNIT_TEST( TestAddBoundsMinZ );
	CPPUNIT_TEST( TestAddBoundsMaxX );
	CPPUNIT_TEST( TestAddBoundsMaxY );
	CPPUNIT_TEST( TestAddBoundsMaxZ );
	CPPUNIT_TEST( TestAddBoundsInside );
	CPPUNIT_TEST( TestAddBoundsCleared );
	CPPUNIT_TEST_SUITE_END();

public:
	virtual void setUp();

	void TestConstructor();
	void TestOperatorIndex();
	void TestOperatorPlusVec3();
	void TestClear();
	void TestZero();
	void TestAddPointMinX();
	void TestAddPointMinY();
	void TestAddPointMinZ();
	void TestAddPointMaxX();
	void TestAddPointMaxY();
	void TestAddPointMaxZ();
	void TestAddPointInside();
	void TestAddPointCleared();
	void TestAddBoundsMinX();
	void TestAddBoundsMinY();
	void TestAddBoundsMinZ();
	void TestAddBoundsMaxX();
	void TestAddBoundsMaxY();
	void TestAddBoundsMaxZ();
	void TestAddBoundsInside();
	void TestAddBoundsCleared();
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

void idBoundsTest::TestOperatorPlusVec3() {
	idBounds b1 ( idVec3( 2.0f, 3.0f, 4.0f ), idVec3( 20.0f, 30.0f, 25.0f ) );
	idBounds b2 = b1 + idVec3( 10.0f, 5.0f, 100.0f );
	CPPUNIT_ASSERT_EQUAL( idVec3( 12.0f, 8.0f, 104.0f ), b2[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( idVec3( 30.0f, 35.0f, 125.0f ), b2[ 1 ] );
}

void idBoundsTest::TestClear() {
	idBounds b;
	b.Clear();
	CPPUNIT_ASSERT_EQUAL( true, b[ 0 ].x > b[ 1 ].x );
	CPPUNIT_ASSERT_EQUAL( true, b[ 0 ].y > b[ 1 ].y );
	CPPUNIT_ASSERT_EQUAL( true, b[ 0 ].z > b[ 1 ].z );
}

void idBoundsTest::TestZero() {
	idBounds b;
	b.Zero();
	CPPUNIT_ASSERT_EQUAL( vec3_origin, b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( vec3_origin, b[ 1 ] );
}

void idBoundsTest::TestAddPointMinX() {
	idBounds b( idVec3( -10.0f, -10.0f, -10.0f), idVec3( 10.0f, 10.0f, 10.0f ) );
	CPPUNIT_ASSERT_EQUAL( true, b.AddPoint( idVec3( -20.0f, 0, 0 ) ) );
	CPPUNIT_ASSERT_EQUAL( idVec3( -20.0f, -10.0f, -10.0f), b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( idVec3( 10.0f, 10.0f, 10.0f ), b[ 1 ] );
}

void idBoundsTest::TestAddPointMinY() {
	idBounds b( idVec3( -10.0f, -10.0f, -10.0f), idVec3( 10.0f, 10.0f, 10.0f ) );
	CPPUNIT_ASSERT_EQUAL( true, b.AddPoint( idVec3( 0, -20.0f, 0 ) ) );
	CPPUNIT_ASSERT_EQUAL( idVec3( -10.0f, -20.0f, -10.0f), b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( idVec3( 10.0f, 10.0f, 10.0f ), b[ 1 ] );
}

void idBoundsTest::TestAddPointMinZ() {
	idBounds b( idVec3( -10.0f, -10.0f, -10.0f), idVec3( 10.0f, 10.0f, 10.0f ) );
	CPPUNIT_ASSERT_EQUAL( true, b.AddPoint( idVec3( 0, 0, -20.0f ) ) );
	CPPUNIT_ASSERT_EQUAL( idVec3( -10.0f, -10.0f, -20.0f), b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( idVec3( 10.0f, 10.0f, 10.0f ), b[ 1 ] );
}

void idBoundsTest::TestAddPointMaxX() {
	idBounds b( idVec3( -10.0f, -10.0f, -10.0f), idVec3( 10.0f, 10.0f, 10.0f ) );
	CPPUNIT_ASSERT_EQUAL( true, b.AddPoint( idVec3( 20.0f, 0, 0 ) ) );
	CPPUNIT_ASSERT_EQUAL( idVec3( -10.0f, -10.0f, -10.0f), b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( idVec3( 20.0f, 10.0f, 10.0f ), b[ 1 ] );
}

void idBoundsTest::TestAddPointMaxY() {
	idBounds b( idVec3( -10.0f, -10.0f, -10.0f), idVec3( 10.0f, 10.0f, 10.0f ) );
	CPPUNIT_ASSERT_EQUAL( true, b.AddPoint( idVec3( 0, 20.0f, 0 ) ) );
	CPPUNIT_ASSERT_EQUAL( idVec3( -10.0f, -10.0f, -10.0f), b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( idVec3( 10.0f, 20.0f, 10.0f ), b[ 1 ] );
}

void idBoundsTest::TestAddPointMaxZ() {
	idBounds b( idVec3( -10.0f, -10.0f, -10.0f), idVec3( 10.0f, 10.0f, 10.0f ) );
	CPPUNIT_ASSERT_EQUAL( true, b.AddPoint( idVec3( 0, 0, 20.0f ) ) );
	CPPUNIT_ASSERT_EQUAL( idVec3( -10.0f, -10.0f, -10.0f), b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( idVec3( 10.0f, 10.0f, 20.0f ), b[ 1 ] );
}

void idBoundsTest::TestAddPointInside() {
	idBounds b( idVec3( -10.0f, -10.0f, -10.0f), idVec3( 10.0f, 10.0f, 10.0f ) );
	CPPUNIT_ASSERT_EQUAL( false, b.AddPoint( idVec3( 0, 0, 0 ) ) );
	CPPUNIT_ASSERT_EQUAL( idVec3( -10.0f, -10.0f, -10.0f), b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( idVec3( 10.0f, 10.0f, 10.0f ), b[ 1 ] );
}

void idBoundsTest::TestAddPointCleared() {
	idVec3 v1( -2.0f, -7.0f, -8.0f );
	idBounds b;
	b.Clear();
	CPPUNIT_ASSERT_EQUAL( true, b.AddPoint( v1 ) );
	CPPUNIT_ASSERT_EQUAL( v1, b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( v1, b[ 1 ] );
}

void idBoundsTest::TestAddBoundsMinX() {
	idBounds b( idVec3( -10.0f, -10.0f, -10.0f), idVec3( 10.0f, 10.0f, 10.0f ) );
	CPPUNIT_ASSERT_EQUAL( true, b.AddBounds( idBounds( idVec3( -20.0f, 0, 0 ), vec3_origin ) ) );
	CPPUNIT_ASSERT_EQUAL( idVec3( -20.0f, -10.0f, -10.0f), b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( idVec3( 10.0f, 10.0f, 10.0f ), b[ 1 ] );
}

void idBoundsTest::TestAddBoundsMinY() {
	idBounds b( idVec3( -10.0f, -10.0f, -10.0f), idVec3( 10.0f, 10.0f, 10.0f ) );
	CPPUNIT_ASSERT_EQUAL( true, b.AddBounds( idBounds( idVec3( 0, -20.0f, 0 ), vec3_origin ) ) );
	CPPUNIT_ASSERT_EQUAL( idVec3( -10.0f, -20.0f, -10.0f), b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( idVec3( 10.0f, 10.0f, 10.0f ), b[ 1 ] );
}

void idBoundsTest::TestAddBoundsMinZ() {
	idBounds b( idVec3( -10.0f, -10.0f, -10.0f), idVec3( 10.0f, 10.0f, 10.0f ) );
	CPPUNIT_ASSERT_EQUAL( true, b.AddBounds( idBounds( idVec3( 0, 0, -20.0f ), vec3_origin ) ) );
	CPPUNIT_ASSERT_EQUAL( idVec3( -10.0f, -10.0f, -20.0f), b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( idVec3( 10.0f, 10.0f, 10.0f ), b[ 1 ] );
}

void idBoundsTest::TestAddBoundsMaxX() {
	idBounds b( idVec3( -10.0f, -10.0f, -10.0f), idVec3( 10.0f, 10.0f, 10.0f ) );
	CPPUNIT_ASSERT_EQUAL( true, b.AddBounds( idBounds( vec3_origin, idVec3( 20.0f, 0, 0 ) ) ) );
	CPPUNIT_ASSERT_EQUAL( idVec3( -10.0f, -10.0f, -10.0f), b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( idVec3( 20.0f, 10.0f, 10.0f ), b[ 1 ] );
}

void idBoundsTest::TestAddBoundsMaxY() {
	idBounds b( idVec3( -10.0f, -10.0f, -10.0f), idVec3( 10.0f, 10.0f, 10.0f ) );
	CPPUNIT_ASSERT_EQUAL( true, b.AddBounds( idBounds( vec3_origin, idVec3( 0, 20.0f, 0 ) ) ) );
	CPPUNIT_ASSERT_EQUAL( idVec3( -10.0f, -10.0f, -10.0f), b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( idVec3( 10.0f, 20.0f, 10.0f ), b[ 1 ] );
}

void idBoundsTest::TestAddBoundsMaxZ() {
	idBounds b( idVec3( -10.0f, -10.0f, -10.0f), idVec3( 10.0f, 10.0f, 10.0f ) );
	CPPUNIT_ASSERT_EQUAL( true, b.AddBounds( idBounds( vec3_origin, idVec3( 0, 0, 20.0f ) ) ) );
	CPPUNIT_ASSERT_EQUAL( idVec3( -10.0f, -10.0f, -10.0f), b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( idVec3( 10.0f, 10.0f, 20.0f ), b[ 1 ] );
}

void idBoundsTest::TestAddBoundsInside() {
	idBounds b( idVec3( -10.0f, -10.0f, -10.0f), idVec3( 10.0f, 10.0f, 10.0f ) );
	CPPUNIT_ASSERT_EQUAL( false, b.AddBounds( idBounds( vec3_origin, vec3_origin ) ) );
	CPPUNIT_ASSERT_EQUAL( idVec3( -10.0f, -10.0f, -10.0f), b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( idVec3( 10.0f, 10.0f, 10.0f ), b[ 1 ] );
}

void idBoundsTest::TestAddBoundsCleared() {
	idVec3 v1( -2.0f, -7.0f, -8.0f );
	idVec3 v2( 2.0f, 7.0f, 8.0f );
	idBounds b;
	b.Clear();
	CPPUNIT_ASSERT_EQUAL( true, b.AddBounds( idBounds( v1, v2 ) ) );
	CPPUNIT_ASSERT_EQUAL( v1, b[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( v2, b[ 1 ] );
}
