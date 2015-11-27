#include <iostream>
#include <string>
#include <functional>

template <typename T>
using Parser = std::function<T (const char **)>;

/*
parseTest p s = do
    print $ evalState p s
    `catch` \(SomeException e) ->
        putStr $ show e
*/
template <typename T>
void parseTest(const Parser<T> &p, const char *s) {
    try {
        std::cout << p(&s) << std::endl;
    } catch (const char *e) {
        std::cout << e << std::endl;
    }
}

/*
satisfy f = state $ satisfy where
    satisfy (x:xs) | f x = (x, xs)
*/
Parser<char> satisfy(const std::function<bool (char)> &f) {
    return [=](const char **xs) {
        if (!xs || !*xs) throw "null pointer";
        char x = **xs;
        if (x == '\0') throw "too short";
        if (!f(x)) throw "not satisfy";
        ++*xs;
        return x;
    };
}

/*
anyChar = state $ anyChar where
    anyChar (x:xs) = (x, xs)
*/
auto anyChar = satisfy([](char) { return true; });

/*
char c = satisfy (== c)
*/
Parser<char> char1(char ch) {
    return satisfy([=](char c) { return c == ch; });
}
/*
import Data.Char
*/
#include <cctype>
bool isDigit   (char ch) { return std::isdigit(ch); }
bool isUpper   (char ch) { return std::isupper(ch); }
bool isLower   (char ch) { return std::islower(ch); }
bool isAlpha   (char ch) { return std::isalpha(ch); }
bool isAlphaNum(char ch) { return isalpha(ch) || isdigit(ch); }
bool isLetter  (char ch) { return isalpha(ch) || ch == '_';   }

/*
digit    = satisfy isDigit
upper    = satisfy isUpper
lower    = satisfy isLower
alpha    = satisfy isAlpha
alphaNum = satisfy isAlphaNum
letter   = satisfy isLetter
*/
auto digit    = satisfy(isDigit   );
auto upper    = satisfy(isUpper   );
auto lower    = satisfy(isLower   );
auto alpha    = satisfy(isAlpha   );
auto alphaNum = satisfy(isAlphaNum);
auto letter   = satisfy(isLetter  );

/*
test1 = do
    x1 <- anyChar
    x2 <- anyChar
    return [x1, x2]
*/
Parser<std::string> test1 = [](const char **xs) {
    char x1 = anyChar(xs);
    char x2 = anyChar(xs);
    return std::string({x1, x2});
};

/*
test2 = do
    x1 <- test1
    x2 <- anyChar
    return $ x1 ++ [x2]
*/
Parser<std::string> test2 = [](const char **xs) {
    auto x1 = test1(xs);
    char x2 = anyChar(xs);
    return x1 + x2;
};

/*
test3 = do
    x1 <- letter
    x2 <- digit
    x3 <- digit
    return [x1, x2, x3]
*/
Parser<std::string> test3 = [](const char **xs) {
    char x1 = letter(xs);
    char x2 = digit(xs);
    char x3 = digit(xs);
    return std::string({x1, x2, x3});
};

/*
main = do
    parseTest anyChar "abc"
    parseTest test1   "abc"
    parseTest test2   "abc"
    parseTest test2   "12"      -- NG
    parseTest test2   "123"
    parseTest (char 'a') "abc"
    parseTest (char 'a') "123"  -- NG
    parseTest digit  "abc"      -- NG
    parseTest digit  "123"
    parseTest letter "abc"
    parseTest letter "123"      -- NG
    parseTest test3  "abc"      -- NG
    parseTest test3  "123"      -- NG
    parseTest test3  "a23"
    parseTest test3  "a234"
*/
int main() {
    parseTest(anyChar   , "abc" );
    parseTest(test1     , "abc" );
    parseTest(test2     , "abc" );
    parseTest(test2     , "12"  );  // NG
    parseTest(test2     , "123" );
    parseTest(char1('a'), "abc" );
    parseTest(char1('a'), "123" );  // NG
    parseTest(digit     , "abc" );  // NG
    parseTest(digit     , "123" );
    parseTest(letter    , "abc" );
    parseTest(letter    , "123" );  // NG
    parseTest(test3     , "abc" );  // NG
    parseTest(test3     , "123" );  // NG
    parseTest(test3     , "a23" );
    parseTest(test3     , "a234");
}
