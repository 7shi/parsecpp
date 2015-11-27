#include "parsecpp.cpp"

int main() {
    Source s1 = "abc123";
    auto s1a = many(alpha)(&s1);
    auto s1b = many(digit)(&s1);
    std::cout << s1a << "," << s1b << std::endl;

    Source s2 = "abcde9";
    auto s2a = many(alpha)(&s2);
    auto s2b = many(digit)(&s2);
    std::cout << s2a << "," << s2b << std::endl;
}
