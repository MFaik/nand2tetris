#include "vmwriter.h"
#include <stdexcept>
#include <unordered_map>

std::string segment_to_str(Segment seg) {
    switch(seg) {
        case Segment::CONSTANT:
            return "constant";
        case Segment::ARGUMENT:
            return "argument";
        case Segment::LOCAL:
            return "local";
        case Segment::STATIC:
            return "static";
        case Segment::THIS:
            return "this";
        case Segment::THAT:
            return "that";
        case Segment::POINTER:
            return "pointer";
        case Segment::TEMP:
            return "temp";
        default:
            throw std::runtime_error("internal error, segment_to_str");
    }
}

std::unordered_map<std::string, Segment> segmentTable = {
    {"constant", Segment::CONSTANT},
    {"argument", Segment::ARGUMENT},
    {"local", Segment::LOCAL},
    {"static", Segment::STATIC},
    {"this", Segment::THIS},
    {"that", Segment::THAT},
    {"pointer", Segment::POINTER},
    {"temp", Segment::TEMP},
};

Segment str_to_segment(std::string str) {
    if(segmentTable.find(str) == segmentTable.end()) {
        throw std::runtime_error("segment not found: " + str);
    }
    return segmentTable[str];
}

std::string command_to_str(Command com) {
    switch(com) {
        case Command::ADD:
            return "add";
        case Command::SUB:
            return "sub";
        case Command::NEG:
            return "neg";
        case Command::EQ:
            return "eq";
        case Command::GT:
            return "gt";
        case Command::LT:
            return "lt";
        case Command::AND:
            return "and";
        case Command::OR:
            return "or";
        case Command::NOT:
            return "not";
        default:
            throw std::runtime_error("internal error, command_to_str");
    }
}

std::unordered_map<std::string, Command> commandTable = {
    {"add", Command::ADD},
    {"sub", Command::SUB},
    {"neg", Command::NEG},
    {"eq", Command::EQ},
    {"gt", Command::GT},
    {"lt", Command::LT},
    {"and", Command::AND},
    {"or", Command::OR},
    {"not", Command::NOT},
};

Command str_to_command(std::string str) {
    if(commandTable.find(str) == commandTable.end()) {
        throw std::runtime_error("unkown command: " + str);
    }
    return commandTable[str];
}

VMWriter::VMWriter() : out() {}

void VMWriter::writePush(Segment seg, int i) {
    out << "push " << 
        segment_to_str(seg) << ' ' << 
        i << std::endl;
}

void VMWriter::writePop(Segment seg, int i) {
    out << "pop " <<
        segment_to_str(seg) << ' ' <<
        i << std::endl;
}

void VMWriter::writeArithmetic(Command c) {
    out << command_to_str(c) << std::endl;
}

void VMWriter::writeLabel(std::string str) {
    out << "label " << str << std::endl;
}

void VMWriter::writeGoto(std::string str) {
    out << "goto " << str << std::endl;
}

void VMWriter::writeIf(std::string str) {
    out << "if-goto " << str << std::endl;
}

void VMWriter::writeCall(std::string name, int nArgs) {
    out << "call " << name << ' ' << nArgs << std::endl;
}

void VMWriter::writeFunction(std::string name, int nArgs) {
    out << "function " << name << ' ' << nArgs << std::endl;
}

void VMWriter::writeReturn() {
    out << "return" << std::endl;
}

std::string VMWriter::get() {
    return out.str();
}
