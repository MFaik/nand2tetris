#include <stdexcept>
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <bitset>
#include <filesystem>

typedef std::vector<std::string> vs;

vs readFileToVector(const std::string& filename)
{
    std::ifstream source;
    source.open(filename);
    vs lines;
    std::string line;
    while (std::getline(source, line))
    {
        lines.push_back(line);
    }
    return lines;
}

vs removeWhiteSpace(vs v) {
    vs ret;
    for(int l = 0; l < v.size();l++) {
        std::string str;
        for(int i = 0; i < v[l].size(); i++) {
            if(i+1 < v[l].size() && v[l][i] == '/' && v[l][i+1] == '/') {
                break;
            }
            if(!isspace(v[l][i])) {
                str.push_back(v[l][i]);
            }
        }
        if(str.size() > 0)
            ret.push_back(str);
    }
    return ret;
}

vs replaceSymbols(vs v)
{
    std::unordered_map<std::string, int> symbols = {
        {"SP", 0},
        {"LCL",1},
        {"ARG",2},
        {"THIS",3},
        {"THAT",4},
        {"R0", 0}, {"R1", 1}, {"R2", 2}, {"R3", 3}, {"R4", 4}, {"R5", 5}, {"R6", 6}, {"R7", 7}, {"R8", 8}, {"R9", 9}, {"R10", 10}, {"R11", 11}, {"R12", 12}, {"R13", 13}, {"R14", 14}, {"R15", 15}, 
        {"SCREEN",16384},
        {"KBD", 24576}
    };
    vs ret;
    int cnt = 0;
    for(int i = 0; i < v.size(); i++,cnt++) {
        if(v[i][0] == '(') {
            if(isdigit(v[i][1])) {
                throw std::runtime_error("constant can't start with number: " + v[i]);
            }
            if(v[i][v[i].size()-1] != ')') {
                throw std::runtime_error("unmatched parathesis: " + v[i]);
            }
            std::string symbol = v[i].substr(1, v[i].size()-2);
            if(symbols.find(symbol) != symbols.end()) {
                throw std::runtime_error("constant is already declared: " + symbol);
            }
            symbols[symbol] = cnt;
            cnt--;
        }
    }
    int symbolCnt = 16;
    for(int i = 0; i < v.size(); i++) {
        if(v[i][0] == '@') {
            if(!isdigit(v[i][1])) {
                //symbol
                auto s = symbols.find(v[i].substr(1));
                if(s == symbols.end()) {
                    symbols[v[i].substr(1)] = symbolCnt;
                    symbolCnt++;
                }
                ret.push_back("@" + std::to_string(symbols[v[i].substr(1)]));
                continue;
            }
        }
        if(v[i][0] == '(')
            continue;
        ret.push_back(v[i]);
    }

    return ret;
}

constexpr int str2int(const char* s, int h = 0) {
    return !s[h] ? 0 : (str2int(s, h+1)<<8) + s[h];
}

std::string resolveComp(std::string comp) {
    switch(str2int(comp.c_str())) {
        case str2int("0"):
            return "101010";
        case str2int("1"):
            return "111111";
        case str2int("-1"):
            return "111010";
        case str2int("D"):
            return "001100";
        case str2int("A"):
            return "110000";
        case str2int("!D"):
            return "001101";
        case str2int("!A"):
            return "110001";
        case str2int("-D"):
            return "001111";
        case str2int("-A"):
            return "110011";
        case str2int("D+1"):
        case str2int("1+D"):
            return "011111";
        case str2int("A+1"):
        case str2int("1+A"):
            return "110111";
        case str2int("D-1"):
            return "001110";
        case str2int("A-1"):
            return "110010";
        case str2int("D+A"):
        case str2int("A+D"):
            return "000010";
        case str2int("D-A"):
            return "010011";
        case str2int("A-D"):
            return "000111";
        case str2int("D&A"):
        case str2int("A&D"):
            return "000000";
        case str2int("D|A"):
        case str2int("A|D"):
            return "010101";
    }
    throw std::runtime_error("unkown comp: " + comp);
}

std::string resolveDest(std::string dest) {
    std::string ret = "000";
    if(dest.find('A') != std::string::npos) {
        ret[0] = '1';
    } 
    if(dest.find('D') != std::string::npos) {
        ret[1] = '1';
    } 
    if(dest.find('M') != std::string::npos) {
        ret[2] = '1';
    } 
    return ret;
}

std::string resolveJump(std::string jump) {
    switch(str2int(jump.c_str())) {
        case str2int(""):
            return "000";
        case str2int("JGT"):
            return "001";
        case str2int("JEQ"):
            return "010";
        case str2int("JGE"):
            return "011";
        case str2int("JLT"):
            return "100";
        case str2int("JNE"):
            return "101";
        case str2int("JLE"):
            return "110";
        case str2int("JMP"):
            return "111";
    }
    throw std::runtime_error("unkown jump: " + jump);
}

vs resolveInstructions(vs v) {
    vs ret;
    for(int i = 0; i < v.size(); i++) {
        if(v[i][0] == '@') {
            ret.push_back(std::bitset<16>(std::stoi(v[i].substr(1))).to_string());
        } else {
            std::string ins = "111";

            std::string comp = v[i];

            std::string dest;
            auto eq = comp.find('=');
            if(eq != std::string::npos) {
                dest = comp.substr(0, eq);
                comp = v[i].substr(eq+1);
            } else {
                eq = 0;
            }

            std::string jump;
            auto sc = comp.find(';');
            if(sc != std::string::npos) {
                jump = comp.substr(sc+1);
                comp = v[i].substr(0, sc);
            } else {
                sc = comp.size()-eq;
            }

            auto mPos = comp.find('M');
            if(mPos != std::string::npos) {
                ins.push_back('1');
                comp[mPos] = 'A';
            } else {
                ins.push_back('0');
            }
            try {
            ins += resolveComp(comp);
            ins += resolveDest(dest);
            ins += resolveJump(jump);
            } catch(std::runtime_error e) {
                throw std::runtime_error(std::string(e.what()) + " at line: " + v[i]); 
            }

            ret.push_back(ins);
        }
    }
    return ret;
}

namespace fs = std::filesystem;

int main(int argc,  char **argv)
{
    if(argc != 2) {
        std::cout << "You need to provide one file\n";
        return 1;
    }
    std::string path(argv[1]);
    vs code = readFileToVector(path);
    
    std::ofstream out;
    out.open(fs::path(path).stem().string() + ".hack");

    try {
        code = removeWhiteSpace(code);
        code = replaceSymbols(code);
        code = resolveInstructions(code);
    } catch(std::runtime_error e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    for(int i = 0; i < code.size(); i++) {
        out << code[i] << '\n';  
    }
}
