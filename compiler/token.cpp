#include "token.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>

std::string token_to_str(Token t) {
    switch(t.type) {
        case TokenType::KEYWORD:
            return keywordTableReversed[t.keyword] + "(keyword)";
        case TokenType::STRING:
            return t.str + "(string)";
        case TokenType::SYMBOL:
            return std::string(1, t.symbol) + "(symbol)";
        case TokenType::INTEGER:
            return std::to_string(t.i) + "(integer)";
        case TokenType::IDENTIFIER:
            return t.str + "(identifier)";
    }
    throw std::runtime_error("internal error, token type not recognized");
}

Tokenizer::Tokenizer(std::string s) : str(s) { }

std::vector<Token> Tokenizer::tokenize() {
    while(eatToken()){
        // std::cout << token_to_str(tokens[tokens.size()-1]) << std::endl;
    }
    return tokens;
}

void Tokenizer::skipSpace() {
    while(ptr < str.size() && isspace(str[ptr])) {
        ptr++;
    }
}

Keyword Tokenizer::eatKeyword() {
    skipSpace();
    if(ptr == str.size()) {
        return Keyword::notfound;
    }

    int cnt = 0;
    while(ptr+cnt < str.size() && (isalnum(str[ptr+cnt]) || str[ptr+cnt] == '_')) {
        cnt++;
    }
    std::string keyword = str.substr(ptr, cnt);

    if(keywordTable.find(keyword) != keywordTable.end()) {
        ptr += cnt;
        return keywordTable[keyword];
    }
    return Keyword::notfound;
}

char Tokenizer::tryEatSymbol() {
    skipSpace();
    if(ptr == str.size())
        return 0;

    const auto symbols = std::string("(){}[].,;+-*/&|<>=~");
    if(symbols.find(str[ptr]) != std::string::npos) {
        ptr++;
        return str[ptr-1];
    }
    return 0;
}

int Tokenizer::eatInteger() {
    skipSpace();
    if(ptr == str.size())
        return -1;

    int cnt = 0;
    while(ptr+cnt < str.size() && isdigit(str[ptr+cnt])) {
        cnt++;
    }

    if(cnt == 0)
        return -1;

    int ret = stoi(str.substr(ptr, cnt));
    ptr += cnt;
    return ret;
}

std::string Tokenizer::eatString() {
    skipSpace();
    if(ptr == str.size())
        return "";

    int cnt = 0;
    if(str[ptr] == '"') {
        cnt++;
    } else {
        return "";
    }

    while(ptr+cnt < str.size() && str[ptr+cnt] != '"') {
        cnt++;
    }
    if(ptr+cnt == str.size())
        throw std::runtime_error("unmatched quote");
    cnt++;

    std::string ret = str.substr(ptr, cnt);

    ptr += cnt;
    return ret;
}

std::string Tokenizer::eatIdentifier() {
    skipSpace();
    if(ptr == str.size())
        return "";

    if(isdigit(str[ptr])) {
        return "";
    }

    int cnt = 0;
    while(ptr+cnt < str.size() && (isalnum(str[ptr+cnt]) || str[ptr+cnt] == '_')) {
        cnt++;
    }

    std::string ret = str.substr(ptr, cnt);

    ptr += cnt;
    return ret;
}

bool Tokenizer::eatToken() {
    int i = eatInteger();
    if(i != -1) {
        tokens.push_back(Token{TokenType::INTEGER, "", i, Keyword::notfound, 0});
        return true;
    }

    Keyword k = eatKeyword();
    if(k != Keyword::notfound) {
        tokens.push_back(Token{TokenType::KEYWORD, "", -1, k, 0});
        return true;
    }

    std::string s = eatString();
    if(s.size() != 0) {
        tokens.push_back(Token{TokenType::STRING, s.substr(1, s.size()-2), -1, Keyword::notfound, 0});
        return true;
    }

    char c = tryEatSymbol();
    if(c != 0) {
        tokens.push_back(Token{TokenType::SYMBOL, "", -1, Keyword::notfound, c});
        return true;
    }

    s = eatIdentifier();
    if(s.size() != 0) {
        tokens.push_back(Token{TokenType::IDENTIFIER, s, -1, Keyword::notfound, 0});
        return true;
    }

    return false;
}


