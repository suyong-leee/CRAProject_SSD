#include "gmock/gmock.h"

using namespace testing;

#define MAX_LBA 100;

int ssdMap[100];

class SSDDriver {
	void write(int addr, int value) {
		ssdMap[addr] = value;
	}
	int read(int addr) {
		return ssdMap[addr];
	}
};

TEST(a, b)
{
	EXPECT_EQ(1, 1);
}