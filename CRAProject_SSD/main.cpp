#include "gmock/gmock.h"
#define __DEBUG__ (1)

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

#if (__DEBUG__ == 1)
int main()
{
	testing::InitGoogleMock();
	return RUN_ALL_TESTS();
}
#else
int main()
{
	while (1)
	{

	}
	return 0;
}
#endif