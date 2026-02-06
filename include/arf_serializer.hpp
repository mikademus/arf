// arf_serializer.hpp - A Readable Format (Arf!) - Serializer
// Version 0.3.0
// Copyright 2025 Mikael Ueno A
// Licenced as-is under the MIT licence.

#ifndef ARF_SERIALIZER_HPP
#define ARF_SERIALIZER_HPP

#include "arf_document.hpp"
#include <ostream>
#include <cassert>

namespace arf
{

//========================================================================
// SERIALIZER_OPTIONS
//========================================================================

    struct serializer_options
    {
        enum class type_policy
        {
            preserve,        // Emit types as declared in source
            force_tacit,     // Strip all type annotations
            force_explicit   // Force all values to show types
        } types = type_policy::preserve;

        enum class blank_line_policy
        {
            preserve,   // Emit paragraph events as-is (may include empty lines)
            compact,    // Skip empty paragraph events
            readable    // Add strategic blank lines (after categories, tables)
        } blank_lines = blank_line_policy::preserve;

        bool emit_comments = true;      // If false, skip comment events
        bool emit_paragraphs = true;    // If false, skip paragraph events
    };

//========================================================================
// SERIALIZER
//========================================================================

    class serializer
    {
    public:
        serializer(const document& doc, serializer_options opts = {})
            : doc_(doc), opts_(opts)
        {
        }

        void write(std::ostream& out)
        {
            out_ = &out;

            // Walk root's ordered_items
            for (const auto& item : doc_.categories_.front().ordered_items)
                write_source_item(item);
        }

    private:
        const document&    doc_;
        std::ostream*      out_;        
        serializer_options opts_;
        size_t             indent_ {0};  // Current indentation level

    private:
        //----------------------------------------------------------------
        // Source item dispatcher
        //----------------------------------------------------------------

        void write_source_item(const document::source_item_ref& ref)
        {
std::cout << " - write_source_item\n";
            std::visit([&](auto&& id) { write_item(id); }, ref.id);
        }

        //----------------------------------------------------------------
        // Item writers (overloaded on ID type)
        //----------------------------------------------------------------

        void write_item(key_id id)
        {
std::cout << " - writing key\n";
            auto it = doc_.find_node_by_id(doc_.keys_, id);
            assert(it != doc_.keys_.end());
            write_key(*it);
        }

        void write_item(category_id id)
        {
std::cout << " - writing cat\n";
            auto it = doc_.find_node_by_id(doc_.categories_, id);
            assert(it != doc_.categories_.end());
            write_category_open(*it);
        }

        void write_item(const document::category_close_marker& marker)
        {
std::cout << " - writing cat close\n";
            write_category_close(marker);
        }

        void write_item(table_id id)
        {
std::cout << " - writing table\n";
            auto it = doc_.find_node_by_id(doc_.tables_, id);
            assert(it != doc_.tables_.end());
            write_table(*it);
        }

        void write_item(table_row_id id)
        {
std::cout << "  - writing row\n";
            auto it = doc_.find_node_by_id(doc_.rows_, id);
            assert(it != doc_.rows_.end());
            write_row(*it);
        }

        void write_item(comment_id id)
        {
std::cout << " - writing comment\n";
            if (!opts_.emit_comments)
                return;

            auto it = std::ranges::find_if(doc_.comments_,
                [id](auto const& c) { return c.id == id; });
            assert(it != doc_.comments_.end());

            write_comment(*it);
        }

        void write_item(paragraph_id id)
        {
std::cout << " - writing paragraph\n";
            if (!opts_.emit_paragraphs)
                return;

            auto it = std::ranges::find_if(doc_.paragraphs_,
                [id](auto const& p) { return p.id == id; });
            assert(it != doc_.paragraphs_.end());

            write_paragraph(*it);
        }

        //----------------------------------------------------------------
        // Key
        //----------------------------------------------------------------

        void write_key(const document::key_node& k)
        {                
            // Authored key with source available - emit original
            if (!k.is_edited && k.source_event_index && doc_.source_context_)
            {
                const auto& event = doc_.source_context_->document.events[*k.source_event_index];
                *out_ << event.text << '\n';
                return;
            }

            // Generated or edited key - reconstruct
            write_indent();
            *out_ << k.name;
            
            // Type annotation
            if (should_emit_type(k.value.type_source, k.value.type))
            {
                *out_ << ":" << type_string(k.value.type);
            }
            
            *out_ << " = ";
            write_value(k.value);
            *out_ << '\n';
        }

        //----------------------------------------------------------------
        // Category open
        //----------------------------------------------------------------

        void write_category_open(const document::category_node& cat)
        {
            bool is_root = (cat.id == category_id{0});

            if (is_root)
            {
                // Root category: just emit its contents (no header)
                write_category_contents(cat);
                return;
            }

            // Authored category with source - emit original open
            if (!cat.is_edited && cat.source_event_index_open && doc_.source_context_)
            {
                const auto& event = doc_.source_context_->document.events[*cat.source_event_index_open];
                *out_ << event.text << '\n';
                ++indent_;
                write_category_contents(cat);
                return;
            }

            // Generated or edited category - reconstruct
            bool is_top_level = (cat.parent == category_id{0});

            if (is_top_level)
            {
                write_indent();
                *out_ << cat.name << ":\n";
                ++indent_;
            }
            else
            {
                write_indent();
                *out_ << ":" << cat.name << '\n';
                ++indent_;
            }

            write_category_contents(cat);
        }

        void write_category_contents(const document::category_node& cat)
        {
            for (const auto& item : cat.ordered_items)
            {
                write_source_item(item);
            }
        }

        //----------------------------------------------------------------
        // Category close
        //----------------------------------------------------------------

