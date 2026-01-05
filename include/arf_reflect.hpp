// arf_reflect.hpp - A Readable Format (Arf!) - Reflection Interface
// Version 0.3.0
// Copyright 2025 Mikael Ueno A
// Licenced as-is under the MIT licence.

#ifndef ARF_REFLECT_HPP
#define ARF_REFLECT_HPP

#include "arf_document.hpp"
#include <span>
#include <optional>
#include <vector>
#include <cassert>

namespace arf
{
//========================================================================
// Forward declarations
//========================================================================
    
    class value_ref;
    class table_ref;
    class table_column_ref;
    class table_row_ref;
    class table_cell_ref;
    class table_partition_ref;

//========================================================================
// value_ref - Reflection over typed_value
//========================================================================
    
    class value_ref 
    {
    public:
        value_ref(const typed_value& v) : tv_(&v) {}

    // --- Provenance ---
        value_type      type() const { return tv_->type; }
        bool            declared() const { return tv_->type_source == type_ascription::declared; }
        type_ascription type_source() const { return tv_->type_source; }
        value_locus     origin_site() const { return tv_->origin; }
        
        bool is_valid() const { return tv_->semantic == semantic_state::valid; }
        bool is_contaminated() const { return tv_->contamination == contamination_state::contaminated; }

    // --- Raw access ---
        const value& raw() const { return tv_->val; }
        std::optional<std::string_view> source_str() const;
        
    // --- Format queries ---        
        bool is_scalar() const; 
        bool is_array() const;

        bool is_string() const { return std::holds_alternative<std::string>(tv_->val); }
        bool is_int() const { return std::holds_alternative<int64_t>(tv_->val); }
        bool is_float() const { return std::holds_alternative<double>(tv_->val); }
        bool is_bool() const { return std::holds_alternative<bool>(tv_->val); }
        
        bool is_string_array() const { return tv_->type == value_type::string_array; }
        bool is_int_array() const { return tv_->type == value_type::int_array; }
        bool is_float_array() const { return tv_->type == value_type::float_array; }
        
    // --- Scalar access (with conversion) ---
        std::optional<std::string> as_string() const;        
        std::optional<int64_t> as_int() const { return to_int(tv_->val); }
        std::optional<double> as_float() const { return to_float(tv_->val); }
        std::optional<bool> as_bool() const { return to_bool(tv_->val); }
        
    // --- Array element access ---
        size_t array_size() const;
        value_ref array_element(size_t index) const;

    private:
        const typed_value* tv_;

        std::optional<std::string> to_string(const value& v) const;
        std::optional<int64_t> to_int(const value& v) const;
        std::optional<double> to_float(const value& v) const;
        std::optional<bool> to_bool(const value& v) const;
    };

//========================================================================
// table_column_ref - Reflection over column
//========================================================================

    class table_column_ref
    {
    public:
        table_column_ref(const column* col, size_t index)
            : col_(col), index_(index) {}

        const std::string& name() const { return col_->name; }
        size_t index() const { return index_; }

        value_type type() const { return col_->type; }
        type_ascription type_source() const { return col_->type_source; }
        
        bool is_valid() const { return col_->semantic == semantic_state::valid; }

        std::optional<std::string_view> declared_type_literal() const
        {
            if (col_->declared_type)
                return *col_->declared_type;
            return std::nullopt;
        }

    private:
        const column* col_;
        size_t index_;
    };

//========================================================================
// table_cell_ref - Reflection over individual table cell
//========================================================================

    class table_cell_ref
    {
    public:
        table_cell_ref(const table_row_ref& row, const table_column_ref& col);

        size_t row_index() const;
        size_t column_index() const { return col_.index(); }

        value_ref value() const;

        const table_column_ref& column() const { return col_; }
        const table_row_ref& row() const { return *row_; }
             
    private:
        const table_row_ref* row_;
        table_column_ref col_;
    };

//========================================================================
// table_row_ref - Reflection over table_row_view
//========================================================================

    class table_row_ref
    {
    public:
        table_row_ref(const document* doc, table_row_id id);

        table_row_id id() const { return id_; }
        
        bool is_valid() const;
        bool is_contaminated() const;
        
        // Structure
        const document* document() const { return doc_; }
        document::category_view owning_category() const;
        
        size_t cell_count() const;
        value_ref cell_value(size_t col_index) const;
        table_cell_ref cell(const table_column_ref& col) const;
        
        // Iteration
        auto cells() const;

