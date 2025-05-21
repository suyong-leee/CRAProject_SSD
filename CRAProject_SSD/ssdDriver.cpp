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
    CommandBufferManager& commandBufferManager = CommandBufferManager::getInstance();

    void run(int argc, char* argv[]) {

        if (argc <= 1) return ctx.handleError();

        vector<string> args = parseArguments(argc, argv);

        if (isValid(args) == false) {
            return ctx.handleError();
        }

        unique_ptr<Command> cmd = make_unique<NoopCommand>();
        string command = args[0];
        if (command == "W" || command == "E") {
            cmd = preprocessWE(args);
        }
        else if (command == "R") {
            cmd = preprocessR(args);
        }
        else if (command == "F") {
            cmd = preprocessF(args);
        }
        else {
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

    unique_ptr<Command> preprocessWE(vector<string> args) {
        unique_ptr<Command> cmd = make_unique<NoopCommand>();
        bool needFlush = commandBufferManager.pushCommandBuffer(args);
        if (needFlush == true) {
            cmd = make_unique<FlushCommand>(ctx, commandBufferManager.getBuffer());
        }
        else {
            cmd = make_unique<NoopCommand>();
        }

        return cmd;
    }

    unique_ptr<Command> preprocessR(vector<string> args) {
        unique_ptr<Command> cmd = make_unique<NoopCommand>();
        string value = commandBufferManager.getCommand(args);
        if (value == "") {
            cmd = make_unique<ReadCommand>(ctx, stoi(args[1]));
        }
        else {
            cmd = make_unique<FastReadCommand>(ctx, value);
        }
        return cmd;
    }

    unique_ptr<Command> preprocessF(vector<string> args) {
        unique_ptr<Command> cmd = make_unique<NoopCommand>();
        cmd = make_unique<FlushCommand>(ctx, commandBufferManager.getBuffer());
        commandBufferManager.eraseAll();
        return cmd;
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