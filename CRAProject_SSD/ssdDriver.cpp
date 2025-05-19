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
		if (addr < 0 || addr >= 100) {
			handleError();
			return;
		}

		streampos writeOffset = (10 * addr);

		if (!openOrCreateNand(ios::in | ios::out)) {
			handleError();
			return;
		}

		nand.seekp(writeOffset);
		nand << value;
		nand.close();
	}
	virtual string read(int addr) {
		if (addr < 0 || addr >= 100) {
			handleError();
			return "";
		}

		streampos readOffset = 10 * addr;

		if (!openOrCreateNand(ios::in)) {
			handleError();
			return "";
		}

		std::string result(10, '\0');
		nand.read(&result[0], 10);
		nand.close();

		overwriteTextToFile("output.txt", result);

		return result;
	}

private:
	fstream nand;
	const string nandFileName = "ssd_nand.txt";

	void handleError(void) {
		overwriteTextToFile("output.txt", "ERROR");
	}

	void overwriteTextToFile(string fileName, string text)
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

	friend class SddDriverTestFixture;
};