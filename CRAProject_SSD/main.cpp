#include "gmock/gmock.h"
#define __DEBUG__ (1)

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