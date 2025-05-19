#include "gmock/gmock.h"
#include <string>
#include <string>
#include <fstream>

using namespace testing;
using namespace std;

#define MAX_LBA 100;

int ssdMap[100];

class SSDDriver {
public:
	virtual void run(vector<string> args)
	{
		string command = args[0];
		if (command == "W")
		{
			int addr = stoi(args[1]);
			string value = args[2];
			write(addr, value);
		}
		else if (command == "R")
		{
			int addr = stoi(args[1]);
			read(addr);
		}
	}
	virtual void write(int addr, string value) {
		if (addr < 0 || addr >= 100) {
			makeError();
			return;
		}

		streampos writeOffset = (10 * addr);
		
		nand.open(nandFileName, ios::out);
		if (nand.is_open() == false) {
			makeError();
			return;
		}
		nand.seekp(writeOffset);
	    nand << value;
		nand.close();
	}
	virtual int read(int addr) {
		return ssdMap[addr];
	}
private:
	fstream nand;
	const string nandFileName = "ssd_nand.txt";

	void makeError(void) {
		ofstream error("output.txt");
		if (error.is_open() == false) {
			return;
		}
		error << "ERROR";
		error.close();
	}
};

vector<string> parseArguments(int argc, char* argv[])
{
	vector<string> args;
	for (int i = 1; i < argc; ++i) 
	{
		args.emplace_back(argv[i]);
	}

	return args;
}

TEST(SSDdrvierTest, TC1correctWrite)
{
	SSDDriver ssdDriver;

	ssdDriver.write(0, "0x11111111");
	
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
		ssdDriver.write(i*5, "0x12222222");
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

class MockSSDDriver : public SSDDriver {
public:
	MOCK_METHOD(void, write, (int addr, string value), (override));
	MOCK_METHOD(int, read, (int addr), (override));
};

TEST(RunCommand, TC1WriteCommand)
{
	MockSSDDriver mockSsdDriver;
	vector<string> args = { "W", "20", "0x1289CDEF" };

	EXPECT_CALL(mockSsdDriver, write(20, "0x1289CDEF"));
	mockSsdDriver.run(args);
}

TEST(RunCommand, TC2ReadCommand)
{
	MockSSDDriver mockSsdDriver;
	vector<string> args = { "R", "20"};

	EXPECT_CALL(mockSsdDriver, read(20));
	mockSsdDriver.run(args);
}

TEST(ArgumentParsing, TC3EmptyArgument)
{
	int argc = 1;
	char* argv[1];
	argv[0] = const_cast<char*>("ssd.exe");

	vector<string> args = parseArguments(argc, argv);

	ASSERT_EQ(args.size(), 0);
}

TEST(ArgumentParsing, WriteArgument)
{
	int argc = 4;
	char* argv[4];
	argv[0] = const_cast<char*>("ssd.exe");
	argv[1] = const_cast<char*>("W");
	argv[2] = const_cast<char*>("20");
	argv[3] = const_cast<char*>("0x1289CDEF");

	vector<string> args = parseArguments(argc, argv);

	ASSERT_EQ(args.size(), 3);
	EXPECT_EQ(args[0], "W");
	EXPECT_EQ(args[1], "20");
	EXPECT_EQ(args[2], "0x1289CDEF");
}

TEST(ArgumentParsing, ReadArgument)
{
	int argc = 3;
	char* argv[3];
	argv[0] = const_cast<char*>("ssd.exe");
	argv[1] = const_cast<char*>("R");
	argv[2] = const_cast<char*>("20");

	vector<string> args = parseArguments(argc, argv);

	ASSERT_EQ(args.size(), 2);
	EXPECT_EQ(args[0], "R");
	EXPECT_EQ(args[1], "20");
}