#ifndef ARF_TESTS_QUERIES__
#define ARF_TESTS_QUERIES__


#pragma once

#include "arf_test_harness.hpp"

#include "../include/arf_query.hpp"
#include "../include/arf.hpp"

namespace arf::tests
{
    using namespace arf;

    // -----------------------------------------------------------------
    // Basic path resolution
    // -----------------------------------------------------------------

    bool query_single_value()
    {
        auto ctx = load(R"(
            world:
                foo = 42
        )");

        auto v = get_integer(ctx.document, "world.foo");
        EXPECT(v.has_value(), "");
        EXPECT(*v == 42, "");

        return true;
    }

    // -----------------------------------------------------------------
    // Table row selection via predicate
    // -----------------------------------------------------------------

    bool query_table_row_by_string_id()
    {
        auto doc = load(R"(
            world:
                # race   poise
                  elves  friendly
                  orcs   hostile
        )");

        auto ctx =
            query_of(doc.document, "world")
                .where("race", "orcs")
                .select("poise")
                .eval();

        EXPECT(ctx.as_string().value() == "hostile", "");

        return true;
    }

    // -----------------------------------------------------------------
    // Plurality preservation
    // -----------------------------------------------------------------

    bool query_plural_results()
    {
        auto ctx = load(R"(
            world:
                # race poise
                  elves  friendly
                  orcs   hostile
                  orcs   drunk
        )");

        auto res =
            query_of(ctx.document, "world")
                .where("race", "orcs")
                .select("poise")
                .eval();

        EXPECT(res.strings().size() == 2, "");

        return true;
    }

    // -----------------------------------------------------------------
    // Ambiguity does not abort
    // -----------------------------------------------------------------

    bool query_reports_ambiguity()
    {
        auto ctx = load(R"(
            world:
                foo = 1
                foo = 2
        )");

        auto res = query_of(ctx.document, "world.foo").eval();

        EXPECT(res.ambiguous(), "");
        EXPECT(!res.issues().empty(), "");

        return true;
    }

    // -----------------------------------------------------------------
    // Ordinal table access
    // -----------------------------------------------------------------

    bool query_ordinal_table()
    {
        auto ctx = load(R"(
            world:
                # race   poise
                  elves  friendly

                # race  poise
                  orcs  hostile
        )");

        auto res =
            query_of(ctx.document, "world")
                .table(1)
                .where("race", "orcs")
                .select("poise")
                .eval();

        EXPECT(res.as_string().value() == "hostile", "");

        return true;
    }

    void run_query_tests()
    {
        RUN_TEST(query_single_value);
        RUN_TEST(query_table_row_by_string_id);
        RUN_TEST(query_plural_results);
        RUN_TEST(query_reports_ambiguity);
        RUN_TEST(query_ordinal_table);
    }
}


#endif
