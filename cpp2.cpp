#include "parsecpp.cpp"

/*
test1  = sequence [anyChar, anyChar]
test2  = (++) <$> test1 <*> sequence [anyChar]
test3  = sequence [letter, digit, digit]
test4  = letter <|> digit
test5  = sequence [letter, digit, digit, digit]
test6  = sequence $ letter : replicate 3 digit
test7  = many letter
test8  = many (letter <|> digit)
test9  =      sequence [char 'a', char 'b']
         <|>  sequence [char 'a', char 'c']
test10 = try (sequence [char 'a', char 'b'])
         <|>  sequence [char 'a', char 'c']
test11 =      string "ab"  <|> string "ac"
test12 = try (string "ab") <|> string "ac"
test13 = sequence [char 'a', char 'b' <|> char 'c']
*/
auto test1  = anyChar + anyChar;
auto test2  = test1 + anyChar;
auto test3  = letter + digit + digit;
auto test4  = letter || digit;
auto test5  = letter + digit + digit + digit;
auto test6  = letter + 3 * digit;
auto test7  = many(letter);
auto test8  = many(letter || digit);
auto test9  =      char1('a') + char1('b')  || char1('a') + char1('c');
auto test10 = tryp(char1('a') + char1('b')) || char1('a') + char1('c');
auto test11 =      string("ab")  || string("ac");
auto test12 = tryp(string("ab")) || string("ac");
auto test13 = char1('a') + (char1('b') || char1('c'));

/*
main = do
    parseTest anyChar    "abc"
    parseTest test1      "abc"
    parseTest test2      "abc"
    parseTest test2      "12"      -- NG
    parseTest test2      "123"
    parseTest (char 'a') "abc"
    parseTest (char 'a') "123"     -- NG
    parseTest digit      "abc"     -- NG
    parseTest digit      "123"
    parseTest letter     "abc"
    parseTest letter     "123"     -- NG
    parseTest test3      "abc"     -- NG
    parseTest test3      "123"     -- NG
    parseTest test3      "a23"
    parseTest test3      "a234"
    parseTest test4      "a"
    parseTest test4      "1"
    parseTest test4      "!"       -- NG
    parseTest test5      "a123"
    parseTest test5      "ab123"   -- NG
    parseTest test6      "a123"
    parseTest test6      "ab123"   -- NG
    parseTest test7      "abc123"
    parseTest test7      "123abc"
    parseTest test8      "abc123"
    parseTest test8      "123abc"
    parseTest test9      "ab"
    parseTest test9      "ac"      -- NG
    parseTest test10     "ab"
    parseTest test10     "ac"
    parseTest test11     "ab"
    parseTest test11     "ac"      -- NG
    parseTest test12     "ab"
    parseTest test12     "ac"
    parseTest test13     "ab"
    parseTest test13     "ac"
*/
int main() {
    parseTest(anyChar   , "abc"   );
    parseTest(test1     , "abc"   );
    parseTest(test2     , "abc"   );
    parseTest(test2     , "12"    );  // NG
    parseTest(test2     , "123"   );
    parseTest(char1('a'), "abc"   );
    parseTest(char1('a'), "123"   );  // NG
    parseTest(digit     , "abc"   );  // NG
    parseTest(digit     , "123"   );
    parseTest(letter    , "abc"   );
    parseTest(letter    , "123"   );  // NG
    parseTest(test3     , "abc"   );  // NG
    parseTest(test3     , "123"   );  // NG
    parseTest(test3     , "a23"   );
    parseTest(test3     , "a234"  );
    parseTest(test4     , "a"     );
    parseTest(test4     , "1"     );
    parseTest(test4     , "!"     );  // NG
    parseTest(test5     , "a123"  );
    parseTest(test5     , "ab123" );  // NG
    parseTest(test6     , "a123"  );
    parseTest(test6     , "ab123" );  // NG
    parseTest(test7     , "abc123");
    parseTest(test7     , "123abc");
    parseTest(test8     , "abc123");
    parseTest(test8     , "123abc");
    parseTest(test9     , "ab"    );
    parseTest(test9     , "ac"    );  // NG
    parseTest(test10    , "ab"    );
    parseTest(test10    , "ac"    );
    parseTest(test11    , "ab"    );
    parseTest(test11    , "ac"    );  // NG
    parseTest(test12    , "ab"    );
    parseTest(test12    , "ac"    );
    parseTest(test13    , "ab"    );
    parseTest(test13    , "ac"    );
}
