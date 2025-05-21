#pragma once

#include <fstream>
#include <string>

using namespace std;

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