        void write_category_close(const document::category_close_marker& marker)
        {
            --indent_;

            // Find the category being closed
            auto it = doc_.find_node_by_id(doc_.categories_, marker.which);
            assert(it != doc_.categories_.end());
            const auto& cat = *it;

            // Authored close with source - emit original
            if (!cat.is_edited && cat.source_event_index_close && doc_.source_context_)
            {
                const auto& event = doc_.source_context_->document.events[*cat.source_event_index_close];
                *out_ << event.text << '\n';
                return;
            }

            // Generated or edited - reconstruct
            write_indent();
            if (marker.form == document::category_close_form::shorthand)
            {
                *out_ << "/\n";
            }
            else
            {
                *out_ << "/" << cat.name << '\n';
            }
        }

        //----------------------------------------------------------------
        // Table
        //----------------------------------------------------------------

        void write_table(const document::table_node& tbl)
        {
            // Authored table with source - emit original header
            if (!tbl.is_edited && tbl.source_event_index && doc_.source_context_)
            {
                const auto& event = doc_.source_context_->document.events[*tbl.source_event_index];
                *out_ << event.text << '\n';
            }
            else
            {
                // Generated or edited table - reconstruct header
                write_indent();
                *out_ << "# ";

                bool first = true;
                for (auto col_id : tbl.columns)
                {
                    auto it = doc_.find_node_by_id(doc_.columns_, col_id);
                    assert(it != doc_.columns_.end());

                    if (!first)
                        *out_ << "  ";

                    *out_ << it->col.name;

                    if (should_emit_type(it->col.type_source, it->col.type))
                    {
                        *out_ << ":" << type_string(it->col.type);
                    }

                    first = false;
                }

                *out_ << '\n';
            }

            // Emit table contents (rows, comments, paragraphs, subcategories)
            for (const auto& item : tbl.ordered_items)
            {
                write_source_item(item);
            }
        }

        //----------------------------------------------------------------
        // Table row
        //----------------------------------------------------------------

        void write_row(const document::row_node& row)
        {
            // Authored row with source - emit original
            if (!row.is_edited && row.source_event_index && doc_.source_context_)
            {
                const auto& event = doc_.source_context_->document.events[*row.source_event_index];
                *out_ << event.text << '\n';
                return;
            }

            // Generated or edited row - reconstruct
            write_indent();

            bool first = true;
            for (const auto& cell : row.cells)
            {
                if (!first)
                    *out_ << "  ";
                
                write_value(cell);
                first = false;
            }

            *out_ << '\n';
        }

        //----------------------------------------------------------------
        // Comment
        //----------------------------------------------------------------

        void write_comment(const document::comment_node& c)
        {
            // Comments are always emitted verbatim from stored text
            // (They don't have edit tracking - if you change a comment, you change its .text)
            *out_ << c.text << '\n';
        }

        //----------------------------------------------------------------
        // Paragraph
        //----------------------------------------------------------------

        void write_paragraph(const document::paragraph_node& p)
        {
            if (opts_.blank_lines == serializer_options::blank_line_policy::compact
                && p.text.empty())
            {
                return;  // Skip empty paragraphs in compact mode
            }

            // Paragraphs are always emitted verbatim from stored text
            *out_ << p.text << '\n';
        }

        //----------------------------------------------------------------
        // Value emission
        //----------------------------------------------------------------

        void write_value(const typed_value& tv)
        {
            // For cells and keys, we'd need to know their parent node to check source
            // Since we don't have that context here, always reconstruct values
            // (Authored preservation happens at the node level: keys, rows)
            write_value_semantic(tv);
        }

        void write_value_semantic(const typed_value& tv)
        {
            switch (tv.type)
            {
                case value_type::string:
                    *out_ << std::get<std::string>(tv.val);
                    break;

                case value_type::integer:
                    *out_ << std::get<int64_t>(tv.val);
                    break;

                case value_type::decimal:
                    *out_ << std::get<double>(tv.val);
                    break;

                case value_type::boolean:
                    *out_ << (std::get<bool>(tv.val) ? "true" : "false");
                    break;

                case value_type::string_array:
                case value_type::int_array:
                case value_type::float_array:
                {
                    auto const& arr = std::get<std::vector<typed_value>>(tv.val);
                    bool first = true;
                    for (auto const& elem : arr)
                    {
                        if (!first)
                            *out_ << '|';
                        write_value(elem);
                        first = false;
                    }
                    break;
                }

                case value_type::unresolved:
                    // Empty cell or missing value
                    break;

                default:
                    *out_ << std::get<std::string>(tv.val);
                    break;
            }
        }

        //----------------------------------------------------------------
        // Type policy
        //----------------------------------------------------------------

        bool should_emit_type(type_ascription source, value_type type) const
        {
            switch (opts_.types)
            {
                case serializer_options::type_policy::force_tacit:
                    return false;

                case serializer_options::type_policy::force_explicit:
                    return type != value_type::unresolved;

                case serializer_options::type_policy::preserve:
                default:
                    return source == type_ascription::declared;
            }
        }

        std::string_view type_string(value_type type) const
        {
            switch (type)
            {
                case value_type::string:       return "str";
                case value_type::integer:      return "int";
                case value_type::decimal:      return "float";
                case value_type::boolean:      return "bool";
                case value_type::date:         return "date";
                case value_type::string_array: return "str[]";
                case value_type::int_array:    return "int[]";
                case value_type::float_array:  return "float[]";
                default:                       return "str";
            }
        }

        //----------------------------------------------------------------
        // Indentation
        //----------------------------------------------------------------

        void write_indent()
        {
            for (size_t i = 0; i < indent_; ++i)
                *out_ << "    ";  // 4 spaces per level
        }
    };

} // namespace arf

#endif // ARF_SERIALIZER_HPP