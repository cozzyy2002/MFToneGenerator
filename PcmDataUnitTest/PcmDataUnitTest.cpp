// PcmDataUnitTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdio.h>

#include <PcmData/PcmData.h>
#define tsm_STATE_MACHINE_EXPORT __declspec(dllexport)
#include <../tsm/public/include/StateMachine/Assert.h>

int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);
	::testing::InitGoogleMock(&argc, argv);
	return RUN_ALL_TESTS();
}

/*static*/ HRESULT tsm::Assert::checkHResult(HRESULT hr, LPCTSTR exp, LPCTSTR sourceFile, int line)
{
	return hr;
}
