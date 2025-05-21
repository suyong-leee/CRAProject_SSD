#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "ssdContext.cpp"

using namespace std;
using namespace std::filesystem;

class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;

    const int LBA_MAX = 100;
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

class FastReadCommand : public Command {
public:
    FastReadCommand(SSDContext& context, string& value)
        : ctx(context), value(value) {
    }

    void execute() override {
        ctx.overwriteTextToFile("ssd_output.txt", value);
    }

private:
    SSDContext& ctx;
    string value;
};

class ReadCommand : public Command {
public:
    ReadCommand(SSDContext& context, int addr)
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
    SSDContext& ctx;
    int addr;
};

class EraseCommand : public Command {
public:
    EraseCommand(SSDContext& context, int addr, string size)
        : ctx(context), addr(addr) {
        eraseSize = atoi(size.c_str());
    }
    void execute() override {
        if ((addr < 0 || addr >= LBA_MAX) ||
            (addr + eraseSize > LBA_MAX) ||
            (!ctx.openOrCreateNand(ios::in | ios::out))) {
            ctx.handleError();
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

class FlushCommand : public Command {
public:
    FlushCommand(SSDContext& context, vector<vector<string>>& buffer)
        : ctx(context), cmdbuffer(buffer) {
    }
    void execute() override {
        for (int i = 0; i < cmdbuffer.size(); i++) {
            if (cmdbuffer[i][0] == "W") {
                int addr = stoi(cmdbuffer[i][1]);
                unique_ptr<Command> cmd = make_unique<WriteCommand>(ctx, addr, cmdbuffer[i][2]);
                cmd->execute();
            }
            else if (cmdbuffer[i][0] == "E") {
                int addr = stoi(cmdbuffer[i][1]);
                unique_ptr<Command> cmd = make_unique<EraseCommand>(ctx, addr, cmdbuffer[i][2]);
                cmd->execute();
            }
        }
        cmdbuffer.clear();
        return;
    }

private:
    vector<vector<string>> cmdbuffer;
    SSDContext& ctx;
};

class NoopCommand : public Command
{
public:
    void execute() override {}
};