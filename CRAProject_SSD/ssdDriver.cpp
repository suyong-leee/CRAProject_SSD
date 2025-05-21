#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "ssdContext.cpp"
#include "commandBuffer.cpp"
#include "command.cpp"

using namespace std;
using namespace std::filesystem;

class SSDDriver {
public:
    void run(int argc, char* argv[]) {

        if (argc <= 1) return ctx.handleError();

        vector<string> fileNames = commandBufferManager.createAndReadFiles();
        buffer = commandBufferManager.parseFileNames(fileNames);
        vector<string> args = parseArguments(argc, argv);


        if (isValid(args) == false) {
            return ctx.handleError();
        }

        unique_ptr<Command> cmd = make_unique<FlushCommand>(ctx, buffer);
        string command = args[0];
        if (command == "W" || command == "E") {
            cmd = processWE(args);
        }
        else if (command == "R") {
            cmd = processR(args);
        }
        else if (command == "F") {
            cmd = processF(args);
        }
        else {
            return ctx.handleError();
        }

        cmd->execute();
    }

private:
    SSDContext ctx;
    CommandBufferManager commandBufferManager{ ctx, "./buffer" };
    vector<vector<string>> buffer;

    static vector<string> parseArguments(int argc, char* argv[]) {
        vector<string> args;
        for (int i = 1; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }
        return args;
    }

    unique_ptr<Command> processWE(vector<string> args)
    {
        unique_ptr<Command> cmd = make_unique<NoopCommand>();
        if (buffer.size() == 5) {
            cmd = make_unique<FlushCommand>(ctx, buffer);
            commandBufferManager.writeCommandBuffer({ args });
        }
        else if (buffer.size() == 0) {
            buffer.push_back({ args });
            commandBufferManager.writeCommandBuffer(buffer);
        }
        else
        {
            mergeAlgorithm(args, buffer);
            commandBufferManager.writeCommandBuffer(buffer);
        }
        return cmd;
    }

    unique_ptr<Command> processR(vector<string> args)
    {
        unique_ptr<Command> cmd = make_unique<NoopCommand>();
        string value = commandBufferManager.getCommand(args, buffer);
        if (value == "") {
            cmd = make_unique<ReadCommand>(ctx, stoi(args[1]));
        }
        else {
            cmd = make_unique<FastReadCommand>(ctx, value);
        }
        return cmd;
    }

    unique_ptr<Command> processF(vector<string> args)
    {
        unique_ptr<Command> cmd = make_unique<NoopCommand>();
        cmd = make_unique<FlushCommand>(ctx, buffer);
        commandBufferManager.eraseAll();
        return cmd;
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