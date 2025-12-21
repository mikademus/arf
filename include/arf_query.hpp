// arf_query.hpp - A Readable Format (Arf!) - Query Interface
// Version 0.2.0

#ifndef ARF_QUERY_HPP
#define ARF_QUERY_HPP

#include "arf_core.hpp"

#include <iterator>
#include <span>
#include <cassert>

namespace arf 
{
    //========================================================================
    // REFLECTION BASE
    //========================================================================
    
    class value_ref 
    {
    public:
        value_ref(const typed_value& v) : tv_(&v) {}

    // --- provenance ---
        value_type      type()          const { return tv_->type; }
        bool            declared()      const { return tv_->type_source == type_ascription::declared; }
        type_ascription type_source()   const { return tv_->type_source; }
        value_locus     origin_site()   const { return tv_->origin_site; }

    // --- raw access ---
        const value& raw() const { return tv_->val; }

        std::optional<std::string_view> source_str() const
        {
            if (!tv_->source_literal)
                return std::nullopt;

            return std::string_view(*tv_->source_literal);
        }        
        
    // --- format ---        
        bool is_scalar() const 
        {
            return std::holds_alternative<std::string>(tv_->val) ||
                   std::holds_alternative<int64_t>(tv_->val) ||
                   std::holds_alternative<double>(tv_->val) ||
                   std::holds_alternative<bool>(tv_->val);
        }
        
        bool is_array() const 
        {
            return std::holds_alternative<std::vector<std::string>>(tv_->val) ||
                   std::holds_alternative<std::vector<int64_t>>(tv_->val) ||
                   std::holds_alternative<std::vector<double>>(tv_->val);
        }
        
        bool is_string() const { return std::holds_alternative<std::string>(tv_->val); }
        bool is_int() const { return std::holds_alternative<int64_t>(tv_->val); }
        bool is_float() const { return std::holds_alternative<double>(tv_->val); }
        bool is_bool() const { return std::holds_alternative<bool>(tv_->val); }
        
        bool is_string_array() const { return std::holds_alternative<std::vector<std::string>>(tv_->val); }
        bool is_int_array() const { return std::holds_alternative<std::vector<int64_t>>(tv_->val); }
        bool is_float_array() const { return std::holds_alternative<std::vector<double>>(tv_->val); }
        
    // --- scalar access: allow conversions ---
        std::optional<std::string> as_string() const
        {
            if (auto s = source_str())
                return std::string(*s);

            return to_string(tv_->val);
        }
        
        std::optional<int64_t> as_int()   const { return to_int(tv_->val); }
        std::optional<double>  as_float() const { return to_float(tv_->val); }
        std::optional<bool>    as_bool()  const { return to_bool(tv_->val); }
        
    // --- array access: non-converting views ---
        std::optional<std::span<const std::string>> string_array() const { return array_view<std::string>(); }
        std::optional<std::span<const int64_t>>     int_array()    const { return array_view<int64_t>(); }
        std::optional<std::span<const double>>      float_array()  const { return array_view<double>(); }


    private:
        const typed_value* tv_;

        template<typename T>
        std::optional<std::span<const T>> array_view() const
        {
            if (auto* arr = std::get_if<std::vector<T>>(&tv_->val))
                return std::span<const T>(*arr);
            return std::nullopt;
        }

        static std::optional<std::string> to_string(const value& v)
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

        static std::optional<int64_t> to_int(const value& v)
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

        static std::optional<double> to_float(const value& v)
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

        static std::optional<bool> to_bool(const value& v)
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
    };
    
    //========================================================================
    // TABLE ROW VIEW
    //========================================================================
    
    class table_view;
    
    class row_view 
    {
    public:
        row_view(const table_row* row, const table_view* table, const category* source)
            : row_(row), table_(table), source_category_(source) {}
        
        // Index-based access
        value_ref operator[](size_t index) const { return value_ref((*row_)[index]); }
        
        // Name-based value access
        const typed_value* get_typed_ptr(const std::string& name) const;

        //const value* get_ptr(const std::string& column_name) const;

        std::optional<value_ref> get(const std::string& name) const;
        
