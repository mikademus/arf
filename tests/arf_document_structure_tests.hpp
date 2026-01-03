#ifndef ARF_TESTS_DOCUMENT_STRUCTURE__
#define ARF_TESTS_DOCUMENT_STRUCTURE__

#include "arf_test_harness.hpp"
#include "../include/arf.hpp"

namespace arf::tests
{

static bool document_root_always_exists()
{
    auto doc = arf::load("");
    EXPECT(doc.has_errors() == false, "");
    EXPECT(doc->category_count() >= 1, "");
    EXPECT(doc->root().has_value(), "");
    return true;
}

static bool document_category_single_level_attaches_to_root()
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

static bool document_category_nested_ownership_preserved()
{
    constexpr std::string_view src =
        "outer:\n"
        "    :inner\n";

    auto doc = load(src);
    EXPECT(doc.has_errors() == false, "");

    EXPECT(doc->category_count() == 3, ""); // root, outer, inner
    return true;
}

static bool document_colon_categories_nest_without_explicit_closure()
{
    constexpr std::string_view src =
        "a:\n"
        "    :b\n"
        ":c\n";

    auto doc = load(src);
    EXPECT(!doc.has_errors(), "rejected correct script");

    EXPECT(doc->category_count() == 4, "expected root, a, b, c");

    auto c = doc->category(category_id{3});
    EXPECT(c.has_value(), "category c must exist");

    auto parent = c->parent();
    EXPECT(parent.has_value(), "category c must have a parent");
    EXPECT(c->parent()->name() == "b", "category c must attach to b");

    return true;
}

static bool document_table_at_root_allowed()
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

static bool document_table_inside_category_allowed()
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

static bool document_multiple_tables_at_same_scope_allowed()
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

static bool document_keys_attach_to_current_category()
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

static bool document_root_key_before_category_is_allowed()
{
    constexpr std::string_view src =
        "x = 1\n"
        "c:\n"
        "    y = 2\n";

    auto doc = load(src);
    EXPECT(doc.has_errors() == false, "document should parse without errors");

    auto root = doc->root();
    EXPECT(root.has_value(), "root category must exist");

    // root key
    auto key0 = doc->key(key_id{0});
    EXPECT(key0.has_value(), "root key must exist");
    EXPECT(key0->owner().is_root(), "key defined before category must attach to root");

    // category key
    auto key1 = doc->key(key_id{1});
    EXPECT(key1.has_value(), "category key must exist");
    EXPECT(!key1->owner().is_root(), "key defined inside category must not attach to root");
    EXPECT(key1->owner().name() == "c", "key must attach to category c");

    return true;
}

static bool document_category_explicit_nesting_does_not_leak_scope()
{
    constexpr std::string_view src =
        "a:\n"
        "  :b\n"
        "    :c\n"
        "d:\n";

    auto doc = load(src);
    EXPECT(doc.has_errors() == false, "document must parse without errors");
    EXPECT(doc->category_count() == 5, "expected root + a + b + c + d");

    auto root = doc->root();
    EXPECT(root.has_value(), "root category must exist");

    // Retrieve categories by traversal
    auto a = doc->category(category_id{1});
    auto b = doc->category(category_id{2});
    auto c = doc->category(category_id{3});
    auto d = doc->category(category_id{4});

    EXPECT(a->parent()->is_root(), "a must attach to root");
    EXPECT(b->parent()->name() == "a", "b must attach to a");
    EXPECT(c->parent()->name() == "b", "c must attach to b");
    EXPECT(d->parent()->is_root(), "d must attach to root after nested declarations");

    return true;}

//----------------------------------------------------------------------------

inline void run_document_structure_tests()
{
/*
1. Root and ownership invariants
• There is exactly one implicit root category
• Everything has a well-defined owner
• Ownership is hierarchical and acyclic
*/    
    RUN_TEST(document_root_always_exists);

/*
2. Category formation rules
• Categories can nest
• Categories implicitly close when indentation decreases
• Deep nesting is handled correctly
*/
    RUN_TEST(document_category_single_level_attaches_to_root);
    RUN_TEST(document_category_nested_ownership_preserved);
    RUN_TEST(document_colon_categories_nest_without_explicit_closure);
    RUN_TEST(document_category_explicit_nesting_does_not_leak_scope);

/*
3. Table placement rules
• Tables may appear at root or inside categories
• Tables belong to exactly one category
• Multiple tables at the same level are allowed
*/
    RUN_TEST(document_table_at_root_allowed);
    RUN_TEST(document_table_inside_category_allowed);
    RUN_TEST(document_multiple_tables_at_same_scope_allowed);

/*
Key placement rules

• Keys may appear at root or inside categories
• Keys and tables may interleave
• Keys attach to the correct owning category regardless of order
*/
    RUN_TEST(document_keys_attach_to_current_category);
    RUN_TEST(document_root_key_before_category_is_allowed);
}

}

#endif