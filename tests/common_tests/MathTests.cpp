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

#include "../../source/common/math/Math.h"

class idMathTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( idMathTest );
	CPPUNIT_TEST( TestDEG2RAD );
	CPPUNIT_TEST( TestRAD2DEG );
	CPPUNIT_TEST( TestRSqrt );
	CPPUNIT_TEST( TestInvSqrt );
	CPPUNIT_TEST( TestSqrt );
	CPPUNIT_TEST( TestACos );
	CPPUNIT_TEST( TestFabs );
	CPPUNIT_TEST( TestFtoiFast );
	CPPUNIT_TEST( TestClampChar );
	CPPUNIT_TEST_SUITE_END();

public:
	virtual void setUp();

	void TestDEG2RAD();
	void TestRAD2DEG();
	void TestRSqrt();
	void TestInvSqrt();
	void TestSqrt();
	void TestACos();
	void TestFabs();
	void TestFtoiFast();
	void TestClampChar();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( idMathTest );

void idMathTest::setUp() {
	idMath::Init();
}

void idMathTest::TestDEG2RAD() {
	CPPUNIT_ASSERT_DOUBLES_EQUAL( idMath::PI / 6, DEG2RAD( 30 ), 0.000001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( idMath::PI * 1.25, DEG2RAD( 225 ), 0.000001 );
}

void idMathTest::TestRAD2DEG() {
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 30, RAD2DEG( idMath::PI / 6 ), 0.0001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 225, RAD2DEG( idMath::PI * 1.25 ), 0.0001 );
}

void idMathTest::TestRSqrt() {
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.25, idMath::RSqrt( 16 ), 0.001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 2, idMath::RSqrt( 0.25 ), 0.01 );
}

void idMathTest::TestInvSqrt() {
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.25, idMath::InvSqrt( 16 ), 0.0000001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 2, idMath::InvSqrt( 0.25 ), 0.0000001 );
}

void idMathTest::TestSqrt() {
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 4, idMath::Sqrt( 16 ), 0.0000001 );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 0.5, idMath::Sqrt( 0.25 ), 0.0000001 );
}

void idMathTest::TestACos() {
	CPPUNIT_ASSERT_EQUAL( 0.0f, idMath::ACos( 23 ) );
	CPPUNIT_ASSERT_EQUAL( idMath::PI, idMath::ACos( -456 ) );
	CPPUNIT_ASSERT_DOUBLES_EQUAL( 1.15928f, idMath::ACos( 0.4 ), 0.000001 );
}

void idMathTest::TestFabs() {
	CPPUNIT_ASSERT_EQUAL( 5.34f, idMath::Fabs( 5.34f ) );
	CPPUNIT_ASSERT_EQUAL( 6.345f, idMath::Fabs( -6.345f ) );
}

void idMathTest::TestFtoiFast() {
	CPPUNIT_ASSERT_EQUAL( 5, idMath::FtoiFast( 5.1 ) );
	CPPUNIT_ASSERT_EQUAL( -6, idMath::FtoiFast( -6.1 ) );
}

void idMathTest::TestClampChar() {
	CPPUNIT_ASSERT_EQUAL( static_cast<signed char>( 127 ), idMath::ClampChar( 64564 ) );
	CPPUNIT_ASSERT_EQUAL( static_cast<signed char>( -128 ), idMath::ClampChar( -9700978 ) );
	CPPUNIT_ASSERT_EQUAL( static_cast<signed char>( 6 ), idMath::ClampChar( 6 ) );
}
