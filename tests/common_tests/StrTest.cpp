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

#include "../../source/common/Str.h"

class idStrTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE( idStrTest );
	CPPUNIT_TEST( TestCtor1 );
	CPPUNIT_TEST( TestCtor2 );
	CPPUNIT_TEST( TestCtor3 );
	CPPUNIT_TEST( TestCtor4 );
	CPPUNIT_TEST( TestCtor5 );
	CPPUNIT_TEST( TestCStr );
	CPPUNIT_TEST( TestOperatorIndex );
	CPPUNIT_TEST( TestOperatorAssign );
	CPPUNIT_TEST( TestOperatorAdd );
	CPPUNIT_TEST( TestOperatorAppend );
	CPPUNIT_TEST( TestOperatorEq );
	CPPUNIT_TEST( TestCmp );
	CPPUNIT_TEST( TestLength );
	CPPUNIT_TEST( TestAppend );
	CPPUNIT_TEST( TestSplit );
	CPPUNIT_TEST( TestSplit2 );
	CPPUNIT_TEST( TestStaticCmp );
	CPPUNIT_TEST( TestStaticLength );
	CPPUNIT_TEST_SUITE_END();

public:
	void TestCtor1();
	void TestCtor2();
	void TestCtor3();
	void TestCtor4();
	void TestCtor5();
	void TestCStr();
	void TestOperatorIndex();
	void TestOperatorAssign();
	void TestOperatorAdd();
	void TestOperatorAppend();
	void TestOperatorEq();
	void TestCmp();
	void TestLength();
	void TestAppend();
	void TestSplit();
	void TestSplit2();
	void TestStaticCmp();
	void TestStaticLength();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( idStrTest );

void idStrTest::TestCtor1() {
	idStr s;
	CPPUNIT_ASSERT_EQUAL( '\0', s.CStr()[0] );
}

void idStrTest::TestCtor2() {
	idStr s( "aaaa" );
	CPPUNIT_ASSERT_EQUAL( 0, idStr::Cmp( s.CStr(), "aaaa" ) );
}

void idStrTest::TestCtor3() {
	idStr s1( "aaaa" );
	idStr s2( s1 );
	CPPUNIT_ASSERT_EQUAL( 0, idStr::Cmp( s2.CStr(), "aaaa" ) );
}

void idStrTest::TestCtor4() {
	idStr s( "1234567890", 3, 7 );
	CPPUNIT_ASSERT_EQUAL( 0, idStr::Cmp( s.CStr(), "4567" ) );
}

void idStrTest::TestCtor5() {
	idStr s1( "1234567890" );
	idStr s2( s1, 3, 7 );
	CPPUNIT_ASSERT_EQUAL( 0, idStr::Cmp( s2.CStr(), "4567" ) );
}

