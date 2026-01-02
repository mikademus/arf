#ifndef ARF_TESTS_MATERIALISER__
#define ARF_TESTS_MATERIALISER__

/* *************************************************************************
 *  This test suite enforces semantic contracts of the ARF materialiser.   *
 *  Test names describe language policy, not implementation behaviour.     *
 ************************************************************************* */

#include "arf_test_harness.hpp"
#include "../include/arf.hpp"

namespace arf::tests
{

static bool scope_keys_are_category_local()
{
    constexpr std::string_view src =
        "a = 1\n"
        "cat:\n"
        "    a = 2\n";

    auto doc = load(src);
    EXPECT(doc.has_errors() == false, "error emitted");
    return true;
}

static bool scope_duplicate_keys_rejected()
{
    constexpr std::string_view src =
        "a = 1\n"
        "a = 2\n";

    auto doc = load(src);
    EXPECT(doc.has_errors() == false, "error emitted");
    return true;
}

static bool type_key_declared_mismatch_collapses()
{
    constexpr std::string_view src =
        "x:int = hello\n";

    auto ctx = load(src);
    EXPECT(ctx.has_errors(), "key type check incorrectly passed");
    return true;
}

static bool type_column_declared_mismatch_collapses()
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

static bool scope_named_collapse_unwinds_all()
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

static bool scope_invalid_named_close_is_error()
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

static bool scope_max_depth_enforced()
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

static bool type_key_invalid_declaration_is_error()
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

static bool type_column_invalid_declaration_is_error()
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

static bool semantic_invalid_key_flagged()
{
    constexpr std::string_view src =
        "x:dragon = 42\n";

    auto ctx = load(src);
    EXPECT(ctx.has_errors(), "no error emitted");

    auto doc = ctx.document;
    EXPECT(doc.key_count() == 1, "incorrect arity");

    auto k = doc.key(key_id{0});
    EXPECT(k.has_value(), "there is no key");
    EXPECT(k->node->semantic == semantic_state::invalid, "the invalid state flag is not set");
    EXPECT(k->value().type == value_type::string, "the key type has not collapsed to string");

    return true;
}

static bool semantic_invalid_column_flagged()
{
    constexpr std::string_view src =
        "# a:dragon\n"
        "  42\n";

    auto ctx = load(src);
    EXPECT(ctx.has_errors(), "no error emitted");

    auto doc = ctx.document;
    auto tbl = doc.table(table_id{0});
    EXPECT(tbl.has_value(), "there is no table");

    auto col = tbl->node->columns[0];
    EXPECT(col.semantic == semantic_state::invalid, "the invalid state flag is not set");
    EXPECT(col.type == value_type::string, "the key type has not collapsed to string");

    return true;
}

static bool semantic_invalid_cell_flagged()
{
    constexpr std::string_view src =
        "# a:int\n"
        "  hello\n";

    auto ctx = load(src);
    EXPECT(ctx.has_errors(), "no error emitted");

    auto row = ctx.document.row(table_row_id{0});
    EXPECT(row.has_value(), "there is no row");

    auto cell = row->node->cells[0];
    EXPECT(cell.semantic == semantic_state::invalid, "the invalid state flag is not set");
    EXPECT(cell.type == value_type::string, "the key type has not collapsed to string");

    return true;
}

static bool contamination_column_contaminates_rows_only()
{
    constexpr std::string_view src =
        "# a:int\n"
        "  hello\n";

    auto ctx = load(src);
    auto doc = ctx.document;

    auto tbl = doc.table(table_id{0});
    EXPECT(tbl.has_value(), "there is no table");
    EXPECT(tbl->is_contaminated(), "table should be contaminated by invalid column");
    EXPECT(tbl->is_locally_valid(), "table itself is not misformed"); 

    auto row = doc.row(table_row_id{0});
    EXPECT(row.has_value(), "there is no row");
    EXPECT(row->is_contaminated(), "row should be contaminated by invalid column");
    EXPECT(row->is_locally_valid(), "row itself is not malformed");    

    return true;
}

static bool view_exposes_row_invalidity()
{
    constexpr std::string_view src =
        "# a:int\n"
        "  42\n"
        "  nope\n";

    auto ctx = load(src);
    auto doc = ctx.document;

    auto row0 = doc.row(table_row_id{0});
    auto row1 = doc.row(table_row_id{1});

    EXPECT(!row0->is_contaminated(), "row should not be in contaminated state");
    EXPECT(row0->is_locally_valid(), "row should be structurally valid");

    EXPECT(row1->is_contaminated(), "row should be in contaminated state");
    EXPECT(row1->is_locally_valid(), "row should be structurally valid");

    return true;
}

static bool array_key_all_elements_valid()
{
    constexpr std::string_view src =
        "arr:int[] = 1|2|3\n";

    auto ctx = load(src);
    auto doc = ctx.document;

    auto key = doc.key(key_id{0});
    EXPECT(key.has_value(), "missing key");
    EXPECT(key->is_locally_valid(), "valid array key rejected");
    EXPECT(!key->is_contaminated(), "clean array key marked as contaminated");

    return true;
}

static bool array_invalid_element_contaminates_key()
{
    constexpr std::string_view src =
        "arr:int[] = 1|nope|3\n";

    auto ctx = load(src);
    auto doc = ctx.document;

    auto key = doc.key(key_id{0});
    EXPECT(key.has_value(), "missing key");
    EXPECT(key->is_locally_valid(), "invalid array element should not invalidate key");
    EXPECT(key->is_contaminated(), "invalid array element should contaminate key");

    return true;
}

static bool array_untyped_collapses_to_string()
{
    constexpr std::string_view src =
        "arr = 1|2|3\n";

    auto ctx = load(src);
    auto doc = ctx.document;

    auto key = doc.key(key_id{0});
    EXPECT(key.has_value(), "missing key");
    EXPECT(key->value().type == value_type::string, "unannotated array literal was not treated as string");

    return true;
}

static bool array_table_cells_valid()
{
    constexpr std::string_view src =
        "# id  vals:int[]\n"
        "  1   1|2|3\n"
        "  2   4|5\n";

    auto ctx = load(src);
    auto doc = ctx.document;

    auto row0 = doc.row(table_row_id{0});
    auto row1 = doc.row(table_row_id{1});

    EXPECT(row0->is_locally_valid(), "valid row rejected");
    EXPECT(row1->is_locally_valid(), "valid row rejected");

    return true;
}

static bool array_invalid_element_contaminates_row()
{
    constexpr std::string_view src =
        "# id  vals:int[]\n"
        "  1   1|2|nope\n"
        "  2   3|4\n";

    auto ctx = load(src);
    auto doc = ctx.document;

    auto row0 = doc.row(table_row_id{0});
    auto row1 = doc.row(table_row_id{1});

    EXPECT(row0->is_locally_valid(), "dirty row should be structurally valid");
    EXPECT(row0->is_contaminated(), "dirty row should be in contaimnated state");

    EXPECT(row1->is_locally_valid(), "clean row should be structurally valid");
    EXPECT(!row1->is_contaminated(), "clean row should not be in contaimnated state");

    return true;
}

static bool array_empty_elements_are_missing_not_contaminating()
{
    constexpr std::string_view src =
        "arr:str[] = a||b|\n";

    auto ctx = load(src);
    auto doc = ctx.document;

    auto key = doc.key(key_id{0});
    EXPECT(key.has_value(), "missing key");
    EXPECT(key->is_locally_valid(), "empty array elements should not invalidate array");
    EXPECT(!key->is_contaminated(), "empty array elements should not contaiminate array");

    return true;
}


//----------------------------------------------------------------------------

inline void run_materialiser_tests()
{
/*
1. Global structural & scoping rules
Policies / invariants

• Categories form a tree with lexical scoping
• Keys are scoped to categories
• Duplicate keys within the same category are illegal
• Same key name in different categories is legal
• Named collapse closes all intermediate scopes
• Invalid named closes are errors
• Maximum nesting depth is enforced
*/    
    RUN_TEST(scope_keys_are_category_local);
    RUN_TEST(scope_duplicate_keys_rejected);
    RUN_TEST(scope_named_collapse_unwinds_all);
    RUN_TEST(scope_invalid_named_close_is_error);
    RUN_TEST(scope_max_depth_enforced);

/*
2. Declared type handling (keys & columns)
Policies / invariants

• Declared types are parsed before coercion
• Unknown declared types are semantic errors
• Declared type mismatch does not drop data
• On mismatch, values collapse to string
• Collapsing marks local invalidity
*/    
    RUN_TEST(type_key_declared_mismatch_collapses);
    RUN_TEST(type_key_invalid_declaration_is_error);
    RUN_TEST(type_column_declared_mismatch_collapses);
    RUN_TEST(type_column_invalid_declaration_is_error);
    
/*
3. Local semantic validity vs contamination

Definitions (as implemented)

• Locally invalid = structurally or semantically malformed
• Contaminated = depends on invalid or contaminated content
• Invalidity does not propagate upward
• Contamination does propagate upward
• Contamination does not propagate downward

Policies / invariants

• Invalid members contaminate containers
• Containers remain locally valid if structurally intact
• Structural integrity is never inferred from child semantics
*/    
    RUN_TEST(semantic_invalid_key_flagged);
    RUN_TEST(semantic_invalid_column_flagged);
    RUN_TEST(semantic_invalid_cell_flagged);  
    RUN_TEST(contamination_column_contaminates_rows_only);
    RUN_TEST(view_exposes_row_invalidity);

/*
4. Arrays: parsing, coercion, and policy
Policies / invariants

• Arrays preserve element boundaries
• Empty elements are missing-but-valid
• Arrays may contain heterogeneously typed values
• Invalid elements contaminate the container
• Invalid elements do not invalidate the container
• Arrays without declared type collapse to string
• Trailing delimiters create empty elements
*/    
    RUN_TEST(array_key_all_elements_valid);
    RUN_TEST(array_invalid_element_contaminates_key);
    RUN_TEST(array_untyped_collapses_to_string);
    RUN_TEST(array_table_cells_valid);
    RUN_TEST(array_invalid_element_contaminates_row);
    RUN_TEST(array_empty_elements_are_missing_not_contaminating);
}

}

#endif