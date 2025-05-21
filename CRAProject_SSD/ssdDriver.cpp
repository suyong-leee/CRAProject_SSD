#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>


#include "ssdContext.cpp"
#include "commandBuffer.cpp"

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

class SSDDriver {
public:
    void run(int argc, char* argv[]) {

        if (argc <= 1) return ctx.handleError();

        vector<string> fileNames = commandBufferManager.createAndReadFiles();
        vector<vector<string>> buffer = commandBufferManager.parseFileNames(fileNames);

        vector<string> args = parseArguments(argc, argv);
        string command = args[0];

        unique_ptr<Command> cmd;


        if (isValid(args) == false) {
            return ctx.handleError();
        }

        if (command == "W" || command == "E") {
            cmd = make_unique<NoopCommand>();
            //commonbuffer control
            if (buffer.size() == 5) {
                //flush
                unique_ptr<Command> flushCmd = make_unique<FlushCommand>(ctx, buffer);
                flushCmd->execute();
                commandBufferManager.eraseAll();

                //regist
                buffer.push_back({ command,args[1],args[2] });
            }
            else if (buffer.size() == 0) {
                //regist
                buffer.push_back({ command,args[1],args[2] });
            }
            else
            {
                mergeAlgorithm(args, buffer);
            }
            //end commonbuffer control
        }
        else if (command == "R") {
            string value = commandBufferManager.getCommand(args, buffer);
            if (value == "") {
                cmd = make_unique<ReadCommand>(ctx, stoi(args[1]));
            }
            else {
                cmd = make_unique<FastReadCommand>(ctx, value);
            }
        }
        else if (command == "F") {
            cmd = make_unique<FlushCommand>(ctx, buffer);
        }
        else {
            return ctx.handleError();
        }

        commandBufferManager.writeCommandBuffer(buffer);

        cmd->execute();

        if (command == "F") {
            commandBufferManager.eraseAll();
        }
    }

private:
    SSDContext ctx;
    CommandBufferManager commandBufferManager{ ctx, "./buffer" };

    static vector<string> parseArguments(int argc, char* argv[]) {
        vector<string> args;
        for (int i = 1; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return args;
    }

    void mergeAlgorithm(vector<string> args, vector<vector<string>>& buffer)
    {
        string command = args[0];
        int bufferCount = buffer.size();

        if (command == "W") {
            for (int i = 0; i < bufferCount; i++) {
                if (buffer[i][0] == "W" && buffer[i][1] == args[1]) {
                    buffer.erase(buffer.begin() + i);
                    bufferCount--;
                    i--;
                }
            }
            buffer.push_back({ command,args[1],args[2] });
        }
        else if (command == "E") {
            for (int i = 0; i < bufferCount; i++) {
                if (buffer[i][0] == "W") {
                    if ((stoi(buffer[i][1]) >= stoi(args[1])) && (stoi(buffer[i][1]) < (stoi(args[1]) + stoi(args[2])))) {
                        buffer.erase(buffer.begin() + i);
                        bufferCount--;
                        i--;
                    }
                }
            }
            bufferCount = buffer.size();
            int newStart = stoi(args[1]);
            int newEnd = stoi(args[1]) + stoi(args[2]) - 1;
            for (int i = bufferCount - 1; i >= 0; i--) {
                if (buffer[i][0] == "E") {
                    int targetStart = stoi(buffer[i][1]);
                    int targetEnd = stoi(buffer[i][1]) + stoi(buffer[i][2]) - 1;

                    if ((targetStart <= newStart) && (targetEnd <= newEnd)) {
                        targetEnd = newEnd;
                        bool merged = mergeBuffer(targetStart, targetEnd, newStart, newEnd, buffer[i]);
                        if (merged) break;
                    }
                    else if ((targetStart >= newStart) && (targetEnd <= newEnd)) {
                        targetStart = newStart;
                        targetEnd = newEnd;
                        bool merged = mergeBuffer(targetStart, targetEnd, newStart, newEnd, buffer[i]);
                        if (merged) break;
                    }
                    else if ((targetStart <= newStart) && (targetEnd >= newEnd)) {
                        targetEnd = newEnd;
                        bool merged = mergeBuffer(targetStart, targetEnd, newStart, newEnd, buffer[i]);
                        if (merged) break;
                    }
                    else if ((targetStart >= newStart) && (targetEnd >= newEnd)) {
                        targetStart = newStart;
                        bool merged = mergeBuffer(targetStart, targetEnd, newStart, newEnd, buffer[i]);
                        if (merged) break;
                    }
                }
            }
            if (newStart != -1) {
                buffer.push_back({ command,to_string(newStart),to_string(newEnd - newStart + 1) });
            }
        }
    }

    bool mergeBuffer(int targetStart, int targetEnd, int& newStart, int& newEnd, vector<string>& command)
    {
        if (targetEnd - targetStart + 1 > 10)
        {
            newStart = targetStart + 10;
            newEnd = targetEnd;
            command[1] = to_string(targetStart);
            command[2] = to_string(10);
            return false;
        }
        else
        {
            command[1] = to_string(targetStart);
            command[2] = to_string(targetEnd - targetStart + 1);
            newStart = -1;
            return true;
        }
    }

    bool isValid(vector<string> args)
    {
        string command = args[0];
        
        if (command == "W")
        {
            int addr = stoi(args[1]);
            string value = args[2];
            const string valid = "0123456789ABCDEF";
            if (addr < 0 || addr >= 100) return false;
            if (value.find("0x") != 0 || value.size() != 10) return false;

            for (int i = 2; i < 10; ++i) {
                if (valid.find(value[i]) == string::npos) return false;
            }
        }
        else if (command == "R") {
            int addr = stoi(args[1]);
            if (addr < 0 || addr >= 100) return false;
        }
        else if (command == "E") {
            int addr = stoi(args[1]);
            int value = stoi(args[2]);

            if (addr < 0 || addr >= 100) return false;
            if (addr + value < 0 || addr + value > 100) return false;
            if (value < 0 || value > 10) return false;
        }

        if (command != "W" && command != "E" && command != "F" && command != "R") return false;

        return true;
    }

    friend class SddDriverTestFixture;
};