#include "gmock/gmock.h"
#include "sddDriver.cpp"
#define __DEBUG__ (1)

#if (__DEBUG__ == 1)
int main()
{
	testing::InitGoogleMock();
	return RUN_ALL_TESTS();
}
#else
int main(int argc, char* argv[])
{
	SSDDriver ssdDriver;
	ssdDriver.run(argc, argv);
	return 0;
}
#endif