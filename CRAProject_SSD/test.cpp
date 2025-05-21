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

	string fastRead(vector<string> args, vector<vector<string>> buffer)
	{
		return ssdDriver->commandBufferManager.getCommand(args, buffer);
	}

	void mergeAlgorithm(vector<string> args, vector<vector<string>>& buffer)
	{
		return ssdDriver->mergeAlgorithm(args, buffer);
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

	void eraseAll(void)
	{
		return ssdDriver->commandBufferManager.eraseAll();
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
	WriteCommand writeCmd(ctx, 0, "0x11112222");
	writeCmd.execute();

	EraseCommand eraseCmd(ctx, 0, "120");
	eraseCmd.execute();

	ReadCommand readCmd(ctx, 0);
	readCmd.execute();

	string data = readFileAsString("ssd_output.txt");
	EXPECT_EQ("0x11112222", data);
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
			if (ec) ctx.handleError();
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

TEST_F(SddDriverTestFixture, FastReadExactErase)
{
	const char* argv1[] = { "ssd.exe", "E", "9", "3" };
	const char* argv2[] = { "ssd.exe", "W", "10", "0x17654897" };
	const char* argv3[] = { "ssd.exe", "R", "9" };

	ssdDriver->run(4, const_cast<char**>(argv1));
	ssdDriver->run(4, const_cast<char**>(argv2));
	ssdDriver->run(3, const_cast<char**>(argv3));

	string readResult = readFileAsString("ssd_output.txt");
	EXPECT_EQ("0x00000000", readResult);
}

TEST_F(SddDriverTestFixture, EraseDuplicatedArea)
{
	const char* argv1[] = { "ssd.exe", "E", "9", "9" };
	const char* argv2[] = { "ssd.exe", "E", "10", "2" };
	const char* argv3[] = { "ssd.exe", "R", "11" };

	ssdDriver->run(4, const_cast<char**>(argv1));
	ssdDriver->run(4, const_cast<char**>(argv2));
	ssdDriver->run(3, const_cast<char**>(argv3));

	string readResult = readFileAsString("ssd_output.txt");
	EXPECT_EQ("0x00000000", readResult);
}

TEST_F(SddDriverTestFixture, TC1FastReadNotInRange)
{
	vector<vector<string>> buffer = {
		{"E", "9", "3"},
		{"W", "10", "0x17654897"}
	};

	vector<string> args = { "R", "12" };
	string readResult = fastRead(args, buffer);

	EXPECT_EQ("", readResult);
}

TEST_F(SddDriverTestFixture, TC1FastReadExactWrite)
{
	vector<vector<string>> buffer = {
		{"E", "9", "3"},
		{"W", "10", "0x17654897"}
	};

	vector<string> args = { "R", "10" };
	string readResult = fastRead(args, buffer);

	EXPECT_EQ("0x17654897", readResult);
}

TEST_F(SddDriverTestFixture, TC1FastReadMultipleExactErase)
{
	vector<vector<string>> buffer = {
		{"W", "10", "0x17654897"},
		{"E", "10", "2"},
		{"W", "11", "0xE45ACC45"}
	};

	vector<string> args = { "R", "10" };
	string readResult = fastRead(args, buffer);

	EXPECT_EQ("0x00000000", readResult);
}

TEST_F(SddDriverTestFixture, TC1FastReadMultipleExactWrite)
{
	vector<vector<string>> buffer = {
		{"W", "10", "0x17654897"},
		{"E", "10", "2"},
		{"W", "11", "0xE45ACC45"}
	};

	vector<string> args = { "R", "11" };
	string readResult = fastRead(args, buffer);

	EXPECT_EQ("0xE45ACC45", readResult);
}

TEST_F(SddDriverTestFixture, ForceToFlush)
{
	const char* argvFlush[] = { "ssd.exe", "F" };
	const char* argv0[] = { "ssd.exe", "W", "0", "0x12345678" };
	const char* argv1[] = { "ssd.exe", "W", "1", "0x12345678" };
	const char* argv2[] = { "ssd.exe", "W", "2", "0x12345678" };
	const char* argv3[] = { "ssd.exe", "W", "3", "0x12345678" };
	const char* argv4[] = { "ssd.exe", "W", "4", "0x12345678" };
	const char* argv5[] = { "ssd.exe", "W", "5", "0x12345678" };

	ssdDriver->run(2, const_cast<char**>(argvFlush));

	ssdDriver->run(4, const_cast<char**>(argv0));
	ssdDriver->run(4, const_cast<char**>(argv1));
	ssdDriver->run(4, const_cast<char**>(argv2));
	ssdDriver->run(4, const_cast<char**>(argv3));
	ssdDriver->run(4, const_cast<char**>(argv4));
	ssdDriver->run(4, const_cast<char**>(argv5));
}

TEST_F(SddDriverTestFixture, MergeAlgorithmEraseMerged)
{
	vector<vector<string>> buffer = 
	{
		{"W", "2", "0xABABABAC"},
		{"W", "1", "0xABABABAB"},
		{"E", "4", "10"}
	};
	vector<string> args = { "E", "0", "10" };

	mergeAlgorithm(args, buffer);

	vector<vector<string>> expectedBuffer = 
	{
		{"E", "0", "10"},
		{"E", "10", "4"}
	};

	EXPECT_EQ(expectedBuffer, buffer);
}

TEST_F(SddDriverTestFixture, MergeAlgorithmWriteMergedToErase)
{
	vector<vector<string>> buffer =
	{
		{"W", "2", "0xABABABAC"},
		{"W", "1", "0xABABABAB"},
	};
	vector<string> args = { "E", "0", "10" };

	mergeAlgorithm(args, buffer);

	vector<vector<string>> expectedBuffer = 
	{
		{"E", "0", "10"}, 
	};

	EXPECT_EQ(expectedBuffer, buffer);
}

TEST_F(SddDriverTestFixture, MergeAlgorithmNoChange)
{
	vector<vector<string>> buffer =
	{
		{"W", "2", "0xABABABAC"},
		{"W", "1", "0xABABABAB"},
		{"E", "4", "10"}
	};
	vector<string> args = { "W", "10", "0x15263748" };

	mergeAlgorithm(args, buffer);

	vector<vector<string>> expectedBuffer =
	{
		{"W", "2", "0xABABABAC"},
		{"W", "1", "0xABABABAB"},
		{"E", "4", "10"},
		{ "W", "10", "0x15263748" }
	};

	EXPECT_EQ(expectedBuffer, buffer);
}

TEST_F(SddDriverTestFixture, MergeAlgorithmEraseOverwritesMultipleWritesOutOfOrder)
{
	vector<vector<string>> buffer =
	{
		{"W", "1", "0xAAAAAAA1"},
		{"W", "2", "0xAAAAAAA2"},
		{"E", "4", "4"}
	};
	vector<string> args = { "E", "4", "4" };

	mergeAlgorithm(args, buffer);

	vector<vector<string>> expectedBuffer =
	{
		{"W", "1", "0xAAAAAAA1"},
		{"W", "2", "0xAAAAAAA2"},
		{"E", "4", "4"}
	};

	EXPECT_EQ(expectedBuffer, buffer);
}

TEST_F(SddDriverTestFixture, TC1MergeArlgorithmRange)
{
	vector<vector<string>> buffer =
	{
		{"E", "1", "3"}
	};
	vector<string> args = { "E", "2", "3" };

	mergeAlgorithm(args, buffer);

	vector<vector<string>> expectedBuffer =
	{
		{"E", "1", "4"}
	};

	EXPECT_EQ(expectedBuffer, buffer);
}

TEST_F(SddDriverTestFixture, TC2MergeArlgorithmRange)
{
	vector<vector<string>> buffer =
	{
		{"E", "1", "10"}
	};
	vector<string> args = { "E", "2", "2" };

	mergeAlgorithm(args, buffer);

	vector<vector<string>> expectedBuffer =
	{
		{"E", "1", "10"}
	};

	EXPECT_EQ(expectedBuffer, buffer);
}

TEST_F(SddDriverTestFixture, TC3MergeArlgorithmRange)
{
	vector<vector<string>> buffer =
	{
		{"E", "4", "5"}
	};
	vector<string> args = { "E", "2", "5" };

	mergeAlgorithm(args, buffer);

	vector<vector<string>> expectedBuffer =
	{
		{"E", "2", "7"}
	};

	EXPECT_EQ(expectedBuffer, buffer);
}

TEST_F(SddDriverTestFixture, TC4MergeArlgorithmRange)
{
	vector<vector<string>> buffer =
	{
		{"E", "2", "3"}
	};
	vector<string> args = { "E", "1", "10" };

	mergeAlgorithm(args, buffer);

	vector<vector<string>> expectedBuffer =
	{
		{"E", "1", "10"}
	};

	EXPECT_EQ(expectedBuffer, buffer);
}

TEST_F(SddDriverTestFixture, MergeAlgorithmErasePartiallyOverwritesWrites)
{
	vector<vector<string>> buffer =
	{
		{"W", "5", "0xAAAAAAA1"},
		{"W", "8", "0xAAAAAAA2"},
	};
	vector<string> args = { "E", "5", "2" };

	mergeAlgorithm(args, buffer);

	vector<vector<string>> expectedBuffer =
	{
		{"W", "8", "0xAAAAAAA2"},
		{"E", "5", "2"}
	};

	EXPECT_EQ(expectedBuffer, buffer);
}

TEST_F(SddDriverTestFixture, MergeAlgorithmWriteOverwritesWrite)
{
	vector<vector<string>> buffer =
	{
		{"W", "10", "0xAAAAAAA1"}
	};
	vector<string> args = { "W", "10", "0xBBBBBBB2" };

	mergeAlgorithm(args, buffer);

	vector<vector<string>> expectedBuffer =
	{
		{"W", "10", "0xBBBBBBB2"}
	};

	EXPECT_EQ(expectedBuffer, buffer);
}

TEST_F(SddDriverTestFixture, MergeAlgorithmWriteToErasedAddress)
{
	vector<vector<string>> buffer =
	{
		{"E", "10", "3"}
	};
	vector<string> args = { "W", "11", "0xCAFEBABE" };

	mergeAlgorithm(args, buffer);

	vector<vector<string>> expectedBuffer =
	{
		{"E", "10", "3"},
		{"W", "11", "0xCAFEBABE"}
	};

	EXPECT_EQ(expectedBuffer, buffer);
}

TEST_F(SddDriverTestFixture, MergeAlgorithmMergePartialOverlappingErase)
{
	vector<vector<string>> buffer =
	{
		{"E", "10", "3"} 
	};
	vector<string> args = { "E", "12", "3" }; 

	mergeAlgorithm(args, buffer);

	vector<vector<string>> expectedBuffer =
	{
		{"E", "10", "5"} 
	};

	EXPECT_EQ(expectedBuffer, buffer);
}

TEST_F(SddDriverTestFixture, MergeAlgorithmMergeAdjacentErases)
{
	vector<vector<string>> buffer =
	{
		{"E", "10", "3"}
	};
	vector<string> args = { "E", "13", "2" };

	mergeAlgorithm(args, buffer);

	vector<vector<string>> expectedBuffer =
	{
		{"E", "10", "5"}
	};

	EXPECT_EQ(expectedBuffer, buffer);
}

TEST_F(SddDriverTestFixture, MergeAlgorithmWriteNoOverlap)
{
	vector<vector<string>> buffer =
	{
		{"W", "1", "0x12345678"},
		{"E", "10", "2"}
	};
	vector<string> args = { "W", "20", "0xABCDEFAB" };

	mergeAlgorithm(args, buffer);

	vector<vector<string>> expectedBuffer =
	{
		{"W", "1", "0x12345678"},
		{"E", "10", "2"},
		{"W", "20", "0xABCDEFAB"}
	};

	EXPECT_EQ(expectedBuffer, buffer);
}

TEST_F(SddDriverTestFixture, EraseWithInvalidStartBlock)
{
	const char* argv1[] = { "ssd.exe", "E", "-1", "9" };

	ssdDriver->run(4, const_cast<char**>(argv1));

	string readResult = readFileAsString("ssd_output.txt");
	EXPECT_EQ("ERROR", readResult);
}

TEST_F(SddDriverTestFixture, EraseWithStartBlockOutOfRange)
{
	const char* argv1[] = { "ssd.exe", "E", "101", "9" };

	ssdDriver->run(4, const_cast<char**>(argv1));

	string readResult = readFileAsString("ssd_output.txt");
	EXPECT_EQ("ERROR", readResult);
}

TEST_F(SddDriverTestFixture, EraseWithInvalidBlockRange)
{
	const char* argv1[] = { "ssd.exe", "E", "99", "9" };

	ssdDriver->run(4, const_cast<char**>(argv1));

	string readResult = readFileAsString("ssd_output.txt");
	EXPECT_EQ("ERROR", readResult);
}

TEST_F(SddDriverTestFixture, EraseWithBlockRangeExceedingLimit)
{
	const char* argv1[] = { "ssd.exe", "E", "5", "11" };

	ssdDriver->run(4, const_cast<char**>(argv1));

	string readResult = readFileAsString("ssd_output.txt");
	EXPECT_EQ("ERROR", readResult);
}

TEST_F(SddDriverTestFixture, EraseWithNegativeBlockCount)
{
	const char* argv1[] = { "ssd.exe", "E", "5", "-1" };

	ssdDriver->run(4, const_cast<char**>(argv1));

	string readResult = readFileAsString("ssd_output.txt");
	EXPECT_EQ("ERROR", readResult);
}

TEST_F(SddDriverTestFixture, EraseAndFlushWithValidInput)
{
	eraseAll();

	const char* argv1[] = { "ssd.exe", "E", "0", "0" };
	const char* argv2[] = { "ssd.exe", "F" };

	ssdDriver->run(4, const_cast<char**>(argv1));
	ssdDriver->run(2, const_cast<char**>(argv2));

	string readResult = readFileAsString("ssd_output.txt");
	EXPECT_EQ("", readResult);
}

TEST_F(SddDriverTestFixture, ReadWithNegativePageNumber)
{
	const char* argv1[] = { "ssd.exe", "R", "-1" };

	ssdDriver->run(3, const_cast<char**>(argv1));

	string readResult = readFileAsString("ssd_output.txt");
	EXPECT_EQ("ERROR", readResult);
}

TEST_F(SddDriverTestFixture, ReadWithPageNumberOutOfRange)
{
	const char* argv1[] = { "ssd.exe", "R", "100" };

	ssdDriver->run(3, const_cast<char**>(argv1));

	string readResult = readFileAsString("ssd_output.txt");
	EXPECT_EQ("ERROR", readResult);
}

TEST_F(SddDriverTestFixture, WriteWithNegativePageNumber)
{
	const char* argv1[] = { "ssd.exe", "W", "-1", "0x12345678" };

	ssdDriver->run(4, const_cast<char**>(argv1));

	string readResult = readFileAsString("ssd_output.txt");
	EXPECT_EQ("ERROR", readResult);
}

TEST_F(SddDriverTestFixture, WriteWithPageNumberOutOfRange)
{
	const char* argv1[] = { "ssd.exe", "W", "100", "0x12345678" };

	ssdDriver->run(4, const_cast<char**>(argv1));

	string readResult = readFileAsString("ssd_output.txt");
	EXPECT_EQ("ERROR", readResult);
}

TEST_F(SddDriverTestFixture, WriteWithInvalidHexCharacters)
{
	const char* argv1[] = { "ssd.exe", "W", "5", "0xZf1" };

	ssdDriver->run(4, const_cast<char**>(argv1));

	string readResult = readFileAsString("ssd_output.txt");
	EXPECT_EQ("ERROR", readResult);
}

TEST_F(SddDriverTestFixture, WriteWithNonHexadecimalData)
{
	const char* argv1[] = { "ssd.exe", "W", "5", "13234" };

	ssdDriver->run(4, const_cast<char**>(argv1));

	string readResult = readFileAsString("ssd_output.txt");
	EXPECT_EQ("ERROR", readResult);
}

TEST_F(SddDriverTestFixture, WriteWithValidHexButInvalidPattern)
{
	const char* argv1[] = { "ssd.exe", "W", "5", "0xf2f2f2f2" };

	ssdDriver->run(4, const_cast<char**>(argv1));

	string readResult = readFileAsString("ssd_output.txt");
	EXPECT_EQ("ERROR", readResult);
}

TEST_F(SddDriverTestFixture, WrongCommandExecution)
{
	const char* argv1[] = { "ssd.exe", "S" };

	ssdDriver->run(2, const_cast<char**>(argv1));

	string readResult = readFileAsString("ssd_output.txt");
	EXPECT_EQ("ERROR", readResult);
}