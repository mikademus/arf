// arf_reflect.hpp - A Readable Format (Arf!) - Reflection Interface
// Version 0.3.0
// Copyright 2025 Mikael Ueno A
// Licenced as-is under the MIT licence.
//
// This reflection surface is *value-centred* and *address-oriented*.
// There are no node identities, no row indices, no cell objects.
// Only values exist. Everything else is an address that can reach them.

#ifndef ARF_REFLECT_HPP
#define ARF_REFLECT_HPP

#include "arf_core.hpp"
#include "arf_document.hpp"

#include <array>
#include <variant>
#include <vector>
#include <optional>
#include <string_view>

namespace arf::reflect
{

    // ------------------------------------------------------------
    // address steps
    // ------------------------------------------------------------

    struct top_category_step
    {
        std::string_view name;
    };

    struct sub_category_step
    {
        std::string_view name;
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
            top_category_step,
            sub_category_step,
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

        address& top(std::string_view name)
        {
            steps.push_back(top_category_step{ name });
            return *this;
        }

        address& sub(std::string_view name)
        {
            steps.push_back(sub_category_step{ name });
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
    // resolve errors
    // ------------------------------------------------------------

    enum class resolve_error_kind
    {
    // Missing context
        no_category_context,
        no_table_context,
        no_row_context,

    // Malformed address
        structure_after_value, 
        top_category_after_category,

    // Missing structure
        top_category_not_found,
        sub_category_not_found,
        key_not_found,
        table_not_found,
        row_not_owned,
        column_not_found,

    // Type error
        not_an_array,
        index_out_of_bounds,
        __LAST
    };

    constexpr std::array<std::string_view, static_cast<size_t>(resolve_error_kind::__LAST)> 
    resolve_error_string =
    {
        "no_category_context",
        "no_table_context",
        "no_row_context",
        "structure_after_value," 
        "top_category_after_category",
        "top_category_not_found",
        "sub_category_not_found",
        "key_not_found",
        "table_not_found",
        "row_not_owned",
        "column_not_found",
        "not_an_array",
        "index_out_of_bounds"
    };


    struct resolve_error
    {
        size_t             step_index;
        resolve_error_kind kind;
    };

    // ------------------------------------------------------------
    // resolve context
    // ------------------------------------------------------------

    struct resolve_context
    {
        const document* doc = nullptr;

        std::optional<document::category_view>  category;
        std::optional<document::table_view>     table;
        std::optional<document::table_row_view> row;
        std::optional<document::column_view>    column;
        const typed_value*                      value = nullptr;

        std::vector<resolve_error> errors;

        bool has_errors() const
        {
            return !errors.empty();
        }
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

    inline bool is_array(value_type v)
    {
        return v == value_type::string_array
            || v == value_type::int_array
            || v == value_type::float_array;
    }

    // ------------------------------------------------------------
    // resolve
    // ------------------------------------------------------------

    inline std::optional<const typed_value*>
    resolve(resolve_context& ctx, const address& addr)
    {
        ctx.errors.clear();
        ctx.value = nullptr;
        ctx.table.reset();
        ctx.row.reset();
        ctx.column.reset();

        ctx.doc = ctx.doc ? ctx.doc : nullptr;

        if (!ctx.doc)
            return std::nullopt;

        ctx.category = ctx.doc->root();

        if (addr.steps.empty())
            return std::nullopt;

        for (size_t i = 0; i < addr.steps.size(); ++i)
        {
            const auto& step = addr.steps[i];

            // ---------------- key
            if (auto s = std::get_if<key_step>(&step))
            {
                if (!ctx.category)
                {
                    ctx.errors.push_back({ i, resolve_error_kind::no_category_context });
                    return std::nullopt;
                }

                std::optional<document::key_view> k;

                if (std::holds_alternative<key_id>(s->id))
                    k = ctx.doc->key(std::get<key_id>(s->id));
                else
                    k = ctx.category->key(std::get<std::string_view>(s->id));

                if (!k)
                {
                    ctx.errors.push_back({ i, resolve_error_kind::key_not_found });
                    return std::nullopt;
                }

                ctx.value = &k->value();
                // do NOT touch category / table / row / column yet
            }

            // ---------------- top category
            else if (auto s = std::get_if<top_category_step>(&step))
            {
                // Note: category navigation is legal after key

                // top() is only legal before any category navigation
                if (ctx.category && ctx.category->id() != ctx.doc->root()->id())
                {
                    ctx.errors.push_back({ i, resolve_error_kind::top_category_after_category });
                    return std::nullopt;
                }

                auto next = ctx.doc->root()->child(s->name);
                if (!next)
                {
                    ctx.errors.push_back({ i, resolve_error_kind::top_category_not_found });
                    return std::nullopt;
                }

                ctx.category = next;
                ctx.table.reset();
                ctx.row.reset();
                ctx.column.reset();
                ctx.value = nullptr;
            }

            // ---------------- sub category
            else if (auto s = std::get_if<sub_category_step>(&step))
            {
                // Note: category navigation is legal after key

                if (!ctx.category || ctx.category->id() == ctx.doc->root()->id())
                {
                    ctx.errors.push_back({ i, resolve_error_kind::no_category_context });
                    return std::nullopt;
                }

                auto next = ctx.category->child(s->name);
                if (!next)
                {
                    ctx.errors.push_back({ i, resolve_error_kind::sub_category_not_found });
                    return std::nullopt;
                }

                ctx.category = next;
                ctx.table.reset();
                ctx.row.reset();
                ctx.column.reset();
                ctx.value = nullptr;
            }

            // ---------------- table
            else if (auto s = std::get_if<table_step>(&step))
            {
                if (ctx.value)
                {
                    ctx.errors.push_back({ i, resolve_error_kind::structure_after_value });
                    return std::nullopt;
                }

                if (!ctx.category)
                {
                    ctx.errors.push_back({ i, resolve_error_kind::no_category_context });
                    return std::nullopt;
                }

                std::optional<table_id> tid;

                if (std::holds_alternative<table_id>(s->id))
                    tid = std::get<table_id>(s->id);
                else
                    tid = resolve_table_ordinal(*ctx.category, std::get<size_t>(s->id));

                if (!tid)
                {
                    ctx.errors.push_back({ i, resolve_error_kind::table_not_found });
                    return std::nullopt;
                }

                ctx.table = ctx.doc->table(*tid);
                if (!ctx.table)
                {
                    ctx.errors.push_back({ i, resolve_error_kind::table_not_found });
                    return std::nullopt;
                }

                ctx.row.reset();
                ctx.column.reset();
                ctx.value = nullptr;
            }

            // ---------------- row
            else if (auto s = std::get_if<row_step>(&step))
            {
                if (ctx.value)
                {
                    ctx.errors.push_back({ i, resolve_error_kind::structure_after_value });
                    return std::nullopt;
                }

                if (!ctx.table)
                {
                    ctx.errors.push_back({ i, resolve_error_kind::no_table_context });
                    return std::nullopt;
                }

                ctx.row = ctx.doc->row(s->id);
                if (!ctx.row)
                {
                    ctx.errors.push_back({ i, resolve_error_kind::no_row_context });
                    return std::nullopt;
                }

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
                {
                    ctx.errors.push_back({ i, resolve_error_kind::row_not_owned });
                    return std::nullopt;
                }

                ctx.column.reset();
                ctx.value = nullptr;
            }

            // ---------------- column
            else if (auto s = std::get_if<column_step>(&step))
            {
                if (ctx.value)
                {
                    ctx.errors.push_back({ i, resolve_error_kind::structure_after_value });
                    return std::nullopt;
                }

                if (!ctx.table || !ctx.row)
                {
                    ctx.errors.push_back({ i, resolve_error_kind::no_row_context });
                    return std::nullopt;
                }

                std::optional<document::column_view> col;

                if (std::holds_alternative<column_id>(s->id))
                    col = ctx.doc->column(std::get<column_id>(s->id));
                else
                    col = ctx.table->column(std::get<std::string_view>(s->id));

                if (!col)
                {
                    ctx.errors.push_back({ i, resolve_error_kind::column_not_found });
                    return std::nullopt;
                }

                ctx.column = col;
                auto idx = col->index();
                ctx.value = &ctx.row->cells()[idx];
            }

            // ---------------- index
            else if (auto s = std::get_if<index_step>(&step))
            {
                if (!ctx.value)
                {
                    ctx.errors.push_back({ i, resolve_error_kind::not_an_array });
                    return std::nullopt;
                }

                if (!is_array(ctx.value->type))
                {
                    ctx.errors.push_back({ i, resolve_error_kind::not_an_array });
                    return std::nullopt;
                }

                auto& arr = std::get<std::vector<typed_value>>(ctx.value->val);

                if (s->index >= arr.size())
                {
                    ctx.errors.push_back({ i, resolve_error_kind::index_out_of_bounds });
                    return std::nullopt;
                }

                ctx.value = &arr[s->index];
            }
        }

        return ctx.value;
    }

    inline std::optional<const typed_value*>
    resolve_ex(resolve_context& ctx, const address& addr)
    {
        auto result = resolve(ctx, addr);
        return ctx.has_errors() ? std::nullopt : result;
    }

} // namespace arf::reflect

#endif // ARF_REFLECT_HPP
