#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

using namespace std;
using namespace std::filesystem;

class Command {
public:
	virtual ~Command() = default;
	virtual void execute() = 0;

	const int LBA_MAX = 100;
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
		if (addr < 0 || addr >= LBA_MAX) return -1;
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
	    if (addr < 0 || addr >= LBA_MAX) return ctx.handleError();
	    if (!ctx.openOrCreateNand(ios::in)) return ctx.handleError();

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

class EraseCommand : public Command {
public:
	EraseCommand(SSDContext& context, int addr, string size)
		: ctx(context), addr(addr) {
		eraseSize = stoi(size);
	}
	void execute() override {
		if ((addr < 0 || addr >= LBA_MAX) ||
			(addr + eraseSize > LBA_MAX) ||
			(!ctx.openOrCreateNand(ios::in | ios::out))) {
			return ctx.handleError();
		}

		for (int offsetIdx = 0; offsetIdx < eraseSize; offsetIdx++) {
			streampos offsetBase = 10 * (addr + offsetIdx);
			ctx.nand.seekp(offsetBase);
			ctx.nand << "0x00000000";
		}
		ctx.nand.close();
	}
private:
	SSDContext& ctx;
	int addr;
	int eraseSize;
};

class NoopCommand : public Command
{
public:
	void execute() override {}
};

class SSDDriver {
public:
	void run(int argc, char* argv[]) {

		if (argc <= 1) return ctx.handleError();

		vector<string> fileNames = createAndReadFiles();
		vector<vector<string>> buffer = parseFileNamse(fileNames);

		vector<string> args = parseArguments(argc, argv);
		string command = args[0];
		int addr = stoi(args[1]);

		unique_ptr<Command> cmd;
		
		try {
			if (command == "W") {
				//cmd = make_unique<NoopCommand>();
				cmd = make_unique<WriteCommand>(ctx, addr, args[2]);
				addCommandBuffer(buffer, args);

				writeCommandBuffer(buffer);
			}
			else if (command == "R") {
				cmd = make_unique<ReadCommand>(ctx, addr);
			}
			else if (command == "E") {
				//cmd = make_unique<NoopCommand>();
				cmd = make_unique<EraseCommand>(ctx, addr, args[2]);
				addCommandBuffer(buffer, args);

				writeCommandBuffer(buffer);
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

	vector<string> createAndReadFiles(void)
	{
		vector<string> names;
		try {
			string dirPath = "./buffer";
	
			// Check And Create
			path p(dirPath);
			if (!exists(p)) {
				if (!create_directory(p)) {
					throw runtime_error("Failed to create directory: " + dirPath);
				}
			}

			vector<string> checkFiles;
			for (const auto& entry : directory_iterator(dirPath)) {
				if (entry.is_directory()) {
					checkFiles.push_back(entry.path().filename().string());
				}
			}

			if (checkFiles.size() == 0) {
				for (int i = 1; i <= 5; ++i) {
					string fileName = to_string(i) + "_empty";
					path filePath = p / fileName;

					ofstream ofs(filePath);
					if (!ofs) {
						throw runtime_error("Failed to create file: " + filePath.string());
					}
				}
			}

			// Read
			for (const auto& entry : directory_iterator(dirPath)) {
				if (!entry.is_regular_file()) continue;

				names.push_back(entry.path().filename().string());
			}
		}
		catch (const exception& err) {
			ctx.handleError();
			return vector<string>();
		}

		return names;
	}

	vector<string> split(const string& str, char delimiter = '_') {
		vector<string> tokens;
		string token;
		stringstream ss(str);

		while (getline(ss, token, delimiter)) {
			tokens.push_back(token);
		}

		return tokens;
	}

	vector<vector<string>> parseFileNamse(vector<string> fileNames)
	{
		vector<vector<string>> result;
		for (const string& fileName : fileNames)
		{
			vector<string> fileNameSpliteed = split(fileName);
			fileNameSpliteed.erase(fileNameSpliteed.begin());

			if (fileNameSpliteed[0] == "empty") continue;

			result.push_back(fileNameSpliteed);
		}

		return result;
	}

	void addCommandBuffer(vector<vector<string>>& buffer, vector<string> args)
	{
		buffer.push_back(args);
	}

	string joinStrings(const vector<string> vec, const string delimiter = "_") {
		string result;
		for (size_t i = 0; i < vec.size(); ++i) {
			result += vec[i];
			if (i != vec.size() - 1) {
				result += delimiter;
			}
		}
		return result;
	}

	void writeCommandBuffer(vector<vector<string>> buffer)
	{
		while (buffer.size() < 5) buffer.push_back(vector<string>({ "empty" }));

		path dirPath = "./buffer";
		for (const auto& entry : directory_iterator(dirPath)) {
			if (entry.is_regular_file()) {
				std::error_code ec;
				remove(entry.path(), ec);
				if (ec) return ctx.handleError();
			}
		}

		for (int i = 0; i < buffer.size(); i++) {
			string fileName = to_string(i + 1) + "_" +  joinStrings(buffer[i]);

			path filePath = dirPath / fileName;

			ofstream ofs(filePath);
			if (!ofs) return ctx.handleError();
			
			ofs.close();
		}
	}


	friend class SddDriverTestFixture;
};