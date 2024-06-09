#include "token.h"
#include "compilationEngine.h"
#include <fstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

std::string readFileToStr(fs::path path) {
    std::ifstream in(path);
    std::ostringstream sstr;
    sstr << in.rdbuf();
    return sstr.str();
}

std::string cleanLines(std::string str) {
    std::string ret = "";
    int commentDepth = 0;
    bool lineComment = false;
    for(int i = 0;i < str.size();i++) {
        if(str[i] == '/') {
            if(i == str.size()-1)
                throw std::runtime_error("file can't end with /");
            if(str[i+1] == '/' && commentDepth == 0) {
                lineComment = true;
                i++;
                continue;
            } else if(str[i+1] == '*') {
                commentDepth++;
                i++;
                continue;
            }
        }
        if(lineComment == true) {
            if(str[i] != '\n')
                continue;
            else
                lineComment = false;
        }
        if(commentDepth > 0) {
            if(str[i] == '*') {
                if(i < str.size()-1 && str[i+1] == '/') {
                    commentDepth--;
                    i++;
                }
            }
            continue;
        }
        if((ret.size() == 0 || isspace(ret[ret.size()-1])) && isspace(str[i])) {
            continue;
        }
        ret.push_back(str[i]);
    }
    if(commentDepth > 0) {
        throw std::runtime_error("mutli-line comment not closed");
    }

    return ret;
}

void compileFile(std::string path) {
    std::ofstream out;
    out.open(fs::path(path).stem().string() + ".vm");

    std::string file = readFileToStr(path);

    file = cleanLines(file);

    Tokenizer tokenizer(file);
    auto tokens = tokenizer.tokenize();

    CompilationEngine eng(tokens);
    out << eng.compile();
    out.close();
}

int main(int argc, char **argv) {
    std::string path;
    if(argc == 1) {
        path = '.';  
    } else if(argc != 2) {
        std::cout << "You need to provide one file\n";
        return 2;
    } else {
        path = argv[1];
    }
    if(fs::is_directory(path)) {
        char sep = fs::path::preferred_separator;
        if(path[path.size()-1] != sep)
            path.push_back(sep);
        std::ofstream out;
        std::string directoryName = fs::path(path).stem();
        if(directoryName.size() == 0)
            directoryName = fs::path(path).parent_path().stem();
        for(const auto & entry : fs::directory_iterator(path)) {
            try {
                if(entry.path().extension() == ".jack")
                    compileFile(entry.path());
            } catch(std::runtime_error e) {
                std::cout << e.what() << "\nat file: " << entry.path() << std::endl;
                return 1;
            }
        }
    } else {
        compileFile(path);
    }
}

