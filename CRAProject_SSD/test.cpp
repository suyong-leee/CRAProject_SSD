#include "gmock/gmock.h"
#include <string>
#include <fstream>

using namespace testing;
using namespace std;

#define MAX_LBA 100;

int ssdMap[100];

class SSDDriver {
public:
	void write(int addr, string value) {
		if (addr < 0 || addr >= 100) {
			makeError();
			return;
		}

		streampos writeOffset = (10 * addr);
		
		nand.open(nandFileName, std::ios::in | std::ios::out);
		nand.seekp(writeOffset);
	    nand << value;
		nand.close();
	}
	int read(int addr) {
		return ssdMap[addr];
	}
private:
	fstream nand;
	const string nandFileName = "ssd_nand.txt";

	void makeError(void) {
		ofstream error("output.txt");
		if (error.is_open()) {
			cout << "dbg " << endl;
		}
		error << "ERROR";
		error.close();
	}
};

TEST(a, b)
{
	EXPECT_EQ(1, 1);
}

TEST(SSDdrvierTest, TC1correctWrite)
{
	SSDDriver ssdDriver;

	ssdDriver.write(0, "0x12345678");
	
	//TODO: check value by read function
	EXPECT_EQ(1, 1);
}

TEST(SSDdrvierTest, TC2correctWriteSeveralTimes)
{
	SSDDriver ssdDriver;

	for (int i = 0;i < 10;i++) {
		ssdDriver.write(i, "0x12344567");
	}

	//TODO: check value by read function
	EXPECT_EQ(1, 1);
}

TEST(SSDdrvierTest, TC3correctWriteInOffset)
{
	SSDDriver ssdDriver;

	for (int i = 0;i < 10;i++) {
		ssdDriver.write(i*5, "0x1234AAAA");
	}

	//TODO: check value by read function
	EXPECT_EQ(1, 1);
}

TEST(SSDdrvierTest, TC4WriteInWrongPosition)
{
	SSDDriver ssdDriver;
	
	ssdDriver.write(120, "0x1234AAAA");
	
	//TODO: check value by read function
	EXPECT_EQ(1, 1);
}