    private:
        const arf::document* doc_;
        table_row_id id_;
        
        const document::row_node* node() const;
        
        friend class table_cell_ref;
    };

//========================================================================
// table_partition_ref - Subcategory participation in table
//========================================================================

    class table_partition_ref
    {
    public:
        table_partition_ref(const table_ref* table, category_id cat_id);

        category_id id() const { return cat_id_; }
        std::string_view name() const;
        
        // Direct rows (declared in this category's scope)
        size_t direct_row_count() const;
        table_row_ref direct_row(size_t i) const;
        auto direct_rows() const;
        
        // All rows (including descendants)
        size_t row_count() const;
        table_row_ref row(size_t i) const;
        auto rows() const;
        
        // Hierarchy
        bool has_parent() const;
        table_partition_ref parent() const;
        
        size_t child_count() const;
        table_partition_ref child(size_t i) const;
        auto children() const;

    private:
        const table_ref* table_;
        category_id cat_id_;
        
        const document* doc() const;
        document::category_view category() const;
        
        friend class table_ref;
    };

//========================================================================
// table_ref - Reflection over table_view
//========================================================================

    class table_ref
    {
    public:
        table_ref(const document* doc, table_id id);
        
        table_id id() const { return id_; }
        
        bool is_valid() const;
        bool is_contaminated() const;
        
        // Schema
        size_t column_count() const;
        table_column_ref column(size_t i) const;
        std::optional<size_t> column_index(std::string_view name) const;
        auto columns() const;
        
        // Rows (document order)
        size_t row_count() const;
        table_row_ref row(size_t i) const;
        auto rows() const;
        
        // Partitions (subcategory participation)
        table_partition_ref root_partition() const;
        size_t partition_count() const;
        table_partition_ref partition(size_t i) const;
        auto partitions() const;
        
        // Access
        const document* document() const { return doc_; }
        document::category_view owning_category() const;

    private:
        const arf::document* doc_;
        table_id id_;
        
        const document::table_node* node() const;
        
        // Partition building
        struct partition_info
        {
            category_id cat_id;
            category_id parent_id;
            std::vector<category_id> children;
            std::vector<table_row_id> direct_rows;
            std::vector<table_row_id> all_rows;
        };
        
        mutable std::vector<partition_info> partitions_;
        mutable bool partitions_built_ = false;
        
        void build_partitions() const;
        size_t find_partition_index(category_id cat_id) const;
        
        friend class table_partition_ref;
        friend class table_row_ref;
    };

//========================================================================
// value_ref implementation
//========================================================================

    inline std::optional<std::string_view> value_ref::source_str() const
    {
        if (tv_->source_literal)
            return *tv_->source_literal;
        return std::nullopt;
    }
    
    inline bool value_ref::is_scalar() const 
    {
        return std::holds_alternative<std::string>(tv_->val) ||
               std::holds_alternative<int64_t>(tv_->val) ||
               std::holds_alternative<double>(tv_->val) ||
               std::holds_alternative<bool>(tv_->val);
    }
    
    inline bool value_ref::is_array() const 
    {
        return std::holds_alternative<std::vector<typed_value>>(tv_->val);
    }
    
    inline std::optional<std::string> value_ref::as_string() const
    {
        if (auto s = source_str())
            return std::string(*s);
        return to_string(tv_->val);
    }

    inline size_t value_ref::array_size() const
    {
        if (auto* arr = std::get_if<std::vector<typed_value>>(&tv_->val))
            return arr->size();
        return 0;
    }

    inline value_ref value_ref::array_element(size_t index) const
    {
        auto* arr = std::get_if<std::vector<typed_value>>(&tv_->val);
        assert(arr && index < arr->size());
        return value_ref((*arr)[index]);
    }

    inline std::optional<std::string> value_ref::to_string(const value& v) const
    {
        if (auto* str = std::get_if<std::string>(&v))
            return *str;
        if (auto* i = std::get_if<int64_t>(&v))
            return std::to_string(*i);
        if (auto* d = std::get_if<double>(&v))
            return std::to_string(*d);
        if (auto* b = std::get_if<bool>(&v))
            return *b ? "true" : "false";
        return std::nullopt;
    }

    inline std::optional<int64_t> value_ref::to_int(const value& v) const
    {
        if (auto* i = std::get_if<int64_t>(&v))
            return *i;
        if (auto* d = std::get_if<double>(&v))
            return static_cast<int64_t>(*d);
        if (auto* s = std::get_if<std::string>(&v))
        {
            try { return std::stoll(*s); }
            catch (...) {}
        }
        return std::nullopt;
    }

