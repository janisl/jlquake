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

#include "../../source/common/Hexen2EffectsRandom.h"

class idHexen2EffectsRandomTest : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(idHexen2EffectsRandomTest);
	CPPUNIT_TEST(TestSetSeed);
	CPPUNIT_TEST(TestSeedRand);
	CPPUNIT_TEST_SUITE_END();

public:
	void TestSetSeed();
	void TestSeedRand();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(idHexen2EffectsRandomTest);

void idHexen2EffectsRandomTest::TestSetSeed()
{
	idHexen2EffectsRandom rnd;
	rnd.SetSeed(23456);
	CPPUNIT_ASSERT_EQUAL(23456u, rnd.seed);
}

void idHexen2EffectsRandomTest::TestSeedRand()
{
	idHexen2EffectsRandom rnd;
	rnd.SetSeed(3456);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(0.121689, rnd.SeedRand(), 0.000001);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(0.779093, rnd.SeedRand(), 0.000001);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(0.32213, rnd.SeedRand(), 0.000001);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(0.566211, rnd.SeedRand(), 0.000001);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(0.6253, rnd.SeedRand(), 0.000001);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(0.446428, rnd.SeedRand(), 0.000001);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(0.575341, rnd.SeedRand(), 0.000001);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(0.631621, rnd.SeedRand(), 0.000001);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(0.989265, rnd.SeedRand(), 0.000001);
	CPPUNIT_ASSERT_DOUBLES_EQUAL(0.643459, rnd.SeedRand(), 0.000001);
}
