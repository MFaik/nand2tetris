#include <unordered_map>
#include <string>
#include "symbol_table.h"

SymbolTable::SymbolTable() : index{0}, table() { }

void SymbolTable::reset() {
    for(int i = 0;i < 4;i++) {
        index[i] = 0;
    }
    table.clear();
}

void SymbolTable::define(std::string name, std::string type, SymbolKind kind) {
    table[name] = Symbol { type, kind, index[kind] };
    index[kind]++;
}

int SymbolTable::varCnt(SymbolKind kind) {
    return index[kind];
}

Symbol SymbolTable::getSymbol(std::string name) {
    if(table.find(name) != table.end()) {
        return table[name];
    }
    return Symbol{"", SymbolKind::NONE, -1};
}
