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

class idVec3Test : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(idVec3Test);
	CPPUNIT_TEST(TestConstructor);
	CPPUNIT_TEST_SUITE_END();

public:
	virtual void setUp();

	void TestConstructor();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(idVec3Test);

void idVec3Test::setUp()
{
//	idMath::Init();
}

void idVec3Test::TestConstructor()
{
	idVec3 v(1.0f, 2.0f, 3.0f);
	CPPUNIT_ASSERT_EQUAL(1.0f, v.x);
	CPPUNIT_ASSERT_EQUAL(2.0f, v.y);
	CPPUNIT_ASSERT_EQUAL(3.0f, v.z);
}
