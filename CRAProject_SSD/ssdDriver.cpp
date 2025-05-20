#include <vector>
#include <string>
#include <fstream>

using namespace std;

class SSDDriver {
public:
	virtual void run(int argc, char* argv[])
	{
		if (argc <= 1)
		{
			handleError();
			return;
		}

		vector<string> args = parseArguments(argc, argv);
		string command = args[0];
		int addr = stoi(args[1]);

		if (command == "W")
		{
			write(addr, string(args[2]));
		}
		else if (command == "R")
		{
			read(addr);
		}
	}
	virtual void write(int addr, string value) {
		if (checkInvalidInputForWrite(addr, value) < 0) return handleError();
		if (!openOrCreateNand(ios::in | ios::out)) return handleError();

		streampos writeOffset = 10 * addr;

		nand.seekp(writeOffset);
		nand << value;
		nand.close();
	}
	virtual string read(int addr) {
		if (addr < 0 || addr >= 100) return handleErrorReturn();
		if (!openOrCreateNand(ios::in)) return handleErrorReturn();

		streampos readOffset = 10 * addr;

		string readResult(10, '\0');
		nand.seekp(readOffset);
		nand.read(&readResult[0], 10);
		std::streamsize bytesRead = nand.gcount();
		nand.close();

		string output = bytesRead == 0 ? "0x00000000" : readResult;
		overwriteTextToFile("output.txt", output);

		return output;
	}

private:
	fstream nand;
	const string nandFileName = "ssd_nand.txt";
	const string validNunber = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

	string handleErrorReturn(void)
	{
		overwriteTextToFile("output.txt", "ERROR");
		return "";
	}

	void handleError(void) {
		overwriteTextToFile("output.txt", "ERROR");
	}

	static void overwriteTextToFile(string fileName, string text)
	{
		ofstream file(fileName);
		if (file.is_open() == false) {
			return;
		}
		file << text;
		file.close();
	}

	bool openOrCreateNand(ios_base::openmode mode) {
		nand.open(nandFileName, mode);
		if (!nand.is_open()) {
			ofstream createFile(nandFileName);
			createFile.close();
			nand.open(nandFileName, mode);
		}
		return nand.is_open();
	}

	static vector<string> parseArguments(int argc, char* argv[])
	{
		vector<string> args;
		for (int i = 1; i < argc; ++i)
		{
			args.emplace_back(argv[i]);
		}

		return args;
	}

	int checkInvalidInputForWrite(int addr, string value)
	{
		if (addr < 0 || addr >= 100) {
			return -1;
		}
		if (value.find("0x", 0) == string::npos || value.size() != 10) {
			return -1;
		}

		int arrayIdx;
		int validNumIdx;
		for (arrayIdx = 2; arrayIdx < 10; arrayIdx++) {
			for (validNumIdx = 0; validNumIdx < 16;validNumIdx++) {
				if (validNunber[validNumIdx] == value[arrayIdx]) {
					break;
				}
			}
			if (validNumIdx == 16) {
				return -1;
			}
		}

		return 0;
	}

	friend class SddDriverTestFixture;
};