// arf_reflect.hpp - A Readable Format (Arf!) - Reflection Interface
// Version 0.3.0
// Copyright 2025 Mikael Ueno A
// Licenced as-is under the MIT licence.

// This reflection surface is *value-centred* and *address-oriented*.
// There are no node identities, no row indices, no cell objects.
// Only values exist. Everything else is an address that can reach them.

#ifndef ARF_REFLECT_HPP
#define ARF_REFLECT_HPP

#include "arf_document.hpp"

#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace arf::reflect
{
    //============================================================
    // Forward declarations
    //============================================================

    struct context;

    //============================================================
    // Address steps
    //============================================================

    struct category_step
    {
        std::string_view name;
    };

    struct key_step
    {
        std::string_view name;
    };

    struct table_step
    {
        // tables are anonymous; resolved by position within category
        size_t ordinal;
    };

    struct row_step
    {
        table_row_id id;
    };

    struct column_step
    {
        std::string_view name;
    };

    struct array_index_step
    {
        size_t index;
    };

    using address_step = std::variant<
        category_step,
        key_step,
        table_step,
        row_step,
        column_step,
        array_index_step
    >;

    using address = std::vector<address_step>;


    //============================================================
    // Address builder helpers
    //------------------------------------------------------------
    // Usage:
    //     address addr;
    //     addr / category_step{"graphics"} / key_step{"resolution"};    
    //============================================================

    inline address& operator/(address& addr, category_step s) 
    { 
        addr.push_back(s); 
        return addr; 
    }
    
    inline address& operator/(address& addr, key_step s) 
    { 
        addr.push_back(s); 
        return addr; 
    }
        
    //============================================================
    // Evaluation context
    //============================================================

    struct context
    {
        const document* doc      {nullptr};
        category_id     category {invalid_id<category_tag>()};

        std::optional<document::category_view> category;
        std::optional<document::table_view>    table;
        std::optional<table_row_id>            row;

        const typed_value* value = nullptr;

        void reset_value()
        {
            value = nullptr;
            row.reset();
            table.reset();
        }
    };

    //============================================================
    // Step application
    //============================================================

    inline bool apply(context& ctx, const category_step& s)
    {
        if (!ctx.category)
        {
            auto cat = ctx.doc->category(s.name);
            if (!cat)
                return false;

            ctx.category = *cat;
            ctx.reset_value();
            return true;
        }

        auto child = ctx.category->child(s.name);
        if (!child)
            return false;

        ctx.category = *child;
        ctx.reset_value();
        return true;
    }

    inline bool apply(context& ctx, const key_step& s)
    {
        if (!ctx.category)
            return false;

        auto key = ctx.category->key(s.name);
        if (!key)
            return false;

        ctx.value = &key->value();
        return true;
    }

    inline bool apply(context& ctx, const table_step& s)
    {
        if (!ctx.category)
            return false;

        auto tables = ctx.category->tables();
        if (s.ordinal >= tables.size())
            return false;

        ctx.table = tables[s.ordinal];
        ctx.reset_value();
        return true;
    }

    inline bool apply(context& ctx, const row_step& s)
    {
        if (!ctx.table)
            return false;

        auto row = ctx.doc->row(s.id);
        if (!row)
            return false;

        ctx.row = s.id;
        ctx.reset_value();
        return true;
    }

    inline bool apply(context& ctx, const column_step& s)
    {
        if (!ctx.table || !ctx.row)
            return false;

        auto row = ctx.doc->row(*ctx.row);
        if (!row)
            return false;

        auto col_index = ctx.table->column_index(s.name);
        if (!col_index)
            return false;

        const auto& cells = row->node->cells;
        if (*col_index >= cells.size())
            return false;

        ctx.value = &cells[*col_index];
        return true;
    }

    inline bool apply(context& ctx, const array_index_step& s)
    {
        if (!ctx.value)
            return false;

        auto* arr = std::get_if<std::vector<typed_value>>(&ctx.value->val);
        if (!arr || s.index >= arr->size())
            return false;

        ctx.value = &(*arr)[s.index];
        return true;
    }

    //============================================================
    // Address evaluation
    //============================================================

    inline std::optional<const typed_value*>
    resolve(const document& doc, const address& addr)
    {
        context ctx;
        ctx.doc = &doc;
        ctx.category = doc.root();

        for (const auto& step : addr)
        {
            bool ok = std::visit(
                [&](auto const& s) { return apply(ctx, s); },
                step
            );

            if (!ok)
                return std::nullopt;
        }

        return ctx.value;
    }

} // namespace arf::reflect

#endif // ARF_REFLECT_HPP
