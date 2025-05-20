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

	bool fastRead(vector<string> args, vector<vector<string>> buffer)
	{
		return ssdDriver->fastRead(args, buffer);
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

TEST_F(SddDriverTestFixture, EraseSuccess)
{
	const char *args[3] = {"E", "0", "1"};

	WriteCommand writeCmd(ctx, 0, "0x11111111");
	writeCmd.execute();
	
	EraseCommand eraseCmd(ctx, 0, "1");
	eraseCmd.execute();
	
	ReadCommand readCmd(ctx, 0);
	readCmd.execute();

	string data = readFileAsString("ssd_output.txt");
	EXPECT_EQ("0x00000000", data);
}

TEST_F(SddDriverTestFixture, EraseFailOutOfRange)
{
	const char* args[3] = { "E", "0", "120" };

	WriteCommand writeCmd(ctx, 0, "0x11111111");
	writeCmd.execute();

	ssdDriver->run(3, const_cast<char**>(args));

	ReadCommand readCmd(ctx, 0);
	readCmd.execute();

	string data = readFileAsString("ssd_output.txt");
	EXPECT_EQ("0x11111111", data);
}

TEST_F(SddDriverTestFixture, EraseSuccessInManyPages)
{
	for (int i = 0; i < 10; ++i) {
		string hex = getHex();

		WriteCommand writeCmd(ctx, i, hex);
		writeCmd.execute();
	}

	EraseCommand eraseCmd(ctx, 0, "5");
	eraseCmd.execute();

	for (int i = 0; i < 5; ++i) {
		ReadCommand readCmd(ctx, 0);
		readCmd.execute();

		string data = readFileAsString("ssd_output.txt");
		EXPECT_EQ("0x00000000", data);
	}
}

TEST_F(SddDriverTestFixture, TC1CommandBufferTest)
{
	// Delete All Files in Directory
	path dirPath("./buffer");
	for (const auto& entry : directory_iterator(dirPath)) {
		if (entry.is_regular_file()) {
			std::error_code ec;
			remove(entry.path(), ec);
			if (ec) return ctx.handleError();
		}
	}
	const char* argv1[] = { "ssd.exe", "E", "9", "3" };
	int argc1 = 4;

	ssdDriver->run(argc1, const_cast<char**>(argv1));

	const char* argv2[] = { "ssd.exe", "W", "10", "0xAB12CD34" };
	int argc2 = 4;

	ssdDriver->run(argc2, const_cast<char**>(argv2));

	const char* argv3[] = { "ssd.exe", "W", "13", "0xE5E5E5E5" };
	int argc3 = 4;

	ssdDriver->run(argc3, const_cast<char**>(argv3));

	// Read All Files in Directory
	int correctFileNamesIdx = 0;
	vector<string> correctFileNames = { "1_E_9_3", "2_W_10_0xAB12CD34", "3_W_13_0xE5E5E5E5", "4_empty", "5_empty" };
	for (const auto& entry : directory_iterator(dirPath)) {
		if (!entry.is_regular_file()) continue;

		string fileName = entry.path().filename().string();
		EXPECT_EQ(correctFileNames[correctFileNamesIdx++], fileName);
	}
}

TEST_F(SddDriverTestFixture, TC1FastReadExactErase)
{
	vector<vector<string>> buffer = {
		{"E", "9", "3"},
		{"W", "10", "0x17654897"}
	};

	vector<string> args = { "R", "9" };
	bool readResult = fastRead(args, buffer);
	string outputResult = readFileAsString("ssd_output.txt");

	EXPECT_EQ(true, readResult);
	EXPECT_EQ("0x00000000", outputResult);
}

TEST_F(SddDriverTestFixture, TC1FastReadProxyErase)
{
	vector<vector<string>> buffer = {
		{"E", "9", "3"},
		{"W", "10", "0x17654897"}
	};

	vector<string> args = { "R", "11" };
	bool readResult = fastRead(args, buffer);
	string outputResult = readFileAsString("ssd_output.txt");

	EXPECT_EQ(true, readResult);
	EXPECT_EQ("0x00000000", outputResult);
}

TEST_F(SddDriverTestFixture, TC1FastReadNotInRange)
{
	vector<vector<string>> buffer = {
		{"E", "9", "3"},
		{"W", "10", "0x17654897"}
	};

	vector<string> args = { "R", "12" };
	bool readResult = fastRead(args, buffer);

	EXPECT_EQ(false, readResult);
}


TEST_F(SddDriverTestFixture, TC1FastReadExactWrite)
{
	vector<vector<string>> buffer = {
		{"E", "9", "3"},
		{"W", "10", "0x17654897"}
	};

	vector<string> args = { "R", "10" };
	bool readResult = fastRead(args, buffer);
	string outputResult = readFileAsString("ssd_output.txt");

	EXPECT_EQ(true, readResult);
	EXPECT_EQ("0x17654897", outputResult);
}

TEST_F(SddDriverTestFixture, TC1FastReadMultipleExactErase)
{
	vector<vector<string>> buffer = {
		{"W", "10", "0x17654897"},
		{"E", "10", "2"},
		{"W", "11", "0xE45ACC45"}
	};

	vector<string> args = { "R", "10" };
	bool readResult = fastRead(args, buffer);
	string outputResult = readFileAsString("ssd_output.txt");

	EXPECT_EQ(true, readResult);
	EXPECT_EQ("0x00000000", outputResult);
}

TEST_F(SddDriverTestFixture, TC1FastReadMultipleExactWrite)
{
	vector<vector<string>> buffer = {
		{"W", "10", "0x17654897"},
		{"E", "10", "2"},
		{"W", "11", "0xE45ACC45"}
	};

	vector<string> args = { "R", "11" };
	bool readResult = fastRead(args, buffer);
	string outputResult = readFileAsString("ssd_output.txt");

	EXPECT_EQ(true, readResult);
	EXPECT_EQ("0xE45ACC45", outputResult);
}