// arf_editor.hpp - A Readable Format (Arf!) - Document Editor
// Version 0.3.0
// Copyright 2025 Mikael Ueno A
// Licenced as-is under the MIT licence.

#ifndef ARF_EDITOR_HPP
#define ARF_EDITOR_HPP

#include "arf_document.hpp"

namespace arf
{
    // This creates an INCOMPLETE typed_value and is only intended to 
    // be used for parameters in editor methods where the editor will
    // update the value_locus and creation_state members.
namespace ed
{
    template<typename T>
    inline typed_value make_typed_value( T value )
    {
        return typed_value{
            .val  = typename vt_conv<T>::stype(value),
            .type = static_cast<value_type>(vt_conv<T>::vtype),
            .type_source = type_ascription::tacit,
            // .origin = orig,
            .semantic = semantic_state::valid,
            .contamination = contamination_state::clean,
            // .creation = cs,
            .is_edited = false
        };
    }    
}

    class editor
    {
    public:
        explicit editor(document& doc) noexcept
            : doc_(doc)
        {}

    //============================================================
    // Key / value manipulation
    //============================================================

        key_id append_key(
            category_id where,
            std::string_view name,
            value v,
            bool untyped = false
        );

        template<typename K>
        key_id insert_key_before(
            id<K> anchor,
            std::string_view name,
            value v,
            bool untyped = false
        );

        template<typename K>
        key_id insert_key_after(
            id<K> anchor,
            std::string_view name,
            value v,
            bool untyped = false
        );

        void set_key_value(
            key_id key,
            value val
        );

        bool erase_key(key_id id);

    //============================================================
    // Comments
    //============================================================

        comment_id append_comment(
            category_id where,
            std::string_view text
        );

        template<typename K>
        comment_id insert_comment_before(
            id<K> anchor,
            std::string_view text
        );

        template<typename K>
        comment_id insert_comment_after(
            id<K> anchor,
            std::string_view text
        );

        void set_comment(
            comment_id id,
            std::string_view text
        );

        bool erase_comment(comment_id id);

    //============================================================
    // Paragraphs (category scope only)
    //============================================================

        paragraph_id append_paragraph(
            category_id where,
            std::string_view text
        );

        template<typename K>
        paragraph_id insert_paragraph_before(
            id<K> anchor,
            std::string_view text
        );

        template<typename K>
        paragraph_id insert_paragraph_after(
            id<K> anchor,
            std::string_view text
        );

        void set_paragraph(
            paragraph_id id,
            std::string_view text
        );

        bool erase_paragraph(paragraph_id id);

    //============================================================
    // Tables
    //============================================================

        table_id append_table(
            category_id where,
            std::vector<std::string> column_names
        );

        bool erase_table(table_id id);

        // Rows are table-scoped only
        table_row_id append_row(
            table_id table,
            std::vector<value> cells
        );

        bool erase_row(table_row_id id);

        void set_cell_value(
            table_row_id row,
            column_id col,
            value val
        );

    //============================================================
    // Array element manipulation
    //============================================================

    // Key/value specialisation
    //-------------------------------
        void append_array_element
        (
            key_id key,
            value val
        );

        void set_array_element
        (
            key_id key,
            size_t index,
            value val
        );

        void delete_array_element
        (
            key_id key,
            size_t index
        );

    // Table specialisation
    //-------------------------------
        void append_array_element
        (
            table_row_id row,
            column_id col,
            value val
        );

        void set_array_element
        (
            table_row_id row,
            column_id col,
            size_t index,
            value val
        );

        void delete_array_element
        (
            table_row_id row,
            column_id col,
            size_t index
        );

    //============================================================
    // Type control (explicit, opt-in)
    //============================================================

        bool set_key_type(
            key_id id,
            value_type type,
            type_ascription ascription = type_ascription::declared
        );

        bool set_column_type(
            column_id id,
            value_type type,
            type_ascription ascription = type_ascription::declared
        );

        // Provides access to the document's internal container
        // node corresponding to an entity ID. This is very 
        // power-user territory and should be avoided in 
        // general use. 
        template<typename Tag>
        typename document::to_node_type<Tag>::type* 
        _access_internal_document_container( id<Tag> id_ )
        {
            return doc_.get_node(id_);
        }

    private:
        document& doc_;

    private:
        //========================================================
        // Internal helpers — NEVER exposed
        //========================================================

        document::source_item_ref make_ref(key_id id)        noexcept;
        document::source_item_ref make_ref(comment_id id)    noexcept;
        document::source_item_ref make_ref(paragraph_id id)  noexcept;
        document::source_item_ref make_ref(table_id id)      noexcept;
        document::source_item_ref make_ref(table_row_id id)  noexcept;

        template<typename T>
        typename document::to_node_type<T>& get_node( id<T> id ) const noexcept;

        template<typename K>
        document::source_item_ref const* locate_anchor(id<K> anchor) const noexcept;

        bool scope_allows_paragraph(category_id where) const noexcept;
        bool scope_allows_key(category_id where) const noexcept;
    };

    void editor::set_key_value( key_id key, value val )
    {
        auto* kn = doc_.get_node(key);

        if (!kn)
            return;
        
        auto& tv = kn->value;

        // Replace payload
        tv.val = std::move(val);

        // Provenance stays key-bound
        tv.origin = value_locus::key_value;

        // Mark as edited
        tv.is_edited = true;

        // Re-evaluate semantic validity only if typed
        if (tv.type != value_type::unresolved)
        {
            if (tv.held_type() != tv.type)
                tv.semantic = semantic_state::invalid;
            else
                tv.semantic = semantic_state::valid;
        }

        // Mark structural node as edited (cheap aggregation flag)
        kn->is_edited = true;
    }


}

#endif