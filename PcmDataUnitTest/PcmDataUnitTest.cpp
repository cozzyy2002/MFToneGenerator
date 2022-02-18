// PcmDataUnitTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::InitGoogleMock(&argc, argv);
	return RUN_ALL_TESTS();
}
