#include <algorithm>
#include <cstring>
#include <ostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

typedef std::vector<std::string> vs;

std::string codeFilename;

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

vs cleanLines(vs v) {
    vs ret;
    for(int l = 0;l < v.size();l++) {
        std::string str;
        for(int i = 0; i < v[l].size(); i++) {
            if(isspace(v[l][i])) {
                if(i == 0 || i == v[l].size()-1) {
                    continue;
                }
                if(isspace(v[l][i-1])) {
                    continue;
                }
                str.push_back(' ');
            } else if(v[l][i] == '/') {
                if(i == v.size()-1 || v[l][i+1] != '/') {
                    throw std::runtime_error("unexpected / at the end of line: " + v[l]);
                } else {
                    break;
                }
            } else {
                str.push_back(v[l][i]);
            }
        }
        if(str.size() > 0)
            ret.push_back(str);
    }
    return ret;
}

vs split (const std::string &s, char delim) {
    vs result;
    std::stringstream ss(s);
    std::string item;

    while(getline(ss, item, delim)) {
        result.push_back(item);
    }

    return result;
}

void push(std::string what, int indirection, std::string offset, vs &v) {
    if(indirection == 0) {
        //0)A = constant
        v.push_back("@" + what);
        v.push_back("D=A");
    } else {
        if(offset == "0") {
            v.push_back("@" + what);
            if(indirection == 2)
                v.push_back("A=M");
            v.push_back("D=M");
        } else if(offset == "1") {
            v.push_back("@" + what);
            if(indirection == 1) {
                v.push_back("A=A+1");
            } else if(indirection == 2) {
                v.push_back("A=M+1");
            }
            v.push_back("D=M");
        } else {
            //1)D = what 2)D = *what
            v.push_back("@" + offset);
            v.push_back("D=A");
            v.push_back("@" + what);
            v.push_back("A=D+M");
            if(indirection == 1)
                v.push_back("D=A");
            else if(indirection == 2)
                v.push_back("D=M");
        }
    }
    //*(SP++) = D
    v.push_back("@SP");
    v.push_back("M=M+1");
    v.push_back("A=M-1");
    v.push_back("M=D");
}

void popOffset1(std::string what, int indirection, vs &v) {
    v.push_back("@SP");
    v.push_back("AM=M-1");
    v.push_back("D=M");

    v.push_back("@" + what);
    if(indirection == 2) {
        v.push_back("A=M+1");
    } else {//indirection == 1
        v.push_back("A=A+1");
    }
    v.push_back("M=D");
}

void pop(std::string what, int indirection, std::string offset, vs &v) {
    if(indirection == 0) {
        throw("can't pop to indirection 0 registers: " + what);
    }

    if(offset == "0") {
        v.push_back("@SP");
        v.push_back("AM=M-1");
        v.push_back("D=M");
        v.push_back("@" + what);
        if(indirection == 2)
            v.push_back("A=M");
        v.push_back("M=D");

    } else if(offset == "1") {
        v.push_back("@SP");
        v.push_back("AM=M-1");
        v.push_back("D=M");

        v.push_back("@" + what);
        if(indirection == 2) {
            v.push_back("A=M+1");
        } else {//indirection == 1
            v.push_back("A=A+1");
        }
        v.push_back("M=D");
    } else {
        v.push_back("@" + offset);
        v.push_back("D=A");
        v.push_back("@" + what);
        if(indirection == 1)
            v.push_back("D=D+A");
        else 
            v.push_back("D=D+M");
        v.push_back("@R15");
        v.push_back("M=D");
        //D = POP()
        v.push_back("@SP");
        v.push_back("AM=M-1");
        v.push_back("D=M");
        //*R15 = D
        v.push_back("@R15");
        v.push_back("A=M");
        v.push_back("M=D");
    }
}

int replaceRegisterNames(std::string &name, std::string &offset) {
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    if(name == "LOCAL") {
        name = "LCL";
        return 2;
    }
    if(name == "ARGUMENT") {
        name = "ARG";
        return 2;
    }
    if(name == "THIS" || name == "THAT")
        return 2;

    if(name == "POINTER") {
        if(offset == "0") {
            name = "THIS";
            offset = "0";
        } else if(offset == "1") {
            name = "THAT";
            offset = "0";
        } else {
            throw std::runtime_error("pointer " + offset + " does not exist");
        }
        return 1;
    }
    if(name == "TEMP") {
        name = std::to_string(stoi(offset) + 5);
        offset = "0";
        return 1;
    } 
    if(name == "CONSTANT") {
        name = offset;
        offset = "0";
        return 0;
    }
    if(name == "STATIC") {
        name = codeFilename+"."+offset;
        offset = "0";
        return 1;
    }
    throw std::runtime_error("unkown register: " + name);
}

