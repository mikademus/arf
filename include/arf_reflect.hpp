// arf_reflect.hpp - A Readable Format (Arf!) - Reflection Interface
// Version 0.3.0
// Copyright 2025 Mikael Ueno A
// Licenced as-is under the MIT licence.
//
// This reflection surface is *value-centred* and *address-oriented*.
// There are no node identities, no row indices, no cell objects.
// Only values exist. Everything else is an address that can reach them.

#ifndef ARF_REFLECT_H__
#define ARF_REFLECT_H__
#ifndef ARF_REFLECT_HPP
#define ARF_REFLECT_HPP

#include "arf_core.hpp"
#include "arf_document.hpp"

#include <variant>
#include <vector>
#include <optional>
#include <string_view>

namespace arf::reflect
{

    // ------------------------------------------------------------
    // address steps
    // ------------------------------------------------------------

    struct category_step
    {
        std::variant<category_id, std::string_view> id;
    };

    struct key_step
    {
        std::variant<key_id, std::string_view> id;
    };

    struct table_step
    {
        std::variant<table_id, size_t> id; // id or local ordinal
    };

    struct row_step
    {
        table_row_id id;
    };

    struct column_step
    {
        std::variant<column_id, std::string_view> id;
    };

    struct index_step
    {
        size_t index;
    };

    using address_step =
        std::variant<
            category_step,
            key_step,
            table_step,
            row_step,
            column_step,
            index_step
        >;

    // ------------------------------------------------------------
    // address builder
    // ------------------------------------------------------------

    struct address
    {
        std::vector<address_step> steps;

        address& category(category_id id)
        {
            steps.push_back(category_step{ id });
            return *this;
        }

        address& category(std::string_view name)
        {
            steps.push_back(category_step{ name });
            return *this;
        }

        address& key(key_id id)
        {
            steps.push_back(key_step{ id });
            return *this;
        }

        address& key(std::string_view name)
        {
            steps.push_back(key_step{ name });
            return *this;
        }

        address& table(table_id id)
        {
            steps.push_back(table_step{ id });
            return *this;
        }

        address& local_table(size_t ordinal)
        {
            steps.push_back(table_step{ ordinal });
            return *this;
        }

        address& row(table_row_id id)
        {
            steps.push_back(row_step{ id });
            return *this;
        }

        address& column(column_id id)
        {
            steps.push_back(column_step{ id });
            return *this;
        }

        address& column(std::string_view name)
        {
            steps.push_back(column_step{ name });
            return *this;
        }

        address& index(size_t i)
        {
            steps.push_back(index_step{ i });
            return *this;
        }
    };

    inline address root()
    {
        return address{};
    }

    // ------------------------------------------------------------
    // resolution context
    // ------------------------------------------------------------

    struct resolve_context
    {
        const document* doc = nullptr;

        std::optional<document::category_view> category;
        std::optional<document::table_view>    table;
        std::optional<document::table_row_view> row;
        std::optional<document::column_view>   column;
        const typed_value*           value = nullptr;
    };

    // ------------------------------------------------------------
    // helpers
    // ------------------------------------------------------------

    inline std::optional<table_id>
    resolve_table_ordinal(const document::category_view& cat, size_t ordinal)
    {
        auto tbls = cat.tables();
        if (ordinal >= tbls.size())
            return std::nullopt;

        return tbls[ordinal];
    }

    inline bool is_array( value_type v )
    {
        return v == value_type::string_array || v == value_type::int_array || v == value_type::float_array;
    }


    // ------------------------------------------------------------
    // resolve
    // ------------------------------------------------------------

    inline std::optional<const typed_value*>
    resolve(const document& doc, const address& addr)
    {
        resolve_context ctx;
        ctx.doc = &doc;
        ctx.category = doc.root();

        if (addr.steps.empty())
            return std::nullopt;

        for (const auto& step : addr.steps)
        {
            // ---------------- category
            if (auto s = std::get_if<category_step>(&step))
            {
                if (!ctx.category)
                    return std::nullopt;

                if (std::holds_alternative<category_id>(s->id))
                {
                    ctx.category = doc.category(std::get<category_id>(s->id));
                }
                else
                {
                    ctx.category = ctx.category->child(std::get<std::string_view>(s->id));
                }

                ctx.table.reset();
                ctx.row.reset();
                ctx.column.reset();
                ctx.value = nullptr;

                if (!ctx.category)
                    return std::nullopt;
            }

            // ---------------- key
            else if (auto s = std::get_if<key_step>(&step))
            {
                if (!ctx.category)
                    return std::nullopt;

                std::optional<document::key_view> k;

                if (std::holds_alternative<key_id>(s->id))
                    k = doc.key(std::get<key_id>(s->id));
                else
                    k = ctx.category->key(std::get<std::string_view>(s->id));

                if (!k)
                    return std::nullopt;

                ctx.category.reset();
                ctx.table.reset();
                ctx.row.reset();
                ctx.column.reset();
                ctx.value = &k->value();
            }

            // ---------------- table
            else if (auto s = std::get_if<table_step>(&step))
            {
                if (!ctx.category)
                    return std::nullopt;

                std::optional<table_id> tid;

                if (std::holds_alternative<table_id>(s->id))
                    tid = std::get<table_id>(s->id);
                else
                    tid = resolve_table_ordinal(*ctx.category, std::get<size_t>(s->id));

                if (!tid)
                    return std::nullopt;

                ctx.table = doc.table(*tid);
                ctx.row.reset();
                ctx.column.reset();
                ctx.value = nullptr;

                if (!ctx.table)
                    return std::nullopt;
            }

            // ---------------- row
            else if (auto s = std::get_if<row_step>(&step))
            {
                if (!ctx.table)
                    return std::nullopt;

                ctx.row = doc.row(s->id);
                ctx.column.reset();
                ctx.value = nullptr;

                if (!ctx.row)
                    return std::nullopt;

                // ownership validation
                bool owned = false;
                for (auto rid : ctx.table->rows())
                {
                    if (rid == s->id)
                    {
                        owned = true;
                        break;
                    }
                }

                if (!owned)
                    return std::nullopt;
            }

            // ---------------- column
            else if (auto s = std::get_if<column_step>(&step))
            {
                if (!ctx.table || !ctx.row)
                    return std::nullopt;

                std::optional<document::column_view> col;

                if (std::holds_alternative<column_id>(s->id))
                    col = doc.column(std::get<column_id>(s->id));
                else
                    col = ctx.table->column(std::get<std::string_view>(s->id));

                if (!col)
                    return std::nullopt;

                ctx.column = col;

                auto idx = col->index();
                ctx.value = &ctx.row->cells()[idx];
            }

            // ---------------- index
            else if (auto s = std::get_if<index_step>(&step))
            {
                if (!ctx.value)
                    return std::nullopt;

                if (!is_array(ctx.value->type))
                    return std::nullopt;

                auto& arr = std::get<std::vector<typed_value>>(ctx.value->val);

                if (s->index >= arr.size())
                    return std::nullopt;

                ctx.value = &arr[s->index];
            }
        }

        return ctx.value;
    }

} // namespace arf::reflect

#endif // ARF_REFLECT_HPP