    inline std::optional<double> value_ref::to_float(const value& v) const
    {
        if (auto* d = std::get_if<double>(&v))
            return *d;
        if (auto* i = std::get_if<int64_t>(&v))
            return static_cast<double>(*i);
        if (auto* s = std::get_if<std::string>(&v))
        {
            try { return std::stod(*s); }
            catch (...) {}
        }
        return std::nullopt;
    }

    inline std::optional<bool> value_ref::to_bool(const value& v) const
    {
        if (auto* b = std::get_if<bool>(&v))
            return *b;
        if (auto* s = std::get_if<std::string>(&v))
        {
            std::string lower = detail::to_lower(*s);
            if (lower == "true" || lower == "yes" || lower == "1") return true;
            if (lower == "false" || lower == "no" || lower == "0") return false;
        }
        return std::nullopt;
    }

//========================================================================
// table_row_ref implementation
//========================================================================

    inline table_row_ref::table_row_ref(const document* doc, table_row_id id)
        : doc_(doc), id_(id)
    {
    }

    inline const document::row_node* table_row_ref::node() const
    {
        auto row_view = doc_->row(id_);
        assert(row_view.has_value());
        return row_view->node;
    }

    inline bool table_row_ref::is_valid() const
    {
        return node()->semantic == semantic_state::valid;
    }

    inline bool table_row_ref::is_contaminated() const
    {
        return node()->contamination == contamination_state::contaminated;
    }

    inline document::category_view table_row_ref::owning_category() const
    {
        return doc_->row(id_)->owner();
    }

    inline size_t table_row_ref::cell_count() const
    {
        return node()->cells.size();
    }

    inline value_ref table_row_ref::cell_value(size_t col_index) const
    {
        const auto& cells = node()->cells;
        assert(col_index < cells.size());
        return value_ref(cells[col_index]);
    }

    inline table_cell_ref table_row_ref::cell(const table_column_ref& col) const
    {
        return table_cell_ref(*this, col);
    }

    inline auto table_row_ref::cells() const
    {
        struct cell_range
        {
            const table_row_ref* row;
            
            struct iterator
            {
                const table_row_ref* row;
                size_t index;
                
                value_ref operator*() const { return row->cell_value(index); }
                iterator& operator++() { ++index; return *this; }
                bool operator!=(const iterator& other) const { return index != other.index; }
            };
            
            iterator begin() const { return {row, 0}; }
            iterator end() const { return {row, row->cell_count()}; }
        };
        
        return cell_range{this};
    }

//========================================================================
// table_cell_ref implementation
//========================================================================

    inline table_cell_ref::table_cell_ref(const table_row_ref& row, const table_column_ref& col)
        : row_(&row), col_(col)
    {
    }

    inline size_t table_cell_ref::row_index() const
    {
        return row_->id().val;
    }

    inline value_ref table_cell_ref::value() const
    {
        return row_->cell_value(col_.index());
    }

//========================================================================
// table_ref implementation
//========================================================================

    inline table_ref::table_ref(const document* doc, table_id id)
        : doc_(doc), id_(id)
    {
    }

    inline const document::table_node* table_ref::node() const
    {
        auto table_view = doc_->table(id_);
        assert(table_view.has_value());
        return table_view->node;
    }

    inline bool table_ref::is_valid() const
    {
        return node()->semantic == semantic_state::valid;
    }

    inline bool table_ref::is_contaminated() const
    {
        return node()->contamination == contamination_state::contaminated;
    }

    inline document::category_view table_ref::owning_category() const
    {
        return doc_->table(id_)->owner();
    }

    inline size_t table_ref::column_count() const
    {
        return node()->columns.size();
    }

    inline table_column_ref table_ref::column(size_t i) const
    {
        const auto& cols = node()->columns;
        assert(i < cols.size());
        return table_column_ref(&cols[i], i);
    }

    inline std::optional<size_t> table_ref::column_index(std::string_view name) const
    {
        const auto& cols = node()->columns;
        for (size_t i = 0; i < cols.size(); ++i)
        {
            if (cols[i].name == name)
                return i;
        }
        return std::nullopt;
    }

    inline auto table_ref::columns() const
    {
        struct column_range
        {
            const table_ref* table;
            
            struct iterator
            {
                const table_ref* table;
                size_t index;
                
                table_column_ref operator*() const { return table->column(index); }
                iterator& operator++() { ++index; return *this; }
                bool operator!=(const iterator& other) const { return index != other.index; }
            };
            
            iterator begin() const { return {table, 0}; }
            iterator end() const { return {table, table->column_count()}; }
        };
        
        return column_range{this};
    }

