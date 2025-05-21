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
    CommandBufferManager(SSDContext& context, const string& directory)
        : ctx(context), dirPath(directory) {
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

    void addCommandBuffer(vector<vector<string>>& buffer, const vector<string>& args) {
        buffer.push_back(args);
    }

    string getCommand(vector<string>& args, vector<vector<string>> buffer)
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
    }

private:
    string dirPath;
    SSDContext& ctx;

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
