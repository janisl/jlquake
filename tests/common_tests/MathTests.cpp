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

class idMathTest : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(idMathTest);
	CPPUNIT_TEST(TestRSqrt);
	CPPUNIT_TEST(TestDEG2RAD);
	CPPUNIT_TEST(TestRAD2DEG);
	CPPUNIT_TEST(TestFtoiFast);
	CPPUNIT_TEST_SUITE_END();

public:
	void TestRSqrt();
	void TestDEG2RAD();
	void TestRAD2DEG();
	void TestFtoiFast();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(idMathTest);

void idMathTest::TestRSqrt()
{
	CPPUNIT_ASSERT_DOUBLES_EQUAL(0.25, idMath::RSqrt(16), 0.001);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(2, idMath::RSqrt(0.25), 0.01);
}

void idMathTest::TestDEG2RAD()
{
	CPPUNIT_ASSERT_DOUBLES_EQUAL(idMath::PI / 6, DEG2RAD(30), 0.000001);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(idMath::PI * 1.25, DEG2RAD(225), 0.000001);
}

void idMathTest::TestRAD2DEG()
{
	CPPUNIT_ASSERT_DOUBLES_EQUAL(30, RAD2DEG(idMath::PI / 6), 0.0001);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(225, RAD2DEG(idMath::PI * 1.25), 0.0001);
}

void idMathTest::TestFtoiFast()
{
	CPPUNIT_ASSERT_EQUAL(5, idMath::FtoiFast(5.1));
	CPPUNIT_ASSERT_EQUAL(-6, idMath::FtoiFast(-6.1));
}