    inline size_t table_ref::row_count() const
    {
        return node()->rows.size();
    }

    inline table_row_ref table_ref::row(size_t i) const
    {
        const auto& row_ids = node()->rows;
        assert(i < row_ids.size());
        return table_row_ref(doc_, row_ids[i]);
    }

    inline auto table_ref::rows() const
    {
        struct row_range
        {
            const table_ref* table;
            
            struct iterator
            {
                const table_ref* table;
                size_t index;
                
                table_row_ref operator*() const { return table->row(index); }
                iterator& operator++() { ++index; return *this; }
                bool operator!=(const iterator& other) const { return index != other.index; }
            };
            
            iterator begin() const { return {table, 0}; }
            iterator end() const { return {table, table->row_count()}; }
        };
        
        return row_range{this};
    }

    inline void table_ref::build_partitions() const
    {
        if (partitions_built_) return;
        
        partitions_.clear();
        
        // Root partition
        auto owner = owning_category();
        partitions_.push_back({
            owner.id(),
            invalid_id<category_tag>(),
            {},
            {},
            {}
        });
        
        // Collect all participating categories
        std::vector<category_id> stack;
        stack.push_back(owner.id());
        
        auto process_category = [&](auto& self, category_id cat_id) -> void
        {
            size_t part_idx = find_partition_index(cat_id);
            if (part_idx == npos())
            {
                category_id parent_id = stack.empty() ? invalid_id<category_tag>() : stack.back();
                
                partitions_.push_back({
                    cat_id,
                    parent_id,
                    {},
                    {},
                    {}
                });
                part_idx = partitions_.size() - 1;
                
                if (parent_id != invalid_id<category_tag>())
                {
                    size_t parent_idx = find_partition_index(parent_id);
                    if (parent_idx != npos())
                        partitions_[parent_idx].children.push_back(cat_id);
                }
            }
            
            stack.push_back(cat_id);
            
            auto cat_view = doc_->category(cat_id);
            if (cat_view)
            {
                for (auto child_id : cat_view->children())
                    self(self, child_id);
            }
            
            stack.pop_back();
        };
        
        for (auto child_id : owner.children())
            process_category(process_category, child_id);
        
        // Assign rows to partitions
        for (auto row_id : node()->rows)
        {
            auto row_view = doc_->row(row_id);
            if (!row_view) continue;
            
            category_id row_owner = row_view->owner().id();
            size_t part_idx = find_partition_index(row_owner);
            
            if (part_idx != npos())
                partitions_[part_idx].direct_rows.push_back(row_id);
        }
        
        // Build all_rows bottom-up
        for (size_t i = partitions_.size(); i-- > 0; )
        {
            auto& part = partitions_[i];
            part.all_rows = part.direct_rows;
            
            for (auto child_id : part.children)
            {
                size_t child_idx = find_partition_index(child_id);
                if (child_idx != npos())
                {
                    const auto& child_rows = partitions_[child_idx].all_rows;
                    part.all_rows.insert(part.all_rows.end(), 
                                        child_rows.begin(), 
                                        child_rows.end());
                }
            }
        }
        
        partitions_built_ = true;
    }

    inline size_t table_ref::find_partition_index(category_id cat_id) const
    {
        for (size_t i = 0; i < partitions_.size(); ++i)
        {
            if (partitions_[i].cat_id == cat_id)
                return i;
        }
        return npos();
    }

    inline table_partition_ref table_ref::root_partition() const
    {
        build_partitions();
        return table_partition_ref(this, owning_category().id());
    }

    inline size_t table_ref::partition_count() const
    {
        build_partitions();
        return partitions_.size();
    }

    inline table_partition_ref table_ref::partition(size_t i) const
    {
        build_partitions();
        assert(i < partitions_.size());
        return table_partition_ref(this, partitions_[i].cat_id);
    }

    inline auto table_ref::partitions() const
    {
        struct partition_range
        {
            const table_ref* table;
            
            struct iterator
            {
                const table_ref* table;
                size_t index;
                
                table_partition_ref operator*() const { return table->partition(index); }
                iterator& operator++() { ++index; return *this; }
                bool operator!=(const iterator& other) const { return index != other.index; }
            };
            
            iterator begin() const { return {table, 0}; }
            iterator end() const { return {table, table->partition_count()}; }
        };
        
        return partition_range{this};
    }

//========================================================================
// table_partition_ref implementation
//========================================================================

