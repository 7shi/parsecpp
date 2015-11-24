#include <iostream>
#include <string>
#include <cctype>

class Parser {
private:
    const char *p;

public:
    Parser(const char *p) : p(p) {}

    char peek() {
        if (!*p) throw std::string("too short");
        return *p;
    }

    void next() {
        if (!*p) throw std::string("at last");
        ++p;
    }
};

template<typename T> void parseTest(T (*p)(Parser *), const char *s) {
    Parser parser = s;
    try {
        std::cout << p(&parser) << std::endl;
    } catch (std::string e) {
        std::cout << e << std::endl;
    }
}

template<bool (*f)(char)> char satisfy(Parser *p, const std::string &err) {
    char x = p->peek();
    if (!f(x)) throw "not " + err + ": '" + x + "'";
    p->next();
    return x;
}
template<bool (*f)(char)> char satisfy(Parser *p) {
    return satisfy<f>(p, "???");
}

bool any(char) { return true; }
char anyChar(Parser *p) { return satisfy<any>(p, "anyChar"); }

template<char c> bool isChar(char ch) { return ch == c; }
template<char c> char char1(Parser *p) {
    return satisfy< isChar<c> >(p, std::string("char '") + c + "'");
}

bool isDigit   (char ch) { return std::isdigit(ch); }
bool isUpper   (char ch) { return std::isupper(ch); }
bool isLower   (char ch) { return std::islower(ch); }
bool isAlpha   (char ch) { return std::isalpha(ch); }
bool isAlphaNum(char ch) { return isalpha(ch) || isdigit(ch); }
bool isLetter  (char ch) { return isalpha(ch) || ch == '_';   }

char digit   (Parser *p) { return satisfy<isDigit   >(p, "digit"   ); }
char upper   (Parser *p) { return satisfy<isUpper   >(p, "upper"   ); }
char lower   (Parser *p) { return satisfy<isLower   >(p, "lower"   ); }
char alpha   (Parser *p) { return satisfy<isAlpha   >(p, "alpha"   ); }
char alphaNum(Parser *p) { return satisfy<isAlphaNum>(p, "alphaNum"); }
char letter  (Parser *p) { return satisfy<isLetter  >(p, "letter"  ); }

std::string test1(Parser *p) {
    char x1 = anyChar(p);
    char x2 = anyChar(p);
    char ret[] = {x1, x2, 0};
    return ret;
}

std::string test2(Parser *p) {
    std::string x1 = test1(p);
    char x2 = anyChar(p);
    return x1 + x2;
}

std::string test3(Parser *p) {
    char x1 = letter(p);
    char x2 = digit(p);
    char x3 = digit(p);
    char ret[] = {x1, x2, x3, 0};
    return ret;
}

int main() {
    parseTest(anyChar   , "abc" );
    parseTest(test1     , "abc" );
    parseTest(test2     , "abc" );
    parseTest(test2     , "12"  );  // NG
    parseTest(test2     , "123" );
    parseTest(char1<'a'>, "abc" );
    parseTest(char1<'a'>, "123" );  // NG
    parseTest(digit     , "abc" );  // NG
    parseTest(digit     , "123" );
    parseTest(letter    , "abc" );
    parseTest(letter    , "123" );  // NG
    parseTest(test3     , "abc" );  // NG
    parseTest(test3     , "123" );  // NG
    parseTest(test3     , "a23" );
    parseTest(test3     , "a234");
}
