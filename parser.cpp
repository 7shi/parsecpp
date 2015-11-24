#include <iostream>
#include <string>
#include <cctype>

class Source {
private:
    const char *p;

public:
    Source(const char *p) : p(p) {}

    char peek() {
        if (!*p) throw std::string("too short");
        return *p;
    }

    void next() {
        if (!*p) throw std::string("at last");
        ++p;
    }
};

template<typename T> void parseTest(T (*p)(Source *), const char *s) {
    Source src = s;
    try {
        std::cout << p(&src) << std::endl;
    } catch (std::string e) {
        std::cout << e << std::endl;
    }
}

template<bool (*f)(char)> char satisfy(Source *s, const std::string &err) {
    char x = s->peek();
    if (!f(x)) throw "not " + err + ": '" + x + "'";
    s->next();
    return x;
}
template<bool (*f)(char)> char satisfy(Source *s) {
    return satisfy<f>(s, "???");
}

bool any(char) { return true; }
char anyChar(Source *s) { return satisfy<any>(s, "anyChar"); }

template<char c> bool isChar(char ch) { return ch == c; }
template<char c> char char1(Source *s) {
    return satisfy< isChar<c> >(s, std::string("char '") + c + "'");
}

bool isDigit   (char ch) { return std::isdigit(ch); }
bool isUpper   (char ch) { return std::isupper(ch); }
bool isLower   (char ch) { return std::islower(ch); }
bool isAlpha   (char ch) { return std::isalpha(ch); }
bool isAlphaNum(char ch) { return isalpha(ch) || isdigit(ch); }
bool isLetter  (char ch) { return isalpha(ch) || ch == '_';   }

char digit   (Source *s) { return satisfy<isDigit   >(s, "digit"   ); }
char upper   (Source *s) { return satisfy<isUpper   >(s, "upper"   ); }
char lower   (Source *s) { return satisfy<isLower   >(s, "lower"   ); }
char alpha   (Source *s) { return satisfy<isAlpha   >(s, "alpha"   ); }
char alphaNum(Source *s) { return satisfy<isAlphaNum>(s, "alphaNum"); }
char letter  (Source *s) { return satisfy<isLetter  >(s, "letter"  ); }

std::string test1(Source *s) {
    char x1 = anyChar(s);
    char x2 = anyChar(s);
    char ret[] = {x1, x2, 0};
    return ret;
}

std::string test2(Source *s) {
    std::string x1 = test1(s);
    char x2 = anyChar(s);
    return x1 + x2;
}

std::string test3(Source *s) {
    char x1 = letter(s);
    char x2 = digit(s);
    char x3 = digit(s);
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
