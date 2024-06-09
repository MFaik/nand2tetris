#pragma once
#include <sstream>
#include <string>

enum class Segment {
    CONSTANT, ARGUMENT, LOCAL, STATIC, THIS, THAT, POINTER, TEMP
};

Segment str_to_segment(std::string);

enum class Command {
    ADD, SUB, NEG, EQ, GT, LT, AND, OR, NOT
};

Command str_to_command(std::string);

class VMWriter {
    std::stringstream out;   

    public:
        VMWriter();

        void write(std::string str);
        
        void writePush(Segment segment, int index);
        void writePop(Segment segment, int index);
        void writeArithmetic(Command command);
        void writeLabel(std::string label);
        void writeGoto(std::string label);
        void writeIf(std::string label);
        void writeCall(std::string name, int nArgs);
        void writeFunction(std::string name, int nArgs);
        void writeReturn();

        std::string get();
};
