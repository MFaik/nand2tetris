#pragma once
#include <string>
#include <vector>
#include <unordered_map>

enum class Keyword {
    CLASS, 
    CONSTRUCTOR, FUNCTION, METHOD, 
    FIELD, STATIC, VAR,
    INT, CHAR, BOOLEAN, VOID,
    TRUE, FALSE, NULL_, THIS, 
    LET, DO, IF, ELSE, WHILE, RETURN,
    notfound,
};

static std::unordered_map<std::string, Keyword> keywordTable = {
    {"class", Keyword::CLASS},
    {"constructor", Keyword::CONSTRUCTOR},
    {"function", Keyword::FUNCTION},
    {"method", Keyword::METHOD},
    {"field", Keyword::FIELD},
    {"static", Keyword::STATIC},
    {"var", Keyword::VAR},
    {"int", Keyword::INT},
    {"char", Keyword::CHAR},
    {"boolean", Keyword::BOOLEAN},
    {"void", Keyword::VOID},
    {"true", Keyword::TRUE},
    {"false", Keyword::FALSE},
    {"null", Keyword::NULL_},
    {"this", Keyword::THIS},
    {"let", Keyword::LET},
    {"do", Keyword::DO},
    {"if", Keyword::IF},
    {"else", Keyword::ELSE},
    {"while", Keyword::WHILE},
    {"return", Keyword::RETURN},
};

static std::unordered_map<Keyword, std::string> keywordTableReversed = {
    {Keyword::CLASS, "class"},
    {Keyword::CONSTRUCTOR, "constructor"},
    {Keyword::FUNCTION, "function"},
    {Keyword::METHOD, "method"},
    {Keyword::FIELD, "field"},
    {Keyword::STATIC, "static"},
    {Keyword::VAR, "var"},
    {Keyword::INT, "int"},
    {Keyword::CHAR, "char"},
    {Keyword::BOOLEAN, "boolean"},
    {Keyword::VOID, "void"},
    {Keyword::TRUE, "true"},
    {Keyword::FALSE, "false"},
    {Keyword::NULL_, "null"},
    {Keyword::THIS, "this"},
    {Keyword::LET, "let"},
    {Keyword::DO, "do"},
    {Keyword::IF, "if"},
    {Keyword::ELSE, "else"},
    {Keyword::WHILE, "while"},
    {Keyword::RETURN, "return"},
};

enum class TokenType {
    KEYWORD, SYMBOL, INTEGER, STRING, IDENTIFIER
};

struct Token {
    TokenType type;  
    std::string str;
    int i;
    Keyword keyword;
    char symbol;
};

std::string token_to_str(Token t);

class Tokenizer {
    std::string str;
    size_t ptr = 0;

    std::vector<Token> tokens;


    void skipSpace();
    Keyword eatKeyword();
    char tryEatSymbol();
    int eatInteger();
    std::string eatString();
    std::string eatIdentifier();
    bool eatToken();

    public:
    Tokenizer(std::string s);
    std::vector<Token> tokenize();
};
