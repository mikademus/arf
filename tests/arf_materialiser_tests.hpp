#ifndef ARF_TESTS_MATERIALISER__
#define ARF_TESTS_MATERIALISER__

#include "arf_test_harness.hpp"
#include "../include/arf.hpp"

namespace arf::tests
{

static bool test_same_key_in_different_categories_allowed()
{
    constexpr std::string_view src =
        "a = 1\n"
        "cat:\n"
        "    a = 2\n";

    auto doc = load(src);
    EXPECT(doc.has_errors() == false, "error emitted");
    return true;
}

static bool test_duplicate_key_rejected()
{
    constexpr std::string_view src =
        "a = 1\n"
        "a = 2\n";

    auto doc = load(src);
    EXPECT(doc.has_errors() == false, "error emitted");
    return true;
}

static bool test_declared_key_type_mismatch()
{
    constexpr std::string_view src =
        "x:int = hello\n";

    auto ctx = load(src);
    EXPECT(ctx.has_errors(), "key type check incorrectly passed");
    return true;
}

static bool test_declared_column_type_mismatch()
{
    constexpr std::string_view src =
        "# a:int\n"
        "  hello\n";

    auto ctx = load(src);
    EXPECT(ctx.has_errors(), "column type check incorrectly passed");
    auto const & e0 = ctx.errors[0].kind;
    EXPECT(is_material_error(e0) && get_material_error(e0) == semantic_error_kind::type_mismatch, "wrong error code");
    return true;
}

static bool test_named_collapse_closes_multiple_scopes()
{
    constexpr std::string_view src =
        ":a\n"
        "  :b\n"
        "    :c\n"
        "/a\n";

    auto ctx = load(src);
    EXPECT(!ctx.has_errors(), "error emitted");
    EXPECT(ctx.document.category_count() == 4, "incorrect arity"); // root + a + b + c
    return true;
}

static bool test_invalid_named_close_is_error()
{
    constexpr std::string_view src =
        ":a\n"
        "/b\n";

    auto ctx = load(src);
    EXPECT(ctx.has_errors(), "no error emitted");
    auto const & e0 = ctx.errors[0].kind;
    EXPECT(is_material_error(e0), "incorrect (non-semantic) error type");
    EXPECT(get_material_error(e0) == semantic_error_kind::invalid_category_close, "incorrect error code");
    return true;
}

static bool test_max_nesting_depth_enforced()
{
    materialiser_options opts;
    opts.max_category_depth = 2;

    constexpr std::string_view src =
        ":a\n"
        "  :b\n"
        "    :c\n";

    auto parse_ctx = parse(src);
    auto ctx = materialise(parse_ctx, opts);
    EXPECT(ctx.has_errors(), "no error emitted");
    EXPECT(ctx.errors.front().kind == semantic_error_kind::depth_exceeded, "incorrect error code");
    return true;
}

static bool test_invalid_declared_key_type_is_error()
{
    constexpr std::string_view src =
        "x:dragon = 42\n";

    auto ctx = load(src);
    EXPECT(ctx.has_errors(), "no error emitted");

    auto e = ctx.errors.front().kind;
    EXPECT(is_material_error(e), "incorrect (non-semantic) error type");
    EXPECT(get_material_error(e) == semantic_error_kind::invalid_declared_type, "incorrect error code");

    return true;
}

static bool test_invalid_declared_column_type_is_error()
{
    constexpr std::string_view src =
        "# a:dragon\n"
        "  42\n";

    auto ctx = load(src);
    EXPECT(ctx.has_errors(), "no error emitted");

    auto e = ctx.errors.front().kind;
    EXPECT(is_material_error(e), "incorrect (non-semantic) error type");
    EXPECT(get_material_error(e) == semantic_error_kind::invalid_declared_type, "incorrect error code");

    return true;
}


//----------------------------------------------------------------------------

inline void run_materialiser_tests()
{
    RUN_TEST(test_same_key_in_different_categories_allowed);
    RUN_TEST(test_duplicate_key_rejected);
    RUN_TEST(test_declared_key_type_mismatch);
    RUN_TEST(test_declared_column_type_mismatch);
    RUN_TEST(test_named_collapse_closes_multiple_scopes);
    RUN_TEST(test_invalid_named_close_is_error);
    RUN_TEST(test_max_nesting_depth_enforced);
    RUN_TEST(test_invalid_declared_key_type_is_error);
    RUN_TEST(test_invalid_declared_column_type_is_error);
}

}

#endif