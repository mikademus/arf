#ifndef ARF_TESTS_DOCUMENT_STRUCTURE__
#define ARF_TESTS_DOCUMENT_STRUCTURE__

#include "arf_test_harness.hpp"
#include "../include/arf.hpp"

namespace arf::tests
{

static bool test_root_exists()
{
    auto doc = arf::load("");
    EXPECT(doc.has_errors() == false, "");
    EXPECT(doc->category_count() >= 1, "");
    EXPECT(doc->root().has_value(), "");
    return true;
}

static bool test_simple_category()
{
    constexpr std::string_view src =
        "category:\n";

    auto doc = load(src);
    EXPECT(doc.has_errors() == false, "");

    EXPECT(doc->category_count() == 2, ""); // root + category

    auto root = doc->root();
    EXPECT(root.has_value(), "");
    return true;
}

static bool test_nested_categories()
{
    constexpr std::string_view src =
        "outer:\n"
        "    :inner\n";

    auto doc = load(src);
    EXPECT(doc.has_errors() == false, "");

    EXPECT(doc->category_count() == 3, ""); // root, outer, inner
    return true;
}

static bool test_category_implicit_closure()
{
    constexpr std::string_view src =
        "a:\n"
        "    :b\n"
        ":c\n";

    auto doc = load(src);
    EXPECT(doc.has_errors() == false, "");

    // root, a, b, c
    EXPECT(doc->category_count() == 4, "");
    return true;
}

static bool test_simple_table()
{
    constexpr std::string_view src =
        "data:\n"
        "    # a  b\n"
        "      1  2\n"
        "      3  4\n";

    auto doc = load(src);
    EXPECT(doc.has_errors() == false, "");

    EXPECT(doc->table_count() == 1, "");
    EXPECT(doc->row_count() == 2, "");
    return true;
}

static bool test_table_in_subcategory()
{
    constexpr std::string_view src =
        "top:\n"
        "    :sub\n"
        "        # x y\n"
        "          a b\n";

    auto doc = load(src);
    EXPECT(doc.has_errors() == false, "");

    EXPECT(doc->table_count() == 1, "");
    EXPECT(doc->row_count() == 1, "");
    return true;
}

static bool test_multiple_root_tables()
{
    constexpr std::string_view src =
        "# a b\n"
        "  1 2\n"
        "\n"
        "# x y\n"
        "  3 4\n";

    auto doc = load(src);
    EXPECT(doc.has_errors() == false, "");

    EXPECT(doc->table_count() == 2, "");
    EXPECT(doc->row_count() == 2, "");

    auto root = doc->root();
    EXPECT(root.has_value(), "");
    EXPECT(root->tables().size() == 2, "");
    return true;
}

static bool test_keys_and_tables_interleaved()
{
    constexpr std::string_view src =
        "top:\n"
        "    a = 1\n"
        "    # x y\n"
        "      2 3\n"
        "    b = 4\n";

    auto doc = load(src);
    EXPECT(doc.has_errors() == false, "");

    auto root = doc->root();
    EXPECT(root.has_value(), "");

    EXPECT(doc->table_count() == 1, "");
    EXPECT(doc->row_count() == 1, "");

    // category access by traversal, not name (yet)
    EXPECT(doc->category_count() == 2, "");
    return true;
}

static bool test_root_and_subcategory_keys()
{
    constexpr std::string_view src =
        "x = 1\n"
        "c:\n"
        "    y = 2\n";

    auto doc = load(src);
    EXPECT(doc.has_errors() == false, "");
    EXPECT(doc->category_count() == 2, "");
    return true;
}

static bool test_deep_implicit_closure()
{
    constexpr std::string_view src =
        "a:\n"
        "  :b\n"
        "    :c\n"
        "d:\n";

    auto doc = load(src);
    EXPECT(doc.has_errors() == false, "");
    EXPECT(doc->category_count() == 5, ""); // root + a + b + c + d
    return true;
}

//----------------------------------------------------------------------------

inline void run_document_structure_tests()
{
    RUN_TEST(test_root_exists);
    RUN_TEST(test_simple_category);
    RUN_TEST(test_nested_categories);
    RUN_TEST(test_category_implicit_closure);
    RUN_TEST(test_simple_table);
    RUN_TEST(test_table_in_subcategory);
    RUN_TEST(test_multiple_root_tables);
    RUN_TEST(test_keys_and_tables_interleaved);
    RUN_TEST(test_root_and_subcategory_keys);
    RUN_TEST(test_deep_implicit_closure);
}

}

#endif