void singleInstruction(std::string ins, vs &v) {
    static int insCnt = 0;
    insCnt++;
    if(ins == "add" || ins == "sub" || ins == "eq" ||
            ins == "gt" || ins == "lt" || ins == "and" ||
            ins == "or") {
        v.push_back("@SP");
        v.push_back("AM=M-1");
        v.push_back("D=M");
        v.push_back("@SP");
        v.push_back("A=M-1");
        if(ins == "add") {
            v.push_back("M=D+M");
        } else if(ins == "and") {
            v.push_back("M=D&M");
        } else if(ins == "or") {
            v.push_back("M=D|M");
        } else if(ins == "eq" || ins == "lt" || ins == "gt") {
            v.push_back("D=D-M");
            v.push_back("M=0");
            v.push_back("@SKIP."+std::to_string(insCnt));
            if(ins == "eq") {
                v.push_back("D;JNE");
            } else if(ins == "lt") {
                v.push_back("D;JLE");
            } else if(ins == "gt") {
                v.push_back("D;JGE");
            }
            v.push_back("@SP");
            v.push_back("A=M-1");
            v.push_back("M=M-1");
            v.push_back("(SKIP." + std::to_string(insCnt) + ")");
        } else { //ins == "sub"
            v.push_back("M=M-D");
        }
    } else if(ins == "not" || ins == "neg") {
        v.push_back("@SP");
        v.push_back("A=M-1");
        if(ins == "not") {
            v.push_back("M=!M");
        } else { //ins == "neg"
            v.push_back("M=-M");
        }
    } else if(ins == "return") {
        //R13 = LCL
        v.push_back("@LCL");
        v.push_back("D=M");
        v.push_back("@R13");
        v.push_back("M=D");
        //R14 = returnAddress
        v.push_back("@5");
        v.push_back("A=D-A");
        v.push_back("D=M");
        v.push_back("@R14");
        v.push_back("M=D");
        // //ARG = pop()
        pop("ARG", 2, "0", v);
        // // SP = ARG+1
        v.push_back("@ARG");
        v.push_back("D=M");
        v.push_back("@SP");
        v.push_back("M=D+1");
        //R13 -= 1
        //THAT = *R13
        v.push_back("@R13");
        v.push_back("AM=M-1");
        v.push_back("D=M");
        v.push_back("@THAT");
        v.push_back("M=D");
        //R13 -= 1
        //THIS = *R13
        v.push_back("@R13");
        v.push_back("AM=M-1");
        v.push_back("D=M");
        v.push_back("@THIS");
        v.push_back("M=D");
        //R13 -= 1
        //ARG = *R13
        v.push_back("@R13");
        v.push_back("AM=M-1");
        v.push_back("D=M");
        v.push_back("@ARG");
        v.push_back("M=D");
        //R13 -= 1
        //LCL = *R13
        v.push_back("@R13");
        v.push_back("AM=M-1");
        v.push_back("D=M");
        v.push_back("@LCL");
        v.push_back("M=D");
        //jump to *R14
        v.push_back("@R14");
        v.push_back("A=M");
        v.push_back("M;JMP");
    } else {
        throw std::runtime_error("unknown command");
    }
}