void idStrTest::TestCStr() {
	idStr s( "abc" );
	CPPUNIT_ASSERT_EQUAL( 'a', s.CStr()[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( 'b', s.CStr()[ 1 ] );
	CPPUNIT_ASSERT_EQUAL( 'c', s.CStr()[ 2 ] );
	CPPUNIT_ASSERT_EQUAL( '\0', s.CStr()[ 3 ] );
}

void idStrTest::TestOperatorIndex() {
	idStr s( "abc" );
	CPPUNIT_ASSERT_EQUAL( 'a', s[ 0 ] );
	CPPUNIT_ASSERT_EQUAL( 'b', s[ 1 ] );
	CPPUNIT_ASSERT_EQUAL( 'c', s[ 2 ] );
	CPPUNIT_ASSERT_EQUAL( '\0', s[ 3 ] );
	s[ 1 ] = 'd';
	CPPUNIT_ASSERT_EQUAL( 'd', s[ 1 ] );
}

void idStrTest::TestOperatorAssign() {
	idStr s1;
	idStr s2;
	s1 = "abc";
	CPPUNIT_ASSERT( s1 == "abc" );
	s2 = s1;
	CPPUNIT_ASSERT( s2 == "abc" );
}

void idStrTest::TestOperatorAdd() {
	idStr s1( "123" );
	idStr s2( "hgf" );
	CPPUNIT_ASSERT( s1 + "abc" == "123abc" );
	CPPUNIT_ASSERT( "abc" + s1 == "abc123" );
	CPPUNIT_ASSERT( s1 + s2 == "123hgf" );
}

void idStrTest::TestOperatorAppend() {
	idStr s1( "123" );
	idStr s2( "456" );
	s1 += "abc";
	CPPUNIT_ASSERT( s1 == "123abc" );
	s2 += s1;
	CPPUNIT_ASSERT( s2 == "456123abc" );
}

void idStrTest::TestOperatorEq() {
	idStr s1( "aaa" );
	idStr s2( "bbb" );
	idStr s3( "bbb" );
	CPPUNIT_ASSERT_EQUAL( false, s1 == s2 );
	CPPUNIT_ASSERT_EQUAL( true, s2 == s3 );
	CPPUNIT_ASSERT_EQUAL( false, s1 == "ccc" );
	CPPUNIT_ASSERT_EQUAL( true, s2 == "bbb" );
	CPPUNIT_ASSERT_EQUAL( false, "ddd" == s2 );
	CPPUNIT_ASSERT_EQUAL( true, "bbb" == s3 );
}

void idStrTest::TestCmp() {
	idStr s( "abc" );
	CPPUNIT_ASSERT_EQUAL( 0, s.Cmp( "abc" ) );
	CPPUNIT_ASSERT_EQUAL( -1, s.Cmp( "abd" ) );
	CPPUNIT_ASSERT_EQUAL( 1, s.Cmp( "aba" ) );
}

void idStrTest::TestLength() {
	idStr s1( "abc" );
	idStr s2( "12345678901234567890" );
	CPPUNIT_ASSERT_EQUAL( 3, s1.Length() );
	CPPUNIT_ASSERT_EQUAL( 20, s2.Length() );
}

void idStrTest::TestAppend() {
	idStr s1( "123" );
	idStr s2( "456" );
	s1.Append( "abc" );
	CPPUNIT_ASSERT( s1 == "123abc" );
	s2.Append( s1 );
	CPPUNIT_ASSERT( s2 == "456123abc" );
}

void idStrTest::TestSplit() {
	idStr s( "a1 a2 a3" );
	idList<idStr> parts;
	s.Split( ' ', parts );
	CPPUNIT_ASSERT_EQUAL( 3, parts.Num() );
	CPPUNIT_ASSERT( parts[ 0 ] == "a1" );
	CPPUNIT_ASSERT( parts[ 1 ] == "a2" );
	CPPUNIT_ASSERT( parts[ 2 ] == "a3" );
}

void idStrTest::TestSplit2() {
	idStr s( "a1-a2/a3" );
	idList<idStr> parts;
	s.Split( " -/", parts );
	CPPUNIT_ASSERT_EQUAL( 3, parts.Num() );
	CPPUNIT_ASSERT( parts[ 0 ] == "a1" );
	CPPUNIT_ASSERT( parts[ 1 ] == "a2" );
	CPPUNIT_ASSERT( parts[ 2 ] == "a3" );
}

void idStrTest::TestStaticCmp() {
	CPPUNIT_ASSERT_EQUAL( 0, idStr::Cmp( "abc", "abc" ) );
	CPPUNIT_ASSERT_EQUAL( -1, idStr::Cmp( "abc", "abd" ) );
	CPPUNIT_ASSERT_EQUAL( 1, idStr::Cmp( "abc", "aba" ) );
}

void idStrTest::TestStaticLength() {
	CPPUNIT_ASSERT_EQUAL( 3, idStr::Length( "abc" ) );
	CPPUNIT_ASSERT_EQUAL( 20, idStr::Length( "12345678901234567890" ) );
}
