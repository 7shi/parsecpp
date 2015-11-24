#include <iostream>
#include <string>
#include <cctype>

bool isAlphaNum(char ch) { return isalpha(ch) || isdigit(ch); }
bool isLetter  (char ch) { return isalpha(ch) || ch == '_';   }

template<typename T> void parseTest(T (*p)(const char **), const char *s) {
    try {
        std::cout << p(&s) << std::endl;
    } catch (const char *e) {
        std::cout << e << std::endl;
    }
}

template<bool (*f)(char)> char satisfy(const char **xs) {
    if (!xs || !*xs) throw "null pointer";
    char x = **xs;
    if (x == '\0') throw "too short";
    if (!f(x)) throw "not satisfy";
    ++*xs;
    return x;
}

bool any(char) { return true; }
char (*anyChar)(const char **) = satisfy<any>;

template<char c> bool isChar(char ch) { return ch == c; }
template<char c> char char1(const char **xs) {
    return satisfy< isChar<c> >(xs);
}

template<int (*f)(int)> bool conv(char ch) { return f(ch); }
char (*digit   )(const char **) = satisfy< conv<std::isdigit> >;
char (*upper   )(const char **) = satisfy< conv<std::isupper> >;
char (*lower   )(const char **) = satisfy< conv<std::islower> >;
char (*alpha   )(const char **) = satisfy< conv<std::isalpha> >;
char (*alphaNum)(const char **) = satisfy< isAlphaNum >;
char (*letter  )(const char **) = satisfy< isLetter   >;

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
