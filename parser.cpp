#include <iostream>
#include <string>
#include <cctype>

bool isAlphaNum(char ch) { return isalpha(ch) || isdigit(ch); }
bool isLetter  (char ch) { return isalpha(ch) || ch == '_';   }

template<typename T> void parseTest(T (*p)(const char **), const char *s) {
    try {
        std::cout << p(&s) << std::endl;
    } catch (std::string e) {
        std::cout << e << std::endl;
    }
}

template<bool (*f)(char)> char satisfy(const char **xs, const std::string &err) {
    if (!xs || !*xs) throw std::string("null pointer");
    char x = **xs;
    if (x == '\0') throw std::string("too short");
    if (!f(x)) throw "not " + err + ": '" + x + "'";
    ++*xs;
    return x;
}
template<bool (*f)(char)> char satisfy(const char **xs) {
    return satisfy<f>(xs, "???");
}
template<int (*f)(int)> bool conv(char ch) { return f(ch); }
template<int (*f)(int)> char satisfy(const char **xs, const std::string &err) {
    return satisfy< conv<f> >(xs, err);
}

bool any(char) { return true; }
char anyChar(const char **xs) { return satisfy<any>(xs, "anyChar"); }

template<char c> bool isChar(char ch) { return ch == c; }
template<char c> char char1(const char **xs) {
    return satisfy< isChar<c> >(xs, std::string("char '") + c + "'");
}

char digit   (const char **xs) { return satisfy<std::isdigit>(xs, "digit"   ); }
char upper   (const char **xs) { return satisfy<std::isupper>(xs, "upper"   ); }
char lower   (const char **xs) { return satisfy<std::islower>(xs, "lower"   ); }
char alpha   (const char **xs) { return satisfy<std::isalpha>(xs, "alpha"   ); }
char alphaNum(const char **xs) { return satisfy<isAlphaNum  >(xs, "alphaNum"); }
char letter  (const char **xs) { return satisfy<isLetter    >(xs, "letter"  ); }

std::string test1(const char **xs) {
    char x1 = anyChar(xs);
    char x2 = anyChar(xs);
    char ret[] = {x1, x2, 0};
    return ret;
}

std::string test2(const char **xs) {
    std::string x1 = test1(xs);
    char x2 = anyChar(xs);
    return x1 + x2;
}

std::string test3(const char **xs) {
    char x1 = letter(xs);
    char x2 = digit(xs);
    char x3 = digit(xs);
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
