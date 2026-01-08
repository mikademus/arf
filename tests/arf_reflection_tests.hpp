#ifndef ARF_TESTS_REFLECTION__
#define ARF_TESTS_REFLECTION__

#include "arf_test_harness.hpp"
#include "../include/arf.hpp"
#include "../include/arf_reflect.hpp"

namespace arf::tests
{

static void dump_resolve_errors(const arf::reflect::resolve_context& rctx)
{
    using namespace arf::reflect;

    for (const auto& e : rctx.errors)
    {
        std::cerr
            << "  [step " << e.step_index << "] "
            << resolve_error_string[static_cast<size_t>(e.kind)]
            << "\n";
    }
}

static bool reflect_empty_address_yields_no_value()
{
    using namespace arf::reflect;

    auto ctx = load("a:\n  x = 1\n");

    resolve_context rctx;
    rctx.doc = &ctx.document;

    auto v = resolve_ex(rctx, root());

    EXPECT(!v.has_value(), "empty address must not resolve");
    EXPECT(!rctx.has_errors(), "empty address must not report errors");
    return true;
}

static bool reflect_top_level_category_key()
{
    using namespace arf::reflect;

    auto ctx = load(
        "a:\n"
        "  x = 1\n"
    );

    resolve_context rctx;
    rctx.doc = &ctx.document;

    auto v =
        resolve_ex(
            rctx,
            root()
                .top("a")
                .key("x")
        );

    if (!v.has_value())
        dump_resolve_errors(rctx);

    EXPECT(v.has_value(), "top-level key not resolved");
    EXPECT((**v).type == value_type::integer, "wrong value type");
    EXPECT(!rctx.has_errors(), "unexpected errors during resolution");
    return true;
}

static bool reflect_explicit_subcategory_key()
{
    using namespace arf::reflect;

    auto ctx = load(
        "a:\n"
        "  :b:\n"
        "    x = 1\n"
    );

    resolve_context rctx;
    rctx.doc = &ctx.document;

    auto v =
        resolve_ex(
            rctx,
            root()
                .top("a")
                .sub("b")
                .key("x")
        );

    if (!v.has_value())
        dump_resolve_errors(rctx);

    EXPECT(v.has_value(), "nested subcategory key not resolved");
    EXPECT((**v).type == value_type::integer, "wrong value type");
    EXPECT(!rctx.has_errors(), "unexpected errors during resolution");
    return true;
}

static bool reflect_subcategory_without_context_fails()
{
    using namespace arf::reflect;

    auto ctx = load(
        "a:\n"
        "  x = 1\n"
    );

    resolve_context rctx;
    rctx.doc = &ctx.document;

    auto v =
        resolve_ex(
            rctx,
            root()
                .sub("a")
        );

    if (v.has_value())
        dump_resolve_errors(rctx);

    EXPECT(!v.has_value(), "sub-category without context must fail");
    EXPECT(rctx.has_errors(), "missing error for invalid sub-category");
    EXPECT(
        rctx.errors[0].kind == resolve_error_kind::no_category_context,
        "wrong error kind for invalid sub-category"
    );
    return true;
}

static bool reflect_top_level_category_does_not_nest()
{
    using namespace arf::reflect;

    auto ctx = load(
        "a:\n"
        "  :b:\n"
        "    x = 1\n"
    );

    resolve_context rctx;
    rctx.doc = &ctx.document;

    auto v =
        resolve_ex(
            rctx,
            root()
                .top("a")
                .top("b")
                .key("x")
        );

    if (v.has_value())
        dump_resolve_errors(rctx);

    EXPECT(!v.has_value(), "top-level lookup must not resolve subcategory");
    EXPECT(rctx.has_errors(), "expected error for invalid top-level category");
    EXPECT(
        rctx.errors[0].kind == resolve_error_kind::top_category_after_category,
        "wrong error kind for invalid top-level category"
    );
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

    resolve_context rctx;
    rctx.doc = &ctx.document;

    auto v =
        resolve_ex(
            rctx,
            root()
                .top("a")
                .local_table(0)
                .row(rid)
                .column("y")
        );

    if (!v.has_value())
        dump_resolve_errors(rctx);

    EXPECT(v.has_value(), "cell not resolved");
    EXPECT((**v).type == value_type::integer, "wrong cell type");
    EXPECT(!rctx.has_errors(), "unexpected errors during table cell resolution");
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

    resolve_context rctx;
    rctx.doc = &ctx.document;

    auto v =
        resolve_ex(
            rctx,
            root()
                .top("a")
                .table(tid)
                .row(rid)
                .column("nope")
        );

    EXPECT(!v.has_value(), "invalid column must not resolve");
    EXPECT(rctx.has_errors(), "missing error for invalid column");
    EXPECT(
        rctx.errors[0].kind == resolve_error_kind::column_not_found,
        "wrong error kind for invalid column"
    );
    return true;
}

static bool reflect_array_index()
{
    using namespace arf::reflect;

    auto ctx = load("a:\n  x:int[] = 1|2|3\n");

    resolve_context rctx;
    rctx.doc = &ctx.document;

    auto v =
        resolve_ex(
            rctx,
            root()
                .top("a")
                .key("x")
                .index(2)
        );

    if (!v.has_value())
        dump_resolve_errors(rctx);

    EXPECT(v.has_value(), "array element not resolved");
    EXPECT((**v).type == value_type::integer, "wrong array element type");
    EXPECT(!rctx.has_errors(), "unexpected errors during array indexing");
    return true;
}

static bool reflect_array_index_out_of_bounds_fails()
{
    using namespace arf::reflect;

    auto ctx = load("a:\n  x:int[] = 1|2|3\n");

    resolve_context rctx;
    rctx.doc = &ctx.document;

    auto v =
        resolve_ex(
            rctx,
            root()
                .top("a")
                .key("x")
                .index(99)
        );

    EXPECT(!v.has_value(), "out-of-bounds index must fail");
    EXPECT(rctx.has_errors(), "missing error for out-of-bounds index");
    EXPECT(
        rctx.errors[0].kind == resolve_error_kind::index_out_of_bounds,
        "wrong error kind for index out of bounds"
    );
    return true;
}

inline void run_reflection_tests()
{
    SUBCAT("Reflection");
    RUN_TEST(reflect_empty_address_yields_no_value);
    RUN_TEST(reflect_top_level_category_key);
    RUN_TEST(reflect_explicit_subcategory_key);
    RUN_TEST(reflect_subcategory_without_context_fails);
    RUN_TEST(reflect_top_level_category_does_not_nest);
    RUN_TEST(reflect_table_cell_by_column_name);
    RUN_TEST(reflect_invalid_column_fails);
    RUN_TEST(reflect_array_index);
    RUN_TEST(reflect_array_index_out_of_bounds_fails);
}

} // namespace arf::tests

#endif
