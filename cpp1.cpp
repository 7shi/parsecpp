#include "parsecpp.cpp"

/*
test1 = do
    x1 <- anyChar
    x2 <- anyChar
    return [x1, x2]
*/
Parser<std::string> test1 = [](Source *s) {
    char x1 = anyChar(s);
    char x2 = anyChar(s);
    return std::string({x1, x2});
};

/*
test2 = do
    x1 <- test1
    x2 <- anyChar
    return $ x1 ++ [x2]
*/
Parser<std::string> test2 = [](Source *s) {
    auto x1 = test1(s);
    char x2 = anyChar(s);
    return x1 + x2;
};

/*
test3 = do
    x1 <- letter
    x2 <- digit
    x3 <- digit
    return [x1, x2, x3]
*/
Parser<std::string> test3 = [](Source *s) {
    char x1 = letter(s);
    char x2 = digit(s);
    char x3 = digit(s);
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
