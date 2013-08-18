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

#include "../../source/common/math/Plane.h"
#include "test_utils.h"

class idPlaneTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( idPlaneTest );
	CPPUNIT_TEST( TestConstructor );
	CPPUNIT_TEST( TestOperatorMinus );
	CPPUNIT_TEST( TestZero );
	CPPUNIT_TEST( TestSettersAndGetters );
	CPPUNIT_TEST( TestFitThroughPoint );
	CPPUNIT_TEST_SUITE_END();

public:
	virtual void setUp();

	void TestConstructor();
	void TestOperatorMinus();
	void TestZero();
	void TestSettersAndGetters();
	void TestFitThroughPoint();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( idPlaneTest );

void idPlaneTest::setUp() {
	idMath::Init();
}

void idPlaneTest::TestConstructor() {
	idPlane p1( 1.0f, 2.0f, 3.0f, 4.0f );
	CPPUNIT_ASSERT_EQUAL( idVec3( 1.0f, 2.0f, 3.0f ), p1.Normal() );
	CPPUNIT_ASSERT_EQUAL( -4.0f, p1.Dist() );

	idVec3 v( 1.0f, 2.0f, 3.0f );
	idPlane p2( v, 56.0f );
	CPPUNIT_ASSERT_EQUAL( v, p2.Normal() );
	CPPUNIT_ASSERT_EQUAL( 56.0f, p2.Dist() );
}

void idPlaneTest::TestOperatorMinus() {
	idPlane p1( 1.0f, 2.0f, 3.0f, 4.0f );
	idPlane p2 = -p1;
	CPPUNIT_ASSERT_EQUAL( idVec3( -1.0f, -2.0f, -3.0f ), p2.Normal() );
	CPPUNIT_ASSERT_EQUAL( 4.0f, p2.Dist() );
}

void idPlaneTest::TestZero() {
	idPlane p;
	p.Zero();
	CPPUNIT_ASSERT_EQUAL( vec3_origin, p.Normal() );
	CPPUNIT_ASSERT_EQUAL( 0.0f, p.Dist() );
}

void idPlaneTest::TestSettersAndGetters() {
	idVec3 v( 1.0f, 2.0f, 3.0f );
	idPlane p;
	p.SetNormal( v );
	p.SetDist( 4556.0f );
	CPPUNIT_ASSERT_EQUAL( v, p.Normal() );
	CPPUNIT_ASSERT_EQUAL( 4556.0f, p.Dist() );
	p.Normal()[ 0 ] = 6.0f;
	p.Normal()[ 1 ] = 98.0f;
	p.Normal()[ 2 ] = 79.0f;
	CPPUNIT_ASSERT_EQUAL( idVec3( 6.0f, 98.0f, 79.0f ), p.Normal() );
}

void idPlaneTest::TestFitThroughPoint() {
	idPlane p( 1.0f, 0.0f, 0.0f, 0.0f );
	p.FitThroughPoint( idVec3( 8.0f, 7.0f, 4.0f ) );
	CPPUNIT_ASSERT_EQUAL( 8.0f, p.Dist() );
}
