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

#include "../../source/common/math/Vec3.h"

class idVec3Test : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( idVec3Test );
	CPPUNIT_TEST( TestConstructor );
	CPPUNIT_TEST( TestSet );
	CPPUNIT_TEST( TestZero );
	CPPUNIT_TEST( TestOperatorIndex );
	CPPUNIT_TEST( TestOperatorAssign );
	CPPUNIT_TEST( TestOperatorPlus );
	CPPUNIT_TEST( TestOperatorMinus );
	CPPUNIT_TEST( TestOperatorMultiplyVec );
	CPPUNIT_TEST( TestOperatorMultiplyFloat );
	CPPUNIT_TEST( TestOperatorPlusEqualsVec );
	CPPUNIT_TEST( TestOperatorMinusEqualsVec );
	CPPUNIT_TEST( TestOperatorMultiplyEqualsFloat );
	CPPUNIT_TEST( TestOperatorFloatMultiplyVec );
	CPPUNIT_TEST( TestCompare );
	CPPUNIT_TEST( TestOperatorEquals );
	CPPUNIT_TEST( TestLength );
	CPPUNIT_TEST( TestLengthSqr );
	CPPUNIT_TEST( TestNormalize );
	CPPUNIT_TEST( TestToFloatPtr );
	CPPUNIT_TEST_SUITE_END();

public:
	virtual void setUp();

	void TestConstructor();
	void TestSet();
	void TestZero();
	void TestOperatorIndex();
	void TestOperatorAssign();
	void TestOperatorPlus();
	void TestOperatorMinus();
	void TestOperatorMultiplyVec();
	void TestOperatorMultiplyFloat();
	void TestOperatorPlusEqualsVec();
	void TestOperatorMinusEqualsVec();
	void TestOperatorMultiplyEqualsFloat();
	void TestOperatorFloatMultiplyVec();
	void TestCompare();
	void TestOperatorEquals();
	void TestLength();
	void TestLengthSqr();
	void TestNormalize();
	void TestToFloatPtr();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( idVec3Test );

void idVec3Test::setUp() {
	idMath::Init();
}

void idVec3Test::TestConstructor() {
	idVec3 v( 1.0f, 2.0f, 3.0f );
	CPPUNIT_ASSERT_EQUAL( 1.0f, v.x );
	CPPUNIT_ASSERT_EQUAL( 2.0f, v.y );
	CPPUNIT_ASSERT_EQUAL( 3.0f, v.z );
}

void idVec3Test::TestSet() {
	idVec3 v;
	v.Set( 1.0f, 2.0f, 3.0f );
	CPPUNIT_ASSERT_EQUAL( 1.0f, v.x );
	CPPUNIT_ASSERT_EQUAL( 2.0f, v.y );
	CPPUNIT_ASSERT_EQUAL( 3.0f, v.z );
}

void idVec3Test::TestZero() {
	idVec3 v( 1.0f, 2.0f, 3.0f );
	v.Zero();
	CPPUNIT_ASSERT_EQUAL( 0.0f, v.x );
	CPPUNIT_ASSERT_EQUAL( 0.0f, v.y );
	CPPUNIT_ASSERT_EQUAL( 0.0f, v.z );
}