    inline table_partition_ref::table_partition_ref(const table_ref* table, category_id cat_id)
        : table_(table), cat_id_(cat_id)
    {
        table_->build_partitions();
    }

    inline const document* table_partition_ref::doc() const
    {
        return table_->document();
    }

    inline document::category_view table_partition_ref::category() const
    {
        auto cat = doc()->category(cat_id_);
        assert(cat.has_value());
        return *cat;
    }

    inline std::string_view table_partition_ref::name() const
    {
        return category().name();
    }

    inline size_t table_partition_ref::direct_row_count() const
    {
        size_t idx = table_->find_partition_index(cat_id_);
        assert(idx != npos());
        return table_->partitions_[idx].direct_rows.size();
    }

    inline table_row_ref table_partition_ref::direct_row(size_t i) const
    {
        size_t idx = table_->find_partition_index(cat_id_);
        assert(idx != npos());
        const auto& rows = table_->partitions_[idx].direct_rows;
        assert(i < rows.size());
        return table_row_ref(doc(), rows[i]);
    }

    inline auto table_partition_ref::direct_rows() const
    {
        struct row_range
        {
            const table_partition_ref* part;
            
            struct iterator
            {
                const table_partition_ref* part;
                size_t index;
                
                table_row_ref operator*() const { return part->direct_row(index); }
                iterator& operator++() { ++index; return *this; }
                bool operator!=(const iterator& other) const { return index != other.index; }
            };
            
            iterator begin() const { return {part, 0}; }
            iterator end() const { return {part, part->direct_row_count()}; }
        };
        
        return row_range{this};
    }

    inline size_t table_partition_ref::row_count() const
    {
        size_t idx = table_->find_partition_index(cat_id_);
        assert(idx != npos());
        return table_->partitions_[idx].all_rows.size();
    }

    inline table_row_ref table_partition_ref::row(size_t i) const
    {
        size_t idx = table_->find_partition_index(cat_id_);
        assert(idx != npos());
        const auto& rows = table_->partitions_[idx].all_rows;
        assert(i < rows.size());
        return table_row_ref(doc(), rows[i]);
    }

    inline auto table_partition_ref::rows() const
    {
        struct row_range
        {
            const table_partition_ref* part;
            
            struct iterator
            {
                const table_partition_ref* part;
                size_t index;
                
                table_row_ref operator*() const { return part->row(index); }
                iterator& operator++() { ++index; return *this; }
                bool operator!=(const iterator& other) const { return index != other.index; }
            };
            
            iterator begin() const { return {part, 0}; }
            iterator end() const { return {part, part->row_count()}; }
        };
        
        return row_range{this};
    }

    inline bool table_partition_ref::has_parent() const
    {
        size_t idx = table_->find_partition_index(cat_id_);
        assert(idx != npos());
        return table_->partitions_[idx].parent_id != invalid_id<category_tag>();
    }

    inline table_partition_ref table_partition_ref::parent() const
    {
        size_t idx = table_->find_partition_index(cat_id_);
        assert(idx != npos());
        category_id parent_id = table_->partitions_[idx].parent_id;
        assert(parent_id != invalid_id<category_tag>());
        return table_partition_ref(table_, parent_id);
    }

    inline size_t table_partition_ref::child_count() const
    {
        size_t idx = table_->find_partition_index(cat_id_);
        assert(idx != npos());
        return table_->partitions_[idx].children.size();
    }

    inline table_partition_ref table_partition_ref::child(size_t i) const
    {
        size_t idx = table_->find_partition_index(cat_id_);
        assert(idx != npos());
        const auto& children = table_->partitions_[idx].children;
        assert(i < children.size());
        return table_partition_ref(table_, children[i]);
    }

    inline auto table_partition_ref::children() const
    {
        struct child_range
        {
            const table_partition_ref* part;
            
            struct iterator
            {
                const table_partition_ref* part;
                size_t index;
                
                table_partition_ref operator*() const { return part->child(index); }
                iterator& operator++() { ++index; return *this; }
                bool operator!=(const iterator& other) const { return index != other.index; }
            };
            
            iterator begin() const { return {part, 0}; }
            iterator end() const { return {part, part->child_count()}; }
        };
        
        return child_range{this};
    }

} // namespace arf

#endif // ARF_REFLECT_HPP