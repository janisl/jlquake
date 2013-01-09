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
	CPPUNIT_TEST_SUITE_END();

public:
	void TestRSqrt();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(idMathTest);

void idMathTest::TestRSqrt()
{
	CPPUNIT_ASSERT_DOUBLES_EQUAL(0.25, idMath::RSqrt(16), 0.001);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(2, idMath::RSqrt(0.25), 0.01);
}
