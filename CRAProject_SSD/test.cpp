#include "gmock/gmock.h"
#include "sddDriver.cpp"

using namespace testing;
using namespace std;

class SddDriverTestFixture : public Test {
protected:
	void SetUp() override {
		ssdDriver = new SSDDriver;
	}
public:
	SSDDriver * ssdDriver;
};

TEST_F(SddDriverTestFixture, TC1correctWrite)
{
	ssdDriver->write(0, "0x11111111");
	string data = ssdDriver->read(0);

	EXPECT_EQ("0x11111111", data);
}

TEST_F(SddDriverTestFixture, TC2correctWriteSeveralTimes)
{
	for (int i = 0;i < 10;i++) {
		ssdDriver->write(i, "0x12344567");
	}
	for (int i = 0; i < 10; i++) {
		string data = ssdDriver->read(i);
		EXPECT_EQ("0x12344567", data);
	}
}

TEST_F(SddDriverTestFixture, TC3correctWriteInOffset)
{
	for (int i = 0;i < 10;i++) {
		ssdDriver->write(i*5, "0x12222222");
	}
	for (int i = 0; i < 10; i++) {
		string data = ssdDriver->read(i*5);
		EXPECT_EQ("0x12222222", data);
	}
}

TEST_F(SddDriverTestFixture, TC4WriteInWrongPosition)
{
	ssdDriver->write(120, "0x1234AAAA");
	
	//TODO: check value by read function
	EXPECT_EQ(1, 1);
}

class MockSSDDriver : public SSDDriver {
public:
	MOCK_METHOD(void, write, (int addr, string value), (override));
	MOCK_METHOD(string, read, (int addr), (override));
			
};
class SSDDriverTest : public ::testing::Test
{
public:
	std::vector<std::string> parseArguments(int argc, char* argv[]) {
		return SSDDriver::parseArguments(argc, argv);
	}

	friend class SSDDriver;
};

TEST(RunCommand, TC1WriteCommand)
{
	MockSSDDriver mockSsdDriver;
	const char* argv[] = {"ssd.exe", "W", "20", "0x1289CDEF" };

	EXPECT_CALL(mockSsdDriver, write(20, "0x1289CDEF"));
	mockSsdDriver.run(4, const_cast<char**>(argv));
}

TEST(RunCommand, TC2ReadCommand)
{
	MockSSDDriver mockSsdDriver;
	const char* argv[] = {"ssd.exe", "R", "20" };

	EXPECT_CALL(mockSsdDriver, read(20));
	mockSsdDriver.run(3, const_cast<char**>(argv));
}

TEST_F(SSDDriverTest, EmptyArgument)
{
	int argc = 1;
	char* argv[1];
	argv[0] = const_cast<char*>("ssd.exe");
	
	vector<string> args = parseArguments(argc, argv);

	ASSERT_EQ(args.size(), 0);
}

TEST_F(SSDDriverTest, WriteArgument)
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

TEST_F(SSDDriverTest, ReadArgument)
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