        // Convenience typed getters
        std::optional<std::string> get_string(const std::string& col) const;
        std::optional<int64_t> get_int(const std::string& col) const;
        std::optional<double> get_float(const std::string& col) const;
        std::optional<bool> get_bool(const std::string& col) const;
        
        // Array getters
        std::optional<std::span<const std::string>> get_string_array(const std::string& col) const;
        std::optional<std::span<const int64_t>> get_int_array(const std::string& col) const;
        std::optional<std::span<const double>> get_float_array(const std::string& col) const;
        
        // Provenance information
        const category* source() const { return source_category_; }
        std::string source_name() const { return source_category_->name; }
        bool is_base_row() const;
        
        // Raw access
        const table_row& raw() const { return *row_; }
        
    private:
    
        template<typename T>
        const T* get_ptr(const std::string& column_name) const;

        const table_row* row_;
        const table_view* table_;
        const category* source_category_;
    };
    
    //========================================================================
    // TABLE VIEW
    //========================================================================
    
    class table_view 
    {
    public:
            // Iterator for document-order row traversal
        class document_iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = row_view;

            document_iterator(const table_view* table, bool at_end = false);

            row_view operator*() const;
            document_iterator& operator++();
            bool operator==(const document_iterator& other) const;

        private:
            void advance();

            const table_view* table_;
            bool at_end_;

            struct frame
            {
                const category* cat;
                size_t decl_index;   // Position in source_order
                const decl_ref* current_row = nullptr;
            };

            std::vector<frame> stack_;
        };

        // Range wrapper for rows_document()
        class document_range
        {
        public:
            document_range(const table_view* table) : table_(table) {}
            document_iterator begin() const { return table_->rows_document_begin(); }
            document_iterator end() const { return table_->rows_document_end(); }
        private:
            const table_view* table_;
        };

        // Iterator for recursive row traversal
        class recursive_iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = row_view;

            recursive_iterator(const table_view* table, bool at_end = false);

            row_view operator*() const;
            recursive_iterator& operator++();
            bool operator==(const recursive_iterator& other) const;

        private:
            void advance();

            const table_view* table_;
            bool at_end_;

            struct frame
            {
                const category* cat;
                size_t row_index;
                std::map<std::string, std::unique_ptr<category>>::const_iterator child_it;
            };