void idVec3Test::TestOperatorIndex() {
	idVec3 v( 1.0f, 2.0f, 3.0f );
	CPPUNIT_ASSERT_EQUAL( 1.0f, v[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( 2.0f, v[ 1 ] );
	CPPUNIT_ASSERT_EQUAL( 3.0f, v[ 2 ] );
}

void idVec3Test::TestOperatorAssign() {
	idVec3 v;
	idVec3 v2( 1.0f, 2.0f, 3.0f );
	v = v2;
	CPPUNIT_ASSERT_EQUAL( 1.0f, v.x );
	CPPUNIT_ASSERT_EQUAL( 2.0f, v.y );
	CPPUNIT_ASSERT_EQUAL( 3.0f, v.z );
}

void idVec3Test::TestOperatorPlus() {
	idVec3 v1( 23.0f, 76.0f, 47.0f );
	idVec3 v2( 4.0f, 5.0f, 6.0f );
	idVec3 v = v1 + v2;
	CPPUNIT_ASSERT_EQUAL( 27.0f, v.x );
	CPPUNIT_ASSERT_EQUAL( 81.0f, v.y );
	CPPUNIT_ASSERT_EQUAL( 53.0f, v.z );
}

void idVec3Test::TestOperatorMinus() {
	idVec3 v1( 34.0f, 73.0f, 83.0f );
	idVec3 v2( 4.0f, 5.0f, 6.0f );
	idVec3 v = v1 - v2;
	CPPUNIT_ASSERT_EQUAL( 30.0f, v.x );
	CPPUNIT_ASSERT_EQUAL( 68.0f, v.y );
	CPPUNIT_ASSERT_EQUAL( 77.0f, v.z );
}

void idVec3Test::TestOperatorMultiplyVec() {
	idVec3 v1( 1.0f, 2.0f, 3.0f );
	idVec3 v2( 4.0f, 5.0f, 6.0f );
	CPPUNIT_ASSERT_EQUAL( 32.0f, v1 * v2 );
}

void idVec3Test::TestOperatorMultiplyFloat() {
	idVec3 v1( 1.0f, 2.0f, 3.0f );
	idVec3 v = v1 * 3;
	CPPUNIT_ASSERT_EQUAL( 3.0f, v.x );
	CPPUNIT_ASSERT_EQUAL( 6.0f, v.y );
	CPPUNIT_ASSERT_EQUAL( 9.0f, v.z );
}

void idVec3Test::TestOperatorPlusEqualsVec() {
	idVec3 v( 1.0f, 2.0f, 3.0f );
	idVec3 v2( 4.0f, 5.0f, 6.0f );
	v += v2;
	CPPUNIT_ASSERT_EQUAL( 5.0f, v.x );
	CPPUNIT_ASSERT_EQUAL( 7.0f, v.y );
	CPPUNIT_ASSERT_EQUAL( 9.0f, v.z );
}

void idVec3Test::TestOperatorMinusEqualsVec() {
	idVec3 v( 34.0f, 73.0f, 83.0f );
	idVec3 v2( 4.0f, 5.0f, 6.0f );
	v -= v2;
	CPPUNIT_ASSERT_EQUAL( 30.0f, v.x );
	CPPUNIT_ASSERT_EQUAL( 68.0f, v.y );
	CPPUNIT_ASSERT_EQUAL( 77.0f, v.z );
}

void idVec3Test::TestOperatorMultiplyEqualsFloat() {
	idVec3 v( 1.0f, 2.0f, 3.0f );
	v *= 3;
	CPPUNIT_ASSERT_EQUAL( 3.0f, v.x );
	CPPUNIT_ASSERT_EQUAL( 6.0f, v.y );
	CPPUNIT_ASSERT_EQUAL( 9.0f, v.z );
}

void idVec3Test::TestOperatorFloatMultiplyVec() {
	idVec3 v1( 1.0f, 2.0f, 3.0f );
	idVec3 v = 3 * v1;
	CPPUNIT_ASSERT_EQUAL( 3.0f, v.x );
	CPPUNIT_ASSERT_EQUAL( 6.0f, v.y );
	CPPUNIT_ASSERT_EQUAL( 9.0f, v.z );
}

void idVec3Test::TestCompare() {
	idVec3 v1( 1.0f, 2.0f, 3.0f );
	idVec3 v2( 1.0f, 2.0f, 3.0f );
	idVec3 v3( 45.0f, 75.0f, 456.0f );
	CPPUNIT_ASSERT_EQUAL( true, v1.Compare( v1 ) );
	CPPUNIT_ASSERT_EQUAL( true, v1.Compare( v2 ) );
	CPPUNIT_ASSERT_EQUAL( false, v1.Compare( v3 ) );
}

void idVec3Test::TestOperatorEquals() {
	idVec3 v1( 1.0f, 2.0f, 3.0f );
	idVec3 v2( 1.0f, 2.0f, 3.0f );
	idVec3 v3( 45.0f, 75.0f, 456.0f );
	CPPUNIT_ASSERT_EQUAL( true, v1 == v1 );
	CPPUNIT_ASSERT_EQUAL( true, v1 == v2 );
	CPPUNIT_ASSERT_EQUAL( false, v1 == v3 );
}

void idVec3Test::TestLength() {
	idVec3 v( 1.0f, 2.0f, 3.0f );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 3.74166f, v.Length(), 0.00001 );
}

void idVec3Test::TestLengthSqr() {
	idVec3 v( 1.0f, 2.0f, 3.0f );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 14.0f, v.LengthSqr(), 0.00001 );
}

void idVec3Test::TestNormalize() {
	idVec3 v( 1.0f, 2.0f, 3.0f );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 3.74166f, v.Normalize(), 0.00001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 1.0f, v.Length(), 0.000001 );
}

void idVec3Test::TestToFloatPtr() {
	idVec3 v( 1.0f, 2.0f, 3.0f );
	CPPUNIT_ASSERT_EQUAL( 1.0f, v.ToFloatPtr()[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( 2.0f, v.ToFloatPtr()[ 1 ] );
	CPPUNIT_ASSERT_EQUAL( 3.0f, v.ToFloatPtr()[ 2 ] );
}