vs resolveInstruction(vs v) {
    vs ret;
    std::string currentFunction = "";
    for(int l = 0; l < v.size(); l++) {
        ret.push_back("//" + v[l]);
        try {
            vs w = split(v[l], ' ');
            if(w.size() == 1) {
                singleInstruction(w[0], ret);
            } else if(w.size() == 2) {
                if(isdigit(w[1][0])) {
                    throw std::runtime_error("label cannot start with digit");
                }
                std::string label = currentFunction+"."+w[1];
                if(w[0] == "label") {
                    ret.push_back("("+label+")");
                } else if(w[0] == "goto") {
                    ret.push_back("@" + label);    
                    ret.push_back("0;JMP");
                } else if(w[0] == "if-goto") {
                    ret.push_back("@SP");
                    ret.push_back("AM=M-1");
                    ret.push_back("D=M");
                    ret.push_back("@" + label);
                    ret.push_back("D;JNE");
                }
            } else if(w.size() == 3) {
                if(w[0] == "push" || w[0] == "pop") {
                    int indirection = replaceRegisterNames(w[1], w[2]);
                    if(w[0] == "push") {
                        push(w[1], indirection, w[2], ret);
                    } else { //w[0] == "pop"
                        pop(w[1], indirection, w[2], ret);
                    }
                } else if(w[0] == "function") {
                    currentFunction = w[1];
                    ret.push_back("("+w[1]+")");
                    int pushCnt = stoi(w[2]);
                    if(pushCnt) {
                        ret.push_back("@SP");
                        ret.push_back("A=M");
                        for(int i = 0; i < pushCnt; i++) {
                            ret.push_back("M=0");
                            if(i != pushCnt -1)
                                ret.push_back("A=A+1");
                        }
                        ret.push_back("@" + w[2]);
                        ret.push_back("D=A");
                        ret.push_back("@SP");
                        ret.push_back("M=D+M");
                    }
                } else if(w[0] == "call") {
                    static int returnCnt = 0;
                    returnCnt++;

                    push(codeFilename + "_" + std::to_string(returnCnt), 0, "0", ret);
                    push("LCL", 1, "0", ret);
                    push("ARG", 1, "0", ret);
                    push("THIS", 1, "0", ret);
                    push("THAT", 1, "0", ret);
                    //D = SP
                    ret.push_back("@SP");
                    ret.push_back("D=M");
                    //LCL = SP
                    ret.push_back("@LCL");
                    ret.push_back("M=D");
                    //ARG = SP-5-w[2]
                    ret.push_back("@" + std::to_string(5+stoi(w[2])));
                    ret.push_back("D=D-A");
                    ret.push_back("@ARG");
                    ret.push_back("M=D");
                    //goto w[1]
                    ret.push_back("@"+w[1]);
                    ret.push_back("0;JMP");
                    //(return_address)
                    ret.push_back("(" + codeFilename + "_" + std::to_string(returnCnt) + ")");
                } else {
                    throw std::runtime_error("unknown command:" + w[0]);
                }
            } else {
                throw std::runtime_error("unknown command");
            }
        } catch(std::runtime_error e) {
            throw std::runtime_error(std::string(e.what()) + "\nat line: " + v[l]);
        }
    }
    return ret;
}

namespace fs = std::filesystem;

void translateFile(fs::path filepath, std::ofstream &out) {
    codeFilename = filepath.stem();
    if(filepath.extension() != ".vm") {
        return;
    }
    vs code = readFileToVector(filepath);

    out << "//" << codeFilename << '\n';

    code = cleanLines(code);
    code = resolveInstruction(code);

    for(int i = 0; i < code.size(); i++) {
        out << code[i] << '\n';
    }
}

int main(int argc,  char **argv)
{
    if(argc != 2) {
        std::cout << "You need to provide one file\n";
        return 2;
    }
    std::string path = argv[1];
    if(fs::is_directory(path)) {
        if(path[path.size()-1] != '/')
            path.push_back('/');
        std::ofstream out;
        std::string directoryName = fs::path(path).stem();
        if(directoryName.size() == 0)
            directoryName = fs::path(path).parent_path().stem();
        out.open(directoryName +".asm");
        out << "@256\n";
        out << "D=A\n";
        out << "@SP\n";
        out << "M=D\n";

        vs callSys = resolveInstruction({"call Sys.init 0"});
        for(int i = 0; i < callSys.size(); i++) {
            out << callSys[i] << '\n';
        }
        ;
        for(const auto & entry : fs::directory_iterator(path)) {
            try {
                if(entry.path().extension() == ".vm")
                    translateFile(entry.path(), out);           
            } catch(std::runtime_error e) {
                std::cout << e.what() << "\nat file: " << entry.path() << std::endl;
                return 1;
            }
        }
        out.close();
    } else {
        std::ofstream out;
        out.open(fs::path(path).stem().string() + ".asm");
        out << "@256\n";
        out << "D=A\n";
        out << "@SP\n";
        out << "M=D\n";

        vs callSys = resolveInstruction({"call Sys.init 0"});
        for(int i = 0; i < callSys.size(); i++) {
            out << callSys[i] << '\n';
        }   

        translateFile(path, out);

        out.close();
    }
}
