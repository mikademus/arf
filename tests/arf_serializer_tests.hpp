#ifndef ARF_TESTS_SERIALIZER__
#define ARF_TESTS_SERIALIZER__

#include "arf_test_harness.hpp"
#include "../include/arf_serializer.hpp"
#include "../include/arf.hpp"

namespace arf::tests
{
using namespace arf;

static bool foo()
{
    constexpr std::string_view src =
        "a = 42\n"
        "# x  y\n"
        "  1  2\n"
        "  3  4\n";

    auto ctx = load(src);
        
    EXPECT(ctx.errors.empty(), "error emitted");

    std::ostringstream out;
    auto s = serializer(ctx.document);
    s.write(out);

    std::cout << "Source:\n";
    std::cout << "\"" << src << "\"" << std::endl;

    std::cout << "\nOutput:\n";
    std::cout << "\"" << out.str() << "\"" << std::endl;

    EXPECT(out.str() == src, "Authored source not preserved");

    return true;
}    

inline void run_seriealizer_tests()
{
    RUN_TEST(foo);
}

} // ns arf::tests

#endif