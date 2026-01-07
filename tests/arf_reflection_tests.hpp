#ifndef ARF_TESTS_REFLECTION__
#define ARF_TESTS_REFLECTION__

#include "arf_test_harness.hpp"
#include "../include/arf.hpp"
#include "../include/arf_reflect.hpp"

namespace arf::tests
{

static bool reflect_empty_address_yields_no_value()
{
    auto ctx = load("a:\n  x = 1\n");

    auto v = arf::reflect::resolve(ctx.document, arf::reflect::root());
    EXPECT(!v.has_value(), "empty address must not resolve");
    return true;
}

static bool reflect_nested_category_key()
{
    using namespace arf::reflect;

    auto ctx = load(
        "a:\n"
        "  b:\n"
        "    x = 1\n");

    auto v =
        resolve(
            ctx.document,
                root()
                .category("a")
                .category("b")
                .key("x")
        );

    EXPECT(v.has_value(), "nested key not resolved");
    EXPECT((**v).type == value_type::integer, "wrong value type");
    return true;
}

static bool reflect_table_cell_by_column_name()
{
    using namespace arf::reflect;

    auto ctx = load(
        "a:\n"
        "  # x y\n"
        "    1 2\n"
    );

    auto cat = ctx.document.category("a");
    auto tid = cat->tables()[0];
    auto rid = ctx.document.table(tid)->rows()[0];

    auto v =
        arf::reflect::resolve(
            ctx.document,
            arf::reflect::root()
                .category("a")
                .local_table(0)
                .row(rid)
                .column("y")
        );

    EXPECT(v.has_value(), "cell not resolved");
    EXPECT((**v).type == value_type::integer, "wrong cell type");
    return true;
}

static bool reflect_invalid_column_fails()
{
    using namespace arf::reflect;

    auto ctx = load(
        "a:\n"
        "  # x\n"
        "    1\n"
    );

    auto cat = ctx.document.category("a");
    auto tid = cat->tables()[0];
    auto rid = ctx.document.table(tid)->rows()[0];

    auto v =
        resolve(
            ctx.document,
                root()
                .category("a")
                .table(tid)
                .row(rid)
                .column("nope")
        );

    EXPECT(!v.has_value(), "invalid column must not resolve");
    return true;
}

static bool reflect_array_index()
{
    using namespace arf::reflect;

    auto ctx = load("a:\n  x:int[] = 1|2|3\n");

    auto v =
        resolve(
            ctx.document,
                root()
                .category("a")
                .key("x")
                .index(2)
        );

    EXPECT(v.has_value(), "array element not resolved");
    EXPECT((**v).type == value_type::integer, "wrong array element type");
    return true;
}

inline void run_reflection_tests()
{
    SUBCAT("Reflection");
    RUN_TEST(reflect_empty_address_yields_no_value);
    RUN_TEST(reflect_nested_category_key);
    RUN_TEST(reflect_table_cell_by_column_name);
    RUN_TEST(reflect_invalid_column_fails);
    RUN_TEST(reflect_array_index);
}

} // namespace arf::tests

#endif
