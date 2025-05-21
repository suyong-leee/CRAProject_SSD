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
    FlushCommand(SSDContext& context, vector<vector<string>> &buffer)
        : ctx(context), cmdbuffer(buffer) {
    }
    void execute() override {
        for (int i = 0;i < cmdbuffer.size(); i++) {
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
        int addr = 0;
        if (command != "F") {
            addr = stoi(args[1]);
        }

        unique_ptr<Command> cmd;
        
        //erase 가드절 생성
        if (command == "E" && args[2] == "0")
        {
            return;
        }

        try {
            if (command == "W" || command == "E") {
                //commonbuffer control
                int bufferCount = buffer.size();
                if (bufferCount == 5) {
                    //flush
                    cmd = make_unique<FlushCommand>(ctx, buffer);
                    cmd->execute();

                    //regist
                    buffer.push_back({command,args[1],args[2]});
                }
                else if (bufferCount == 0) {
                    //regist
                    buffer.push_back({ command,args[1],args[2] });
                }
                else
                {
                    if (command == "W") {
                        for (int i = bufferCount - 1; i >= 0; i--) {
                            if (buffer[i][0] == "W" && buffer[i][1] == args[1]) {
                                buffer.erase(buffer.begin() + i);
                            }
                        }
                        buffer.push_back({ command,args[1],args[2] });
                    }
                    else if (command == "E") {
                        for (int i = bufferCount - 1; i >= 0; i--) {
                            if (buffer[i][0] == "W") {
                                if ((stoi(buffer[i][1]) >= stoi(args[1])) && (stoi(buffer[i][1]) < (stoi(args[1]) + stoi(args[2])))) {
                                    buffer.erase(buffer.begin() + i);
                                }
                            }
                        }
                        int newStart = stoi(args[1]);
                        int newEnd = stoi(args[1]) + stoi(args[2]) - 1;
                        for (int i = bufferCount - 1; i >= 0; i--) {
                            if (buffer[i][0] == "E") {
                                int targetStart = stoi(buffer[i][1]);
                                int targetEnd = stoi(buffer[i][1]) + stoi(buffer[i][2]) - 1;

                                if (targetEnd - targetStart + 1 == 10) {
                                    continue;
                                }
                                if ((targetStart <= newStart) && (targetEnd <= newEnd)) {
                                    targetEnd = newEnd;
                                    if (targetEnd - targetStart + 1 > 10)
                                    {
                                        newStart = targetStart + 10;
                                        newEnd = targetEnd;
                                        buffer[i][1] = to_string(targetStart);
                                        buffer[i][2] = to_string(10);
                                    }
                                    else
                                    {
                                        buffer[i][1] = to_string(targetStart);
                                        buffer[i][2] = to_string(targetEnd - targetStart + 1);
                                        newStart = -1;
                                        break;
                                    }
                                }
                                else if ((targetStart >= newStart) && (targetEnd <= newEnd)) {
                                    targetStart = newStart;
                                    targetEnd = newEnd;
                                    if (targetEnd - targetStart + 1 > 10)
                                    {
                                        newStart = targetStart + 10;
                                        newEnd = targetEnd;
                                        buffer[i][1] = to_string(targetStart);
                                        buffer[i][2] = to_string(10);
                                    }
                                    else
                                    {
                                        buffer[i][1] = to_string(targetStart);
                                        buffer[i][2] = to_string(targetEnd - targetStart + 1);
                                        newStart = -1;
                                        break;
                                    }
                                }
                                else if ((targetStart <= newStart) && (targetEnd >= newEnd)) {
                                    targetEnd = newEnd;
                                    if (targetEnd - targetStart + 1 > 10)
                                    {
                                        newStart = targetStart + 10;
                                        newEnd = targetEnd;
                                        buffer[i][1] = to_string(targetStart);
                                        buffer[i][2] = to_string(10);
                                    }
                                    else
                                    {
                                        buffer[i][1] = to_string(targetStart);
                                        buffer[i][2] = to_string(targetEnd - targetStart + 1);
                                        newStart = -1;
                                        break;
                                    }
                                }
                                else if ((targetStart >= newStart) && (targetEnd >= newEnd)) {
                                    targetStart = newStart;
                                    if (targetEnd - targetStart + 1 > 10)
                                    {
                                        newStart = targetStart + 10;
                                        newEnd = targetEnd;
                                        buffer[i][1] = to_string(targetStart);
                                        buffer[i][2] = to_string(10);
                                    }
                                    else
                                    {
                                        buffer[i][1] = to_string(targetStart);
                                        buffer[i][2] = to_string(targetEnd - targetStart + 1);
                                        newStart = -1;
                                        break;
                                    }
                                }

                            }
                        }
                        if (newStart != -1) {
                            buffer.push_back({ command,to_string(newStart),to_string(newEnd - newStart + 1) });
                        }
                    }
                    else { //불가능한 케이스이긴함...
                        return ctx.handleError();
                    }
                }
                //end commonbuffer control
            }
            else if (command == "R") {
                if (fastRead(args, buffer) == false) {
                    cmd = make_unique<ReadCommand>(ctx, addr);
                }
            }
            else if (command == "F") { // temp code
                cmd = make_unique<FlushCommand>(ctx, buffer);
            }
            else if (command == "F") {
                cmd = make_unique<FlushCommand>(ctx, buffer);
            }
            else {
                return ctx.handleError();
            }
            
            commandBufferManager.writeCommandBuffer(buffer);
        }
        catch (...) {
            return ctx.handleError();
        }

        cmd->execute();
    }

private:
    SSDContext ctx;
    CommandBufferManager commandBufferManager{ctx, "./buffer" };

    static vector<string> parseArguments(int argc, char* argv[]) {
        vector<string> args;
        for (int i = 1; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return args;
    }

    bool fastRead(vector<string> args, vector<vector<string>> buffer)
    {
        int addr = stoi(args[1]);
        for (int i = (int)buffer.size() - 1; i >= 0; i--) {
            string command = buffer[i][0];
            if (command != "W" && command != "E") continue;

            int commandAddr = stoi(buffer[i][1]);
            int commandAddrSize = command == "E" ? stoi(buffer[i][2]) : 1;
            if (addr < commandAddr || addr >= commandAddr + commandAddrSize) continue;

            string value = command == "E" ? "0x00000000" : buffer[i][2];

            ctx.overwriteTextToFile("ssd_output.txt", value);
            return true;
        }

        return false;
    }


    friend class SddDriverTestFixture;
};