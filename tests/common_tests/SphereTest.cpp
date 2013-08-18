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

#include "../../source/common/bv/Sphere.h"
#include "test_utils.h"

class idSphereTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( idSphereTest );
	CPPUNIT_TEST( TestConstructor );
	CPPUNIT_TEST_SUITE_END();

public:
	virtual void setUp();

	void TestConstructor();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( idSphereTest );

void idSphereTest::setUp() {
	idMath::Init();
}

void idSphereTest::TestConstructor() {
	idVec3 v( 1.0f, 2.0f, 3.0f );
	idSphere s1( v );
	CPPUNIT_ASSERT_EQUAL( v, s1.GetOrigin() );
	CPPUNIT_ASSERT_EQUAL( 0.0f, s1.GetRadius() );

	idSphere s2( v, 56.0f );
	CPPUNIT_ASSERT_EQUAL( v, s2.GetOrigin() );
	CPPUNIT_ASSERT_EQUAL( 56.0f, s2.GetRadius() );
}
