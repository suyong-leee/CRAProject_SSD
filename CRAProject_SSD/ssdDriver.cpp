#include <vector>
#include <string>
#include <fstream>

using namespace std;

class Command {
public:
	virtual ~Command() = default;
	virtual void execute() = 0;
};

struct SSDContext {
	fstream nand;
	const string nandFileName = "ssd_nand.txt";

	static void overwriteTextToFile(const string& fileName, const string& text) {
		ofstream file(fileName);
		if (!file.is_open()) return;
		
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

	string handleErrorReturn() {
		overwriteTextToFile("ssd_output.txt", "ERROR");
		return "";
	}

	void handleError() {
		overwriteTextToFile("ssd_output.txt", "ERROR");
	}
};

class WriteCommand : public Command {
public:
	WriteCommand(SSDContext& context, int addr, const string& value)
		: ctx(context), addr(addr), value(value) {
	}

	void execute() override {
		if (checkInvalidInputForWrite() < 0) return ctx.handleError();
		if (!ctx.openOrCreateNand(ios::in | ios::out)) return ctx.handleError();

		streampos offset = 10 * addr;
		ctx.nand.seekp(offset);
		ctx.nand << value;
		ctx.nand.close();
	}

private:
	SSDContext& ctx;
	int addr;
	string value;

	int checkInvalidInputForWrite() {
		const string valid = "0123456789ABCDEF";
		if (addr < 0 || addr >= 100) return -1;
		if (value.find("0x") != 0 || value.size() != 10) return -1;

		for (int i = 2; i < 10; ++i) {
			if (valid.find(value[i]) == string::npos) return -1;
		}
		return 0;
	}
};

class ReadCommand : public Command{
public:
	ReadCommand(SSDContext & context, int addr)
		: ctx(context), addr(addr) {
}

void execute() override {
	if (addr < 0 || addr >= 100) return (void)ctx.handleErrorReturn();
	if (!ctx.openOrCreateNand(ios::in)) return (void)ctx.handleErrorReturn();

	string readResult(10, '\0');
	streampos offset = 10 * addr;

	ctx.nand.seekp(offset);
	ctx.nand.read(&readResult[0], 10);
	streamsize bytesRead = ctx.nand.gcount();
	ctx.nand.close();

	string output = (bytesRead == 0) ? "0x00000000" : readResult;
	ctx.overwriteTextToFile("ssd_output.txt", output);
}

private:
	SSDContext & ctx;
	int addr;
};

class SSDDriver {
public:
	void run(int argc, char* argv[]) {
		if (argc <= 1) return ctx.handleError();

		vector<string> args = parseArguments(argc, argv);
		string command = args[0];
		int addr = stoi(args[1]);

		unique_ptr<Command> cmd;

		try {
			if (command == "W") {
				cmd = make_unique<WriteCommand>(ctx, addr, args[2]);
			}
			else if (command == "R") {
				cmd = make_unique<ReadCommand>(ctx, addr);
			}
			else {
				return ctx.handleError();
			}
		}
		catch (...) {
			return ctx.handleError();
		}

		cmd->execute();
	}

private:
	SSDContext ctx;

	static vector<string> parseArguments(int argc, char* argv[]) {
		vector<string> args;
		for (int i = 1; i < argc; ++i) {
			args.emplace_back(argv[i]);
		}
		return args;
	}

	friend class SddDriverTestFixture;
};