            std::vector<frame> stack_;
        };
        
        // Range wrapper for rows_recursive()
        class recursive_range 
        {
        public:
            recursive_range(const table_view* table) : table_(table) {}
            recursive_iterator begin() const { return table_->rows_recursive_begin(); }
            recursive_iterator end() const { return table_->rows_recursive_end(); }
        private:
            const table_view* table_;
        };
        
        explicit table_view(const category* cat) : cat_(cat) {}
        
        // Column information
        const std::vector<column>& columns() const { return cat_->table_columns; }
        std::optional<size_t> column_index(const std::string& name) const;
        
        // Row access (current category only)
        const std::vector<table_row>& rows() const { return cat_->table_rows; }
        row_view row(size_t index) const;
        
        // Recursive row iteration (includes subcategories, depth-first)
        recursive_iterator rows_recursive_begin() const { return recursive_iterator(this, false); }
        recursive_iterator rows_recursive_end() const { return recursive_iterator(this, true); }
        recursive_range rows_recursive() const { return recursive_range(this); }
        
        // Document-order row iteration (preserves author's ordering)
        document_iterator rows_document_begin() const { return document_iterator(this, false); }
        document_iterator rows_document_end() const { return document_iterator(this, true); }
        document_range rows_document() const { return document_range(this); }

        // Subcategory access
        const std::map<std::string, std::unique_ptr<category>>& subcategories() const 
        { 
            return cat_->subcategories; 
        }
        
        std::optional<table_view> subcategory(const std::string& name) const;
        
        // Raw category access
        const category* raw() const { return cat_; }
        
    private:
        const category* cat_;
        
        friend class recursive_iterator;
        friend class row_view;
    };
    
    //========================================================================
    // REFLECTION CONT.
    //========================================================================

    class column_ref
    {
    public:
        column_ref(const column& col, size_t index)
            : col_(&col), index_(index) {}

        const std::string& name() const { return col_->name; }
        size_t             index() const { return index_; }

        value_type      type()        const { return col_->type; }
        type_ascription type_source() const { return col_->type_source; }

    private:
        const column* col_;
        size_t        index_;
    };

    class row_ref;
    class table_ref;

    class cell_ref
    {
    public:
        cell_ref(const row_ref& row, const column_ref& col)
            : row_(&row)
            , col_(&col)
        {
        }

        size_t row_index()    const; 
        size_t column_index() const;

        value_ref  value()  const;

        const column_ref& column() const { return *col_; }
        const row_ref&    row()    const { return *row_; }   
             
    private:
        const row_ref* row_;
        const column_ref* col_;
    };

    class subcategory_ref;

    class row_ref
    {
    public:
        row_ref(const table_ref& table, size_t row_index)
            : table_(&table), row_index_(row_index) {}

        // --- identity ---
        size_t index() const { return row_index_; }
        const table_ref& table() const { return *table_; }

        // --- structure ---
        subcategory_ref subcategory() const;

        size_t cell_count() const;
        auto cells() const;
        cell_ref cell(const column_ref& col) const;

        // --- compatibility layer ---
        const typed_value* get_typed_ptr(const std::string& name) const;
        const value*       get_ptr(const std::string& name) const;

    private:
        const table_ref* table_;
        size_t           row_index_; // ALWAYS table-space index
    };

    class subcategory_ref
    {
    public:
        subcategory_ref(const table_ref& table, size_t subcat_index);

        const std::string& name() const;

        // number of rows in this subcategory (all descendants included)
        size_t row_count() const;

        // global table row index
        size_t rows_in_table(size_t i) const;

        // index relative to parent subcategory
        size_t rows_in_parent(size_t i) const;

        // hierarchy
        bool has_parent() const;
        subcategory_ref parent() const;

        size_t child_count() const;
        subcategory_ref child(size_t i) const;

        // iteration
        auto rows() const;

    private:
        const table_ref* table_;
        size_t           subcat_index_;
    };  

    class table_ref
    {
    public:
        table_ref(const category& cat)
            : category_(&cat) {}

        const category& schema() const { return *category_; }
                    
        // columns
        size_t      column_count() const;
        column_ref  column(size_t i) const;
        auto        columns() const;
        //size_t      column_index(std::string_view name) const { return category_->column_index(name); }
        
        // rows (document order)
        size_t      row_count() const { return category_->table_rows.size(); }
        row_ref     row(size_t table_row_index) const { return row_ref(*this, table_row_index); }
        auto        rows() const;

        // subcategories
        subcategory_ref root_subcategory() const;
        size_t          subcategory_count() const;
        subcategory_ref subcategory(size_t i) const;
        auto            subcategories() const;

        // resolves the active subcategory for a table row
        // NOTE: parameter is ALWAYS a table row index
        size_t resolve_subcategory_index(size_t table_row_index) const;

    private:
        const category* category_;
    };

    //========================================================================
    // QUERY API
    //========================================================================
    
    // Basic value query (returns proxy for reflection)
    std::optional<value_ref> get(const document& doc, const std::string& path);
    
    // Direct value access
    std::optional<std::string> get_string(const document& doc, const std::string& path);
    std::optional<int64_t> get_int(const document& doc, const std::string& path);
    std::optional<double> get_float(const document& doc, const std::string& path);
    std::optional<bool> get_bool(const document& doc, const std::string& path);

    // Array access helpers
    std::optional<std::span<const std::string>> get_string_array(const document& doc, const std::string& path);        
    std::optional<std::span<const int64_t>> get_int_array(const document& doc, const std::string& path);        
    std::optional<std::span<const double>> get_float_array(const document& doc, const std::string& path);
    
    // Table access
    std::optional<table_view> get_table(const document& doc, const std::string& path);
    
    // Path utilities
    std::string category_path(const category* cat, const document& doc);
    std::string to_path(const row_view& row, const document& doc);
    
    //========================================================================
    // QUERY IMPLEMENTATION
    //========================================================================
    
    namespace detail 
    {
        enum class path_resolution
        {
            key_owner,   // stop before last segment
            category     // consume full path
        };

        inline std::vector<std::string> split_path(const std::string& path)
        {
            std::vector<std::string> parts;
            std::string current;
            
            for (char c : path)
            {
                if (c == '.')
                {
                    if (!current.empty())
                    {
                        parts.push_back(to_lower(current));
                        current.clear();
                    }
                }
                else
                {
                    current += c;
                }
            }
            
            if (!current.empty())
                parts.push_back(to_lower(current));
            
            return parts;
        }
        

        inline const category* resolve_category(
            const document& doc,
            const std::vector<std::string>& path,
            path_resolution target
        )
        {
            if (path.empty()) return nullptr;

            auto it = doc.categories.find(path[0]);
            if (it == doc.categories.end())
            {
                it = doc.categories.find(std::string(ROOT_CATEGORY_NAME));
                if (it == doc.categories.end())
                    return nullptr;
            }

            const category* current = it->second.get();

            const size_t limit =
                (target == path_resolution::key_owner)
                    ? path.size() - 1
                    : path.size();

            for (size_t i = 1; i < limit; ++i)
            {
                auto sub = current->subcategories.find(path[i]);
                if (sub == current->subcategories.end())
                    return nullptr;

                current = sub->second.get();
            }

            return current;
        }

        inline const typed_value* find_typed_value_ptr(const document& doc, const std::string& path)
        {
            auto parts = split_path(path);
            if (parts.empty()) return nullptr;

            const category* cat =
                resolve_category(doc, parts, path_resolution::key_owner);
            if (!cat) return nullptr;

            auto it = cat->key_values.find(parts.back());
            if (it == cat->key_values.end())
                return nullptr;

            return &it->second;
        }
    } // namespace detail

    //========================================================================
    // REFLECTION API IMPLEMENTATION
    //========================================================================

    size_t cell_ref::row_index() const 
    { 
        return row_->index(); 
    }

    size_t cell_ref::column_index() const
    { 
        return col_->index(); 
    }


    inline auto row_ref::cells() const
    {
        struct cell_range
        {
            const row_ref* row;
            struct iterator
            {
                const row_ref* row;
                size_t col;

                cell_ref operator*() const
                {
                    auto cr = row->table().column(col);
                    return row->cell(cr);
                }

                iterator& operator++()
                {
                    ++col;
                    return *this;
                }

                bool operator!=(const iterator& other) const
                {
                    return col != other.col;
                }
            };

            iterator begin() const { return { row, 0 }; }
            iterator end()   const { return { row, row->cell_count() }; }
        };

        return cell_range{ this };
    }

    // inline row_ref    cell_ref::row()    const { return row_ref(*category_, row_index()); }
    // inline column_ref cell_ref::column() const { return column_ref(category_->table_columns[column_index_], column_index_); }

    inline value_ref cell_ref::value() const
    {
        const typed_value& tv =
            row_->table().schema().table_rows[row_->index()][col_->index()];
            //category_->table_rows[row_index_][column_index_];
        return value_ref(tv);
    }

    inline size_t row_ref::cell_count() const 
    { 
        return table_->column_count(); 
    }

    inline cell_ref row_ref::cell(const column_ref& col) const
    // cell_ref row_ref::cell(size_t column_index) const 
    {
        return cell_ref(*this, col);
    }


    //========================================================================
    // TABLE VIEW IMPLEMENTATION
    //========================================================================
    
    // Document iterator implementation
    inline table_view::document_iterator::document_iterator(
        const table_view* table,
        bool at_end
    )
        : table_(table), at_end_(at_end)
    {
        if (!at_end_)
        {
            stack_.push_back({ table_->cat_, 0, nullptr });
            advance();
        }
    }
            
    inline row_view table_view::document_iterator::operator*() const
    {
        const auto& f = stack_.back();
        assert(f.current_row && f.current_row->kind == decl_kind::table_row);

        return row_view(
            &f.cat->table_rows[f.current_row->row_index],
            table_,
            f.cat
        );    
    }
    
    inline table_view::document_iterator&
    table_view::document_iterator::operator++()
    {
        advance();
        return *this;
    }
    
    inline bool table_view::document_iterator::operator==(
        const document_iterator& other
    ) const
    {
        if (at_end_ && other.at_end_) return true;
        if (at_end_ != other.at_end_) return false;
        
        const auto& a = stack_.back();
        const auto& b = other.stack_.back();
        
        return a.cat == b.cat && a.decl_index == b.decl_index;
    }
    
    inline void table_view::document_iterator::advance()
    {
        while (!stack_.empty())
        {
            frame& f = stack_.back();

            while (f.decl_index < f.cat->source_order.size())
            {
                const auto& decl = f.cat->source_order[f.decl_index++];

                switch (decl.kind)
                {
                    case decl_kind::table_row:
                        f.current_row = &decl;
                        return;

                    case decl_kind::subcategory:
                    {
                        auto it = f.cat->subcategories.find(decl.name);
                        if (it != f.cat->subcategories.end())
                        {
                            stack_.push_back({ it->second.get(), 0 });
                            // Continue from top of outer while - will reacquire 'f'
                            goto next_frame;
                        }
                        break;
                    }

                    case decl_kind::key:
                        break;
                }
            }

            stack_.pop_back();
            
            if (stack_.empty())
            {
                at_end_ = true;
                return;
            }

            stack_.back().current_row = nullptr;
            
        next_frame:;  // Label for goto
        }
    }


    inline std::optional<size_t> table_view::column_index(const std::string& name) const 
    {
        std::string lower = detail::to_lower(name);
        for (size_t i = 0; i < cat_->table_columns.size(); ++i) 
        {
            if (cat_->table_columns[i].name == lower)
                return i;
        }
        return std::nullopt;
    }
    
    inline row_view table_view::row(size_t index) const 
    {
        return row_view(&cat_->table_rows[index], this, cat_);
    }
    
    inline std::optional<table_view> table_view::subcategory(const std::string& name) const 
    {
        std::string lower = detail::to_lower(name);
        auto it = cat_->subcategories.find(lower);
        if (it == cat_->subcategories.end()) return std::nullopt;
        return table_view(it->second.get());
    }
    
    // Recursive iterator implementation
    inline table_view::recursive_iterator::recursive_iterator(
        const table_view* table,
        bool at_end
    )
        : table_(table), at_end_(at_end)
    {
        if (at_end_) return;

        stack_.push_back({
            table_->cat_,
            0,
            table_->cat_->subcategories.begin()
        });

        // Ensure first dereference is valid
        if (stack_.back().cat->table_rows.empty())
            advance();
    }
    
    inline row_view table_view::recursive_iterator::operator*() const
    {
        assert(!at_end_ && !stack_.empty());

        const auto& f = stack_.back();
        return row_view(
            &f.cat->table_rows[f.row_index],
            table_,
            f.cat
        );
    }
    
    inline table_view::recursive_iterator&
    table_view::recursive_iterator::operator++()
    {
        advance();
        return *this;
    }
    
    inline bool table_view::recursive_iterator::operator==(
        const recursive_iterator& other
    ) const
    {
        if (at_end_ && other.at_end_) return true;
        if (at_end_ != other.at_end_) return false;

        // Both valid iterators: compare *position*, not history
        const auto& a = stack_.back();
        const auto& b = other.stack_.back();

        return a.cat == b.cat
            && a.row_index == b.row_index;
    }

    inline void table_view::recursive_iterator::advance()
    {
        while (!stack_.empty())
        {
            auto& f = stack_.back();

            // 1. Advance within current category rows
            if (++f.row_index < f.cat->table_rows.size())
                return;

            // 2. Descend into next subcategory with rows
            while (f.child_it != f.cat->subcategories.end())
            {
                const category* child = f.child_it->second.get();
                ++f.child_it;
                stack_.push_back({child, 0, child->subcategories.begin()});
                return;
            }

            // 3. Exhausted this category → pop and continue upward
            stack_.pop_back();
        }

        at_end_ = true;
    }
    
    //========================================================================
    // ROW VIEW IMPLEMENTATION
    //========================================================================
    
    template<typename T>
    const T* row_view::get_ptr(const std::string& column_name) const
    {
        if (const typed_value* tv = get_typed_ptr(column_name))            
            return std::get_if<T>(&tv->val);
        return nullptr;
    }

    inline std::optional<value_ref> row_view::get(const std::string& name) const
    {
        auto idx = table_->column_index(name);
        if (!idx)
            return std::nullopt;
        return value_ref((*row_)[*idx]);        
    }

    inline const typed_value* row_view::get_typed_ptr(const std::string& name) const
    {
        auto idx = table_->column_index(name);
        if (!idx)
            return nullptr;
        return &(*row_)[*idx];        
    }
    
    inline std::optional<std::string> row_view::get_string(const std::string& col) const 
    {
        if (auto p = get_ptr<std::string>(col))
            return *p;
        return std::nullopt;
    }
    
    inline std::optional<int64_t> row_view::get_int(const std::string& col) const
    {
        if (auto p = get_ptr<int64_t>(col))
            return *p;
        return std::nullopt;        
    }

    inline std::optional<double> row_view::get_float(const std::string& col) const 
    {
        if (auto p = get_ptr<double>(col))
            return *p;
        return std::nullopt;        
    }

    inline std::optional<bool> row_view::get_bool(const std::string& col) const 
    {
        if (auto p = get_ptr<bool>(col))
            return *p;
        return std::nullopt;
    }
        
    inline bool row_view::is_base_row() const 
    {
        return source_category_ == table_->cat_;
    }

    //========================================================================
    // PUBLIC QUERY API IMPLEMENTATION
    //========================================================================

    inline std::optional<value_ref> get(const document& doc, const std::string& path)
    {
        if (const typed_value* tv = detail::find_typed_value_ptr(doc, path))
            return value_ref(*tv);
        return std::nullopt;
    }

    inline std::optional<std::string> get_string(const document& doc, const std::string& path)
    {
        if (auto ref = get(doc, path))
        {
            if (ref->declared() && ref->type() != value_type::string)
                return std::nullopt;

            if (auto* s = std::get_if<std::string>(&ref->raw()))
                return *s;
        }
        return std::nullopt;
    }

    inline std::optional<int64_t> get_int(const document& doc, const std::string& path)
    {
        if (auto ref = get(doc, path))
        {
            if (ref->declared() && ref->type() != value_type::integer)
                return std::nullopt;

            return ref->as_int();
        }
        return std::nullopt;
    }

    inline std::optional<double> get_float(const document& doc, const std::string& path)
    {
        if (auto ref = get(doc, path))
        {
            if (ref->declared() && ref->type() != value_type::decimal)
                return std::nullopt;

            return ref->as_float();
        }
        return std::nullopt;
    }

    inline std::optional<bool> get_bool(const document& doc, const std::string& path)
    {
        if (auto ref = get(doc, path))
        {
            if (ref->declared() && ref->type() != value_type::boolean)
                return std::nullopt;

            return ref->as_bool();
        }
        return std::nullopt;
    }
    
    inline std::optional<std::span<const std::string>>
    get_string_array(const document& doc, const std::string& path)
    {
        if (auto ref = get(doc, path))
            return ref->string_array();
        return std::nullopt;
    }    
        
    inline std::optional<std::span<const int64_t>>
    get_int_array(const document& doc, const std::string& path)
    {
        if (auto ref = get(doc, path))
            return ref->int_array();
        return std::nullopt;
    }    
        
    inline std::optional<std::span<const double>>
    get_float_array(const document& doc, const std::string& path)
    {
        if (auto ref = get(doc, path))
            return ref->float_array();
        return std::nullopt;
    }

    inline std::optional<table_view> get_table(const document& doc, const std::string& path)
    {
        auto parts = detail::split_path(path);
        if (parts.empty()) return std::nullopt;
        
        const category* cat = detail::resolve_category(doc, parts, detail::path_resolution::category);
        if (!cat) return std::nullopt;
        
        return table_view(cat);
    }
    
    inline std::string category_path(const category* cat, const document& doc)
    {
        if (!cat) return "";

        std::vector<std::string> parts;
        while (cat)
        {
            parts.push_back(cat->name);
            cat = cat->parent;
        }

        std::reverse(parts.begin(), parts.end());

        std::string path;
        for (size_t i = 0; i < parts.size(); ++i)
        {
            if (i) path += '.';
            path += parts[i];
        }

        return path;        
    }
    
    inline std::string to_path(const row_view& row, const document& doc)
    {
        return category_path(row.source(), doc);
    }
    
} // namespace arf

#endif // ARF_QUERY_HPP