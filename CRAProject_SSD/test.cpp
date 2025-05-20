#include "gmock/gmock.h"
#include "ssdDriver.cpp"

using namespace testing;
using namespace std;

class SddDriverTestFixture : public Test {
protected:
	void SetUp() override {
		ssdDriver = new SSDDriver;
		srand(static_cast<unsigned int>(time(nullptr)));
		overwriteTextToFile("ssd_nand.txt", "");
		overwriteTextToFile("ssd_output.txt", "");
	}
public:
	SSDContext ctx;
	SSDDriver* ssdDriver;

	vector<std::string> parseArguments(int argc, char* argv[]) {
		return SSDDriver::parseArguments(argc, argv);
	}

	void overwriteTextToFile(string fileName, string text)
	{
		return SSDContext::overwriteTextToFile(fileName, text);
	}

	string getHex() {
		static const char hexDigits[] = "0123456789ABCDEF";
		string result = "0x";

		for (int i = 0; i < 8; ++i) {
			result += hexDigits[rand() % 16];
		}

		return result;
	}

	string readFileAsString(const string filename) {
		ifstream file(filename);
		stringstream buffer;
		buffer << file.rdbuf();
		return buffer.str();
	}

	friend class SSDDriver;
};

TEST_F(SddDriverTestFixture, TC0EmptyRead)
{
	overwriteTextToFile("ssd_nand.txt", "");

	ReadCommand readCmd(ctx, 0);
	readCmd.execute();

	string dataFromOutputText = readFileAsString("ssd_output.txt");

	EXPECT_EQ("0x00000000", dataFromOutputText);
}

TEST_F(SddDriverTestFixture, TC1correctWrite)
{
	WriteCommand writeCmd(ctx, 0, "0x11111111");
	writeCmd.execute();

	ReadCommand readCmd(ctx, 0);
	readCmd.execute();

	string data = readFileAsString("ssd_output.txt");
	EXPECT_EQ("0x11111111", data);
}

TEST_F(SddDriverTestFixture, TC2correctWriteSeveralTimes)
{
	for (int i = 0; i < 10; ++i) {
		string hex = getHex();

		WriteCommand writeCmd(ctx, i, hex);
		writeCmd.execute();

		ReadCommand readCmd(ctx, i);
		readCmd.execute();

		string dataFromOutputText = readFileAsString("ssd_output.txt");

		EXPECT_EQ(hex, dataFromOutputText);
	}
}

TEST_F(SddDriverTestFixture, TC3correctWriteInOffset)
{
	for (int i = 0; i < 10; ++i) {
		string hex = getHex();

		WriteCommand writeCmd(ctx, i * 5, hex);
		writeCmd.execute();

		ReadCommand readCmd(ctx, i * 5);
		readCmd.execute();

		string dataFromOutputText = readFileAsString("ssd_output.txt");

		EXPECT_EQ(hex, dataFromOutputText);
	}
}

TEST_F(SddDriverTestFixture, TC4WriteInWrongPosition)
{
	WriteCommand writeCmd(ctx, 120, "0x1234AAAA");
	writeCmd.execute();

	ReadCommand readCmd(ctx, 120);
	readCmd.execute();

	string data = readFileAsString("ssd_output.txt");

	EXPECT_NE("0x1234AAAA", data); 
}

TEST_F(SddDriverTestFixture, TC5WriteWithShortInputFormat)
{
	int addr = 3;
	string value = "0xBBBB";

	WriteCommand writeCmd(ctx, addr, value);
	writeCmd.execute();

	ReadCommand readCmd(ctx, addr);
	readCmd.execute();

	string data = readFileAsString("ssd_output.txt").substr(0, 6);

	EXPECT_NE("0xBBBB", data);
}

TEST_F(SddDriverTestFixture, TC5WriteWithNotStart0xFormat)
{
	int addr = 3;
	string value = "0XBBBBAEDE";

	WriteCommand writeCmd(ctx, addr, value);
	writeCmd.execute();

	ReadCommand readCmd(ctx, addr);
	readCmd.execute();

	string data = readFileAsString("ssd_output.txt");

	EXPECT_NE("0XBBBBAEDE", data);
}

TEST_F(SddDriverTestFixture, TC5WriteWithNotNumber)
{
	int addr = 3;
	string value = "0xZEBBAEDE";

	WriteCommand writeCmd(ctx, addr, value);
	writeCmd.execute();

	ReadCommand readCmd(ctx, addr);
	readCmd.execute();

	string data = readFileAsString("ssd_output.txt");

	EXPECT_NE("0xZEBBAEDE", data);
}

TEST_F(SddDriverTestFixture, EmptyArgument)
{
	int argc = 1;
	char* argv[1];
	argv[0] = const_cast<char*>("ssd.exe");

	vector<string> args = parseArguments(argc, argv);

	ASSERT_EQ(args.size(), 0);
}

TEST_F(SddDriverTestFixture, WriteArgument)
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

TEST_F(SddDriverTestFixture, ReadArgument)
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

