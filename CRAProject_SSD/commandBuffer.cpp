#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

#include "ssdContext.cpp"

using namespace std;
using namespace std::filesystem;

class CommandBufferManager {
public:
    static CommandBufferManager& getInstance() {
        static CommandBufferManager instance;
        return instance;
    }

    CommandBufferManager(const CommandBufferManager&) = delete;
    CommandBufferManager& operator=(const CommandBufferManager&) = delete;
    CommandBufferManager(CommandBufferManager&&) = delete;
    CommandBufferManager& operator=(CommandBufferManager&&) = delete;

    void addCommandBuffer(vector<vector<string>>& buffer, const vector<string>& args) {
        buffer.push_back(args);
    }

    string getCommand(vector<string>& args)
    {
        int addr = stoi(args[1]);
        for (int i = (int)buffer.size() - 1; i >= 0; i--) {
            string command = buffer[i][0];
            if (command != "W" && command != "E") continue;

            int commandAddr = stoi(buffer[i][1]);
            int commandAddrSize = command == "E" ? stoi(buffer[i][2]) : 1;
            if (addr < commandAddr || addr >= commandAddr + commandAddrSize) continue;

            string value = command == "E" ? "0x00000000" : buffer[i][2];

            
            return value;
        }

        return "";
    }

    void writeCommandBuffer(const vector<vector<string>>& bufferInput) {
        vector<vector<string>> buffer = bufferInput;
        while (buffer.size() < 5) buffer.push_back({ "empty" });

        path p(dirPath);
        for (const auto& entry : directory_iterator(p)) {
            if (entry.is_regular_file()) {
                error_code ec;
                remove(entry.path(), ec);
                if (ec) {
                    return ctx.handleError();
                }
            }
        }

        for (int i = 0; i < buffer.size(); ++i) {
            string fileName = to_string(i + 1) + "_" + joinStrings(buffer[i]);
            path filePath = p / fileName;
            ofstream ofs(filePath);
            if (!ofs) {
                return ctx.handleError();
            }
        }
        buffer.clear();
    }

    void eraseAll(void)
    {
        path p(dirPath);

        // Delete All Files in Directory
        for (const auto& entry : directory_iterator(p)) {
            if (entry.is_regular_file()) {
                std::error_code ec;
                remove(entry.path(), ec);
                if (ec) return ctx.handleError();
            }
        }

        for (int i = 1; i <= 5; ++i) {
            string fileName = to_string(i) + "_empty";
            path filePath = p / fileName;
            ofstream ofs(filePath);
            if (!ofs) {
                throw runtime_error("Failed to create file: " + filePath.string());
            }
        }        
        buffer.clear();
    }

    bool pushCommandBuffer(vector<string> args)
    {
        if (args[0] == "E" && args[2] == "0") {
            if (buffer.size() == 5) {
                return true;
            }
            else return false;
        }
        if (buffer.size() == 5) {
            writeCommandBuffer({ args });
            return true;
        }
        else if (buffer.size() == 0) {
            buffer.push_back({ args });
            writeCommandBuffer(buffer);
            return false;
        }
        else
        {
            mergeAlgorithm(args);
            writeCommandBuffer(buffer);
            return false;
        }
    }

    void mergeAlgorithm(vector<string> args)
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

                    if ((targetStart <= newStart) && (targetEnd <= newEnd) && (newStart <= targetEnd)) {
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
                    else if ((targetStart >= newStart) && (targetEnd >= newEnd) && (targetStart <= newEnd)) {
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

    void setBuffer(const vector<vector<string>>& inputBuffer)
    {
        buffer = inputBuffer;
        return;
    }

    vector<vector<string>>& getBuffer()
    {
        return buffer;
    }

private:
    CommandBufferManager() {
        vector<string> fileNames = createAndReadFiles();
        buffer = parseFileNames(fileNames);
    }

    ~CommandBufferManager() = default;
    string dirPath = "./buffer";
    SSDContext ctx;
    vector<vector<string>> buffer;

    vector<vector<string>> parseFileNames(const vector<string>& fileNames) {
        vector<vector<string>> result;
        for (const string& fileName : fileNames) {
            vector<string> splitName = split(fileName);
            if (splitName.size() < 2) continue;
            splitName.erase(splitName.begin());

            if (splitName[0] == "empty") continue;

            result.push_back(splitName);
        }
        return result;
    }

    vector<string> createAndReadFiles() {
        vector<string> names;
        try {
            path p(dirPath);
            if (!exists(p)) {
                if (!create_directory(p)) {
                    throw runtime_error("Failed to create directory: " + dirPath);
                }
            }

            vector<string> checkFiles;
            for (const auto& entry : directory_iterator(p)) {
                if (entry.is_directory()) {
                    checkFiles.push_back(entry.path().filename().string());
                }
            }

            if (checkFiles.empty()) {
                for (int i = 1; i <= 5; ++i) {
                    string fileName = to_string(i) + "_empty";
                    path filePath = p / fileName;
                    ofstream ofs(filePath);
                    if (!ofs) {
                        throw runtime_error("Failed to create file: " + filePath.string());
                    }
                }
            }

            for (const auto& entry : directory_iterator(p)) {
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

    string joinStrings(const vector<string>& vec, const string& delimiter = "_") {
        string result;
        for (size_t i = 0; i < vec.size(); ++i) {
            result += vec[i];
            if (i != vec.size() - 1) {
                result += delimiter;
            }
        }
        return result;
    }
};
