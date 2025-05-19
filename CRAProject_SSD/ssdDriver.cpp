#include <vector>
#include <string>
#include <fstream>

using namespace std;

class SSDDriver {
public:
	virtual void run(int argc, char* argv[])
	{
		if (argc <= 1) return;

		vector<string> args = parseArguments(argc, argv);
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

		nand.open(nandFileName, std::ios::in | std::ios::out);
		if (!nand.is_open()) {
			std::ofstream createFile(nandFileName);
			createFile.close();
			nand.open(nandFileName, std::ios::in | std::ios::out);
		}
		if (!nand.is_open()) {
			makeError();
			return;
		}
		nand.seekp(writeOffset);
		nand << value;
		nand.close();
	}
	virtual string read(int addr) {
		if (addr < 0 || addr >= 100) {
			makeError();
			return "";
		}

		streampos readOffset = 10 * addr;

		nand.open(nandFileName, ios::in);
		if (!nand.is_open()) {
			makeError();
			return "";
		}

		nand.seekg(readOffset);
		char buffer[11] = { 0 };
		nand.read(buffer, 10);
		nand.close();

		ofstream output("output.txt");
		if (output.is_open() == false) {
			return "";
		}
		output << std::string(buffer);
		output.close();

		return std::string(buffer);
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

	static vector<string> parseArguments(int argc, char* argv[])
	{
		vector<string> args;
		for (int i = 1; i < argc; ++i)
		{
			args.emplace_back(argv[i]);
		}

		return args;
	}

	friend class SSDDriverTest;
};