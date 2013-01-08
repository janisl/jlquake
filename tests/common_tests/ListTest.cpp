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

#include "../../source/common/containers/List.h"

class idListTest : public CppUnit::TestFixture
{
	CPPUNIT_TEST_SUITE(idListTest);
	CPPUNIT_TEST(TestNum);
	CPPUNIT_TEST(TestNumAllocated);
	CPPUNIT_TEST(TestClear);
	CPPUNIT_TEST(TestGranularity);
	CPPUNIT_TEST(TestMemory);
	CPPUNIT_TEST(TestIndex);
	CPPUNIT_TEST(TestCondense);
	CPPUNIT_TEST(TestSetNum);
	CPPUNIT_TEST(TestAssureSize);
	CPPUNIT_TEST(TestAssureSize2);
	CPPUNIT_TEST(TestPtr);
	CPPUNIT_TEST(TestAlloc);
	CPPUNIT_TEST(TestAddUnique);
	CPPUNIT_TEST(TestInsert);
	CPPUNIT_TEST(TestFindIndex);
	CPPUNIT_TEST(TestFind);
	CPPUNIT_TEST(TestFindNull);
	CPPUNIT_TEST(TestIndexOf);
	CPPUNIT_TEST(TestRemoveIndex);
	CPPUNIT_TEST(TestRemove);
	CPPUNIT_TEST(TestSort);
	CPPUNIT_TEST(TestSortSubSection);
	CPPUNIT_TEST(TestSwap);
	CPPUNIT_TEST(TestDeleteContents);
	CPPUNIT_TEST(TestDeleteContents2);
	CPPUNIT_TEST_SUITE_END();

public:
	void TestNum();
	void TestNumAllocated();
	void TestClear();
	void TestGranularity();
	void TestMemory();
	void TestIndex();
	void TestCondense();
	void TestSetNum();
	void TestAssureSize();
	void TestAssureSize2();
	void TestPtr();
	void TestAlloc();
	void TestAddUnique();
	void TestInsert();
	void TestFindIndex();
	void TestFind();
	void TestFindNull();
	void TestIndexOf();
	void TestRemoveIndex();
	void TestRemove();
	void TestSort();
	void TestSortSubSection();
	void TestSwap();
	void TestDeleteContents();
	void TestDeleteContents2();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(idListTest);

void idListTest::TestNum()
{
	idList<int> list;
	list.Append(4);
	list.Append(5);
	CPPUNIT_ASSERT_EQUAL(2, list.Num());
}

void idListTest::TestNumAllocated()
{
	idList<int> list;
	list.Resize(4);
	CPPUNIT_ASSERT_EQUAL(4, list.NumAllocated());
}

void idListTest::TestClear()
{
	idList<int> list;
	list.Append(4);
	list.Append(5);
	list.Clear();
	CPPUNIT_ASSERT_EQUAL(0, list.Num());
}

void idListTest::TestGranularity()
{
	idList<int> list;
	list.SetGranularity(64);
	CPPUNIT_ASSERT_EQUAL(64, list.GetGranularity());
}

void idListTest::TestMemory()
{
	idList<int> list;
	list.Append(4);
	list.Append(5);
	list.Resize(64);
	CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(256), list.Allocated());
	CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(256) + sizeof(list), list.Size());
	CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(8), list.MemoryUsed());
}

void idListTest::TestIndex()
{
	idList<int> list;
	list.Append(4);
	list.Append(5);
	CPPUNIT_ASSERT_EQUAL(4, list[0]);
	CPPUNIT_ASSERT_EQUAL(5, list[1]);
}

void idListTest::TestCondense()
{
	idList<int> list;
	list.Append(4);
	list.Append(5);
	list.Condense();
	CPPUNIT_ASSERT_EQUAL(2, list.NumAllocated());
}

void idListTest::TestSetNum()
{
	idList<int> list;
	list.SetNum(12);
	CPPUNIT_ASSERT_EQUAL(12, list.Num());
}

void idListTest::TestAssureSize()
{
	idList<int> list;
	list.AssureSize(12);
	CPPUNIT_ASSERT_EQUAL(12, list.Num());
}

void idListTest::TestAssureSize2()
{
	idList<int> list;
	list.AssureSize(23, 6);
	CPPUNIT_ASSERT_EQUAL(23, list.Num());
	CPPUNIT_ASSERT_EQUAL(6, list[0]);
	CPPUNIT_ASSERT_EQUAL(6, list[22]);
}

void idListTest::TestPtr()
{
	idList<int> list;
	list.Append(4);
	list.Append(5);
	CPPUNIT_ASSERT_EQUAL(4, list.Ptr()[0]);
	CPPUNIT_ASSERT_EQUAL(5, list.Ptr()[1]);
}

void idListTest::TestAlloc()
{
	idList<int> list;
	list.Alloc() = 7;
	list.Alloc() = 45;
	CPPUNIT_ASSERT_EQUAL(2, list.Num());
	CPPUNIT_ASSERT_EQUAL(7, list[0]);
	CPPUNIT_ASSERT_EQUAL(45, list[1]);
}

void idListTest::TestAddUnique()
{
	idList<int> list;
	list.AddUnique(7);
	list.AddUnique(45);
	list.AddUnique(7);
	CPPUNIT_ASSERT_EQUAL(2, list.Num());
	CPPUNIT_ASSERT_EQUAL(7, list[0]);
	CPPUNIT_ASSERT_EQUAL(45, list[1]);
}

void idListTest::TestInsert()
{
	idList<int> list;
	list.Append(67);
	list.Append(23);
	list.Insert(86, 1);
	CPPUNIT_ASSERT_EQUAL(3, list.Num());
	CPPUNIT_ASSERT_EQUAL(67, list[0]);
	CPPUNIT_ASSERT_EQUAL(86, list[1]);
	CPPUNIT_ASSERT_EQUAL(23, list[2]);
}

void idListTest::TestFindIndex()
{
	idList<int> list;
	list.Append(67);
	list.Append(23);
	CPPUNIT_ASSERT_EQUAL(0, list.FindIndex(67));
	CPPUNIT_ASSERT_EQUAL(1, list.FindIndex(23));
	CPPUNIT_ASSERT_EQUAL(-1, list.FindIndex(888));
}

void idListTest::TestFind()
{
	idList<int> list;
	list.Append(67);
	list.Append(23);
	CPPUNIT_ASSERT_EQUAL(&list[0], list.Find(67));
	CPPUNIT_ASSERT_EQUAL(&list[1], list.Find(23));
	CPPUNIT_ASSERT_EQUAL((int*)NULL, list.Find(888));
}

void idListTest::TestFindNull()
{
	int a, b;
	idList<int*> list;
	list.Append(&a);
	list.Append(&b);
	CPPUNIT_ASSERT_EQUAL(-1, list.FindNull());
	list.Append(NULL);
	CPPUNIT_ASSERT_EQUAL(2, list.FindNull());
}

void idListTest::TestIndexOf()
{
	idList<int> list;
	list.Append(67);
	list.Append(23);
	CPPUNIT_ASSERT_EQUAL(0, list.IndexOf(&list[0]));
	CPPUNIT_ASSERT_EQUAL(1, list.IndexOf(&list[1]));
}

void idListTest::TestRemoveIndex()
{
	idList<int> list;
	list.Append(45);
	list.Append(84);
	list.Append(457);
	list.RemoveIndex(1);
	CPPUNIT_ASSERT_EQUAL(2, list.Num());
	CPPUNIT_ASSERT_EQUAL(45, list[0]);
	CPPUNIT_ASSERT_EQUAL(457, list[1]);
}

void idListTest::TestRemove()
{
	idList<int> list;
	list.Append(45);
	list.Append(84);
	list.Append(457);
	list.Remove(45);
	CPPUNIT_ASSERT_EQUAL(2, list.Num());
	CPPUNIT_ASSERT_EQUAL(84, list[0]);
	CPPUNIT_ASSERT_EQUAL(457, list[1]);
}

void idListTest::TestSort()
{
	idList<int> list;
	list.Append(975);
	list.Append(45);
	list.Append(457);
	list.Append(84);
	list.Sort();
	CPPUNIT_ASSERT_EQUAL(45, list[0]);
	CPPUNIT_ASSERT_EQUAL(84, list[1]);
	CPPUNIT_ASSERT_EQUAL(457, list[2]);
	CPPUNIT_ASSERT_EQUAL(975, list[3]);
}

void idListTest::TestSortSubSection()
{
	idList<int> list;
	list.Append(975);
	list.Append(45);
	list.Append(457);
	list.Append(84);
	list.Append(1);
	list.SortSubSection(1, 3);
	CPPUNIT_ASSERT_EQUAL(975, list[0]);
	CPPUNIT_ASSERT_EQUAL(45, list[1]);
	CPPUNIT_ASSERT_EQUAL(84, list[2]);
	CPPUNIT_ASSERT_EQUAL(457, list[3]);
	CPPUNIT_ASSERT_EQUAL(1, list[4]);
}

void idListTest::TestSwap()
{
	idList<int> list1;
	list1.Append(975);
	list1.Append(45);
	idList<int> list2;
	list2.Append(457);
	list2.Append(84);
	list2.Append(1);
	list1.Swap(list2);
	CPPUNIT_ASSERT_EQUAL(3, list1.Num());
	CPPUNIT_ASSERT_EQUAL(457, list1[0]);
	CPPUNIT_ASSERT_EQUAL(84, list1[1]);
	CPPUNIT_ASSERT_EQUAL(1, list1[2]);
	CPPUNIT_ASSERT_EQUAL(2, list2.Num());
	CPPUNIT_ASSERT_EQUAL(975, list2[0]);
	CPPUNIT_ASSERT_EQUAL(45, list2[1]);
}

class idListDeleteContentsHelper
{
public:
	bool& deleted;

	idListDeleteContentsHelper(bool& deleted)
		: deleted(deleted)
	{}
	~idListDeleteContentsHelper()
	{
		deleted = true;
	}
};

void idListTest::TestDeleteContents()
{
	idList<idListDeleteContentsHelper*> list;
	bool deleted = false;
	list.Append(new idListDeleteContentsHelper(deleted));
	list.DeleteContents(true);
	CPPUNIT_ASSERT_EQUAL(0, list.Num());
	CPPUNIT_ASSERT_EQUAL(true, deleted);
}

void idListTest::TestDeleteContents2()
{
	idList<idListDeleteContentsHelper*> list;
	bool deleted = false;
	list.Append(new idListDeleteContentsHelper(deleted));
	list.DeleteContents(false);
	CPPUNIT_ASSERT_EQUAL(1, list.Num());
	CPPUNIT_ASSERT_EQUAL((idListDeleteContentsHelper*)NULL, list[0]);
	CPPUNIT_ASSERT_EQUAL(true, deleted);
}
