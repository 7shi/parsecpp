#include <iostream>
#include <sstream>
#include <string>
#include <cctype>

class Source {
private:
    const char *p;
    int line, col;

public:
    Source(const char *p) : p(p), line(1), col(1) {}

    char peek() {
        if (!*p) throw ex("too short");
        return *p;
    }

    void next() {
        if (!*p) throw ex("at last");
        if (*p == '\n') {
            ++line;
            col = 0;
        }
        ++p;
        ++col;
    }

    std::string ex(const std::string &msg) {
        std::stringstream ss;
        ss << "[line " << line << ", col " << col << "] " << msg;
        return ss.str();
    }
};

void parseTest(std::string (*p)(Source *), const char *s) {
    Source src = s;
    try {
        std::cout << p(&src) << std::endl;
    } catch (std::string e) {
        std::cout << e << std::endl;
    }
}

template<bool (*f)(char)> std::string satisfy(Source *s, const std::string &err) {
    char x = s->peek();
    if (!f(x)) throw s->ex("not " + err + ": '" + x + "'");
    s->next();
    return std::string(1, x);
}
template<bool (*f)(char)> std::string satisfy(Source *s) {
    return satisfy<f>(s, "???");
}

bool any(char) { return true; }
std::string anyChar(Source *s) { return satisfy<any>(s, "anyChar"); }

template<char c> bool isChar(char ch) { return ch == c; }
template<char c> std::string char1(Source *s) {
    return satisfy< isChar<c> >(s, std::string("char '") + c + "'");
}

bool isDigit   (char ch) { return std::isdigit(ch); }
bool isUpper   (char ch) { return std::isupper(ch); }
bool isLower   (char ch) { return std::islower(ch); }
bool isAlpha   (char ch) { return std::isalpha(ch); }
bool isAlphaNum(char ch) { return isalpha(ch) || isdigit(ch); }
bool isLetter  (char ch) { return isalpha(ch) || ch == '_';   }

std::string digit   (Source *s) { return satisfy<isDigit   >(s, "digit"   ); }
std::string upper   (Source *s) { return satisfy<isUpper   >(s, "upper"   ); }
std::string lower   (Source *s) { return satisfy<isLower   >(s, "lower"   ); }
std::string alpha   (Source *s) { return satisfy<isAlpha   >(s, "alpha"   ); }
std::string alphaNum(Source *s) { return satisfy<isAlphaNum>(s, "alphaNum"); }
std::string letter  (Source *s) { return satisfy<isLetter  >(s, "letter"  ); }

std::string test1(Source *s) {
    std::string x1 = anyChar(s);
    std::string x2 = anyChar(s);
    return x1 + x2;
}

std::string test2(Source *s) {
    std::string x1 = test1(s);
    std::string x2 = anyChar(s);
    return x1 + x2;
}

std::string test3(Source *s) {
    std::string x1 = letter(s);
    std::string x2 = digit(s);
    std::string x3 = digit(s);
    return x1 + x2 + x3;
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
