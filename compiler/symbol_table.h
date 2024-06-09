#pragma once
#include <stdexcept>
#include <unordered_map>
#include <string>

enum SymbolKind {
    STATIC = 0, FIELD = 1, ARG = 2, VAR = 3, NONE = -1
};

struct Symbol {
    std::string type;
    SymbolKind kind;
    int index;
};

class SymbolTable {
    std::unordered_map<std::string, Symbol> table;
    int index[4];

    public:
    SymbolTable();

    void reset();

    void define(std::string name, std::string type, SymbolKind kind);

    int varCnt(SymbolKind kind);

    Symbol getSymbol(std::string name);
};

