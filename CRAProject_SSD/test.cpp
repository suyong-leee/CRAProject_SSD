#include "gmock/gmock.h"
#include "ssdDriver.cpp"

using namespace testing;
using namespace std;

class SddDriverTestFixture : public Test {
protected:
	void SetUp() override {
		ssdDriver = new SSDDriver;
		srand(static_cast<unsigned int>(time(nullptr)));
	}
public:
	SSDDriver* ssdDriver;

	vector<std::string> parseArguments(int argc, char* argv[]) {
		return SSDDriver::parseArguments(argc, argv);
	}

	void overwriteTextToFile(string fileName, string text)
	{
		return SSDDriver::overwriteTextToFile(fileName, text);
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
	string dataFromNandText = ssdDriver->read(0);
	string dataFromOutputText = readFileAsString("output.txt");

	EXPECT_EQ("0x00000000", dataFromNandText);
	EXPECT_EQ("0x00000000", dataFromOutputText);
}

TEST_F(SddDriverTestFixture, TC1correctWrite)
{
	ssdDriver->write(0, "0x11111111");
	string data = ssdDriver->read(0);

	EXPECT_EQ("0x11111111", data);
}

TEST_F(SddDriverTestFixture, TC2correctWriteSeveralTimes)
{
	for (int i = 0; i < 10; i++) {
		string hex = getHex();

		ssdDriver->write(i, hex);
		string dataFromNandText = ssdDriver->read(i);
		string dataFromOutputText = readFileAsString("output.txt");

		EXPECT_EQ(hex, dataFromNandText);
		EXPECT_EQ(hex, dataFromOutputText);
	}
}

TEST_F(SddDriverTestFixture, TC3correctWriteInOffset)
{
	for (int i = 0; i < 10; i++) {
		string hex = getHex();

		ssdDriver->write(i * 5, hex);
		string dataFromNandText = ssdDriver->read(i * 5);
		string dataFromOutputText = readFileAsString("output.txt");

		EXPECT_EQ(hex, dataFromNandText);
		EXPECT_EQ(hex, dataFromOutputText);
	}
}

TEST_F(SddDriverTestFixture, TC4WriteInWrongPosition)
{
	ssdDriver->write(120, "0x1234AAAA");

	string data = ssdDriver->read(120);
	EXPECT_NE("0x1234AAAA", data);
}

TEST_F(SddDriverTestFixture, TC5WriteWithShortInputFormat)
{
	const char* argv[] = { "ssd.exe", "W", "3", "0xBBBB" };

	ssdDriver->run(4, const_cast<char**>(argv));

	string data = ssdDriver->read(3);
	data = data.substr(0, 6);

	EXPECT_NE("0xBBBB", data);
}

TEST_F(SddDriverTestFixture, TC5WriteWithNotStart0xFormat)
{
	const char* argv[] = { "ssd.exe", "W", "3", "0XBBBBAEDE" };

	ssdDriver->run(4, const_cast<char**>(argv));

	string data = ssdDriver->read(3);

	EXPECT_NE("0XBBBBAEDE", data);
}

TEST_F(SddDriverTestFixture, TC5WriteWithNotNumber)
{
	const char* argv[] = { "ssd.exe", "W", "3", "0xZEBBAEDE" };

	ssdDriver->run(4, const_cast<char**>(argv));

	string data = ssdDriver->read(3);

	EXPECT_NE("0xZEBBAEDE", data);
}

class MockSSDDriver : public SSDDriver {
public:
	MOCK_METHOD(void, write, (int addr, string value), (override));
	MOCK_METHOD(string, read, (int addr), (override));
};

TEST(RunCommand, TC1WriteCommand)
{
	MockSSDDriver mockSsdDriver;
	const char* argv[] = { "ssd.exe", "W", "20", "0x1289CDEF" };

	EXPECT_CALL(mockSsdDriver, write(20, "0x1289CDEF"));
	mockSsdDriver.run(4, const_cast<char**>(argv));
}

TEST(RunCommand, TC2ReadCommand)
{
	MockSSDDriver mockSsdDriver;
	const char* argv[] = { "ssd.exe", "R", "20" };

	EXPECT_CALL(mockSsdDriver, read(20));
	mockSsdDriver.run(3, const_cast<char**>(argv));
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

