// arf_query.hpp - A Readable Format (Arf!) - Query Interface
// Version 0.2.0

#ifndef ARF_QUERY_HPP
#define ARF_QUERY_HPP

#include "arf_core.hpp"
#include <iterator>

namespace arf 
{
    //========================================================================
    // FORWARD DECLARATIONS
    //========================================================================
    
    class table_view;
    class row_view;
    
    //========================================================================
    // TABLE ROW VIEW
    //========================================================================
    
    class row_view 
    {
    public:
        row_view(const table_row* row, const table_view* table, const category* source)
            : row_(row), table_(table), source_category_(source) {}
        
        // Index-based access
        const value& operator[](size_t index) const { return (*row_)[index]; }
        
        // Name-based value access
        std::optional<value> get(const std::string& column_name) const;
        
        // Typed name-based access
        template<typename T>
        std::optional<T> get(const std::string& column_name) const 
        {
            auto val = get(column_name);
            if (!val) return std::nullopt;
            
            if (auto* typed = std::get_if<T>(&*val))
                return *typed;
            return std::nullopt;
        }
        
        // Convenience typed getters
        std::optional<std::string> get_string(const std::string& col) const;
        std::optional<int64_t> get_int(const std::string& col) const;
        std::optional<double> get_float(const std::string& col) const;
        std::optional<bool> get_bool(const std::string& col) const;
        
        // Array getters
        std::optional<std::vector<std::string>> get_string_array(const std::string& col) const;
        std::optional<std::vector<int64_t>> get_int_array(const std::string& col) const;
        std::optional<std::vector<double>> get_float_array(const std::string& col) const;
        
        // Provenance information
        const category* source() const { return source_category_; }
        std::string source_name() const { return source_category_->name; }
        bool is_base_row() const;
        
        // Raw access
        const table_row& raw() const { return *row_; }
        
    private:
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
        // Iterator for recursive row traversal
        class recursive_iterator 
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = row_view;
            using difference_type = std::ptrdiff_t;
            using pointer = const row_view*;
            using reference = const row_view&;
            
            recursive_iterator(const table_view* table, bool at_end = false);
            
            row_view operator*() const;
            recursive_iterator& operator++();
            recursive_iterator operator++(int);
            
            bool operator==(const recursive_iterator& other) const;
            bool operator!=(const recursive_iterator& other) const { return !(*this == other); }
            
        private:
            void advance_to_next();
            
            const table_view* table_;
            const category* current_category_;
            size_t row_index_;
            std::vector<const category*> category_stack_;
            bool at_end_;
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
        
        // Recursive row iteration (includes subcategories)
        recursive_iterator rows_recursive_begin() const { return recursive_iterator(this, false); }
        recursive_iterator rows_recursive_end() const { return recursive_iterator(this, true); }
        recursive_range rows_recursive() const { return recursive_range(this); }
        
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
    // VALUE PROXY FOR REFLECTION
    //========================================================================
    
    class value_ref 
    {
    public:
        value_ref(const value& v) : val_(&v) {}
        
        bool is_scalar() const 
        {
            return std::holds_alternative<std::string>(*val_) ||
                   std::holds_alternative<int64_t>(*val_) ||
                   std::holds_alternative<double>(*val_) ||
                   std::holds_alternative<bool>(*val_);
        }
        
        bool is_array() const 
        {
            return std::holds_alternative<std::vector<std::string>>(*val_) ||
                   std::holds_alternative<std::vector<int64_t>>(*val_) ||
                   std::holds_alternative<std::vector<double>>(*val_);
        }
        
        bool is_string() const { return std::holds_alternative<std::string>(*val_); }
        bool is_int() const { return std::holds_alternative<int64_t>(*val_); }
        bool is_float() const { return std::holds_alternative<double>(*val_); }
        bool is_bool() const { return std::holds_alternative<bool>(*val_); }
        
        bool is_string_array() const { return std::holds_alternative<std::vector<std::string>>(*val_); }
        bool is_int_array() const { return std::holds_alternative<std::vector<int64_t>>(*val_); }
        bool is_float_array() const { return std::holds_alternative<std::vector<double>>(*val_); }
        
        std::optional<std::string> as_string() const 
        {
            if (auto* s = std::get_if<std::string>(val_))
                return *s;
            return std::nullopt;
        }
        
        std::optional<int64_t> as_int() const 
        {
            if (auto* i = std::get_if<int64_t>(val_))
                return *i;
            return std::nullopt;
        }
        
        std::optional<double> as_float() const 
        {
            if (auto* d = std::get_if<double>(val_))
                return *d;
            return std::nullopt;
        }
        
        std::optional<bool> as_bool() const 
        {
            if (auto* b = std::get_if<bool>(val_))
                return *b;
            return std::nullopt;
        }
        
        template<typename T>
        std::optional<std::vector<T>> as_array() const 
        {
            if (auto* arr = std::get_if<std::vector<T>>(val_))
                return *arr;
            return std::nullopt;
        }
        
        const value& raw() const { return *val_; }
        
    private:
        const value* val_;
    };
    
    //========================================================================
    // QUERY API
    //========================================================================
    
    // Basic value query (returns proxy for reflection)
    std::optional<value_ref> get(const document& doc, const std::string& path);
    
    // Direct value access
    std::optional<value> get_value(const document& doc, const std::string& path);
    std::optional<std::string> get_string(const document& doc, const std::string& path);
    std::optional<int64_t> get_int(const document& doc, const std::string& path);
    std::optional<double> get_float(const document& doc, const std::string& path);
    std::optional<bool> get_bool(const document& doc, const std::string& path);
    
    // Array access helpers
    template<typename T>
    std::optional<std::vector<T>> get_array(const document& doc, const std::string& path);
    
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
        
        inline const category* find_category(const document& doc, const std::vector<std::string>& path)
        {
            if (path.empty()) return nullptr;
            
            auto it = doc.categories.find(path[0]);
            if (it == doc.categories.end()) 
            {
                // Try root category
                it = doc.categories.find(std::string(ROOT_CATEGORY_NAME));
                if (it == doc.categories.end()) return nullptr;
            }
            const category* current = it->second.get();
            size_t start_idx = (path[0] == std::string(ROOT_CATEGORY_NAME)) ? 1 : 1;
            
            for (size_t i = start_idx; i < path.size() - 1; ++i)
            {
                auto sub_it = current->subcategories.find(path[i]);
                if (sub_it == current->subcategories.end()) return nullptr;
                current = sub_it->second.get();
            }

            return current;
        }
        
        inline const category* find_category_final(const document& doc, const std::vector<std::string>& path)
        {
            if (path.empty()) return nullptr;
            
            auto it = doc.categories.find(path[0]);
            if (it == doc.categories.end()) 
            {
                it = doc.categories.find(std::string(ROOT_CATEGORY_NAME));
                if (it == doc.categories.end()) return nullptr;
            }
            const category* current = it->second.get();
            
            size_t start_idx = (path[0] == std::string(ROOT_CATEGORY_NAME)) ? 1 : 1;
            for (size_t i = start_idx; i < path.size(); ++i)
            {
                auto sub_it = current->subcategories.find(path[i]);
                if (sub_it == current->subcategories.end()) return nullptr;
                current = sub_it->second.get();
            }

            return current;
        }
    
    } // namespace detail

    //========================================================================
    // TABLE VIEW IMPLEMENTATION
    //========================================================================
    
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
        
        // Only return table_view if subcategory has table columns
        if (it->second->table_columns.empty()) return std::nullopt;
        
        return table_view(it->second.get());
    }
    
    // Recursive iterator implementation
    inline table_view::recursive_iterator::recursive_iterator(const table_view* table, bool at_end)
        : table_(table), current_category_(table->cat_), row_index_(0), at_end_(at_end)
    {
        if (!at_end_) 
        {
            category_stack_.push_back(current_category_);
            if (current_category_->table_rows.empty())
                advance_to_next();
        }
    }
    
    inline row_view table_view::recursive_iterator::operator*() const 
    {
        return row_view(&current_category_->table_rows[row_index_], table_, current_category_);
    }
    
    inline table_view::recursive_iterator& table_view::recursive_iterator::operator++() 
    {
        advance_to_next();
        return *this;
    }
    
    inline table_view::recursive_iterator table_view::recursive_iterator::operator++(int) 
    {
        recursive_iterator tmp = *this;
        advance_to_next();
        return tmp;
    }
    
    inline bool table_view::recursive_iterator::operator==(const recursive_iterator& other) const 
    {
        if (at_end_ && other.at_end_) return true;
        if (at_end_ != other.at_end_) return false;
        return current_category_ == other.current_category_ && row_index_ == other.row_index_;
    }
    
    inline void table_view::recursive_iterator::advance_to_next() 
    {
        if (at_end_) return;
        
        // Try next row in current category
        ++row_index_;
        if (row_index_ < current_category_->table_rows.size())
            return;
        
        // Process subcategories of current category
        for (const auto& [name, subcat] : current_category_->subcategories) 
        {
            if (!subcat->table_rows.empty()) 
            {
                current_category_ = subcat.get();
                row_index_ = 0;
                category_stack_.push_back(current_category_);
                return;
            }
        }
        
        // Move up the stack
        while (!category_stack_.empty()) 
        {
            category_stack_.pop_back();
            if (category_stack_.empty()) 
            {
                at_end_ = true;
                return;
            }
            
            // Find next sibling
            const category* parent = category_stack_.back();
            bool found_current = false;
            
            for (const auto& [name, subcat] : parent->subcategories) 
            {
                if (found_current && !subcat->table_rows.empty()) 
                {
                    current_category_ = subcat.get();
                    row_index_ = 0;
                    category_stack_.push_back(current_category_);
                    return;
                }
                if (subcat.get() == current_category_)
                    found_current = true;
            }
            
            current_category_ = parent;
        }
        
        at_end_ = true;
    }
    
    //========================================================================
    // ROW VIEW IMPLEMENTATION
    //========================================================================
    
    inline std::optional<value> row_view::get(const std::string& column_name) const 
    {
        auto idx = table_->column_index(column_name);
        if (!idx || *idx >= row_->size()) return std::nullopt;
        return (*row_)[*idx];
    }
    
    inline std::optional<std::string> row_view::get_string(const std::string& col) const 
    {
        return get<std::string>(col);
    }
    
    inline std::optional<int64_t> row_view::get_int(const std::string& col) const 
    {
        return get<int64_t>(col);
    }
    
    inline std::optional<double> row_view::get_float(const std::string& col) const 
    {
        return get<double>(col);
    }
    
    inline std::optional<bool> row_view::get_bool(const std::string& col) const 
    {
        return get<bool>(col);
    }
    
    inline std::optional<std::vector<std::string>> row_view::get_string_array(const std::string& col) const 
    {
        return get<std::vector<std::string>>(col);
    }
    
    inline std::optional<std::vector<int64_t>> row_view::get_int_array(const std::string& col) const 
    {
        return get<std::vector<int64_t>>(col);
    }
    
    inline std::optional<std::vector<double>> row_view::get_float_array(const std::string& col) const 
    {
        return get<std::vector<double>>(col);
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
        auto val = get_value(doc, path);
        if (!val) return std::nullopt;
        
        auto parts = detail::split_path(path);
        if (parts.empty()) return std::nullopt;
        
        const category* cat = detail::find_category(doc, parts);
        if (!cat) return std::nullopt;
        
        const std::string& key = parts.back();
        auto it = cat->key_values.find(key);
        if (it == cat->key_values.end()) return std::nullopt;
        
        return value_ref(it->second);
    }

    inline std::optional<value> get_value(const document& doc, const std::string& path)
    {
        auto parts = detail::split_path(path);
        if (parts.empty()) return std::nullopt;
        
        const category* cat = detail::find_category(doc, parts);
        if (!cat) return std::nullopt;
        
        const std::string& key = parts.back();
        auto it = cat->key_values.find(key);
        if (it == cat->key_values.end()) return std::nullopt;
        
        return it->second;
    }

    inline std::optional<std::string> get_string(const document& doc, const std::string& path)
    {
        auto val = get_value(doc, path);
        if (!val) return std::nullopt;
        
        if (auto* str = std::get_if<std::string>(&*val))
            return *str;
        
        return std::nullopt;
    }

    inline std::optional<int64_t> get_int(const document& doc, const std::string& path)
    {
        auto val = get_value(doc, path);
        if (!val) return std::nullopt;
        
        if (auto* i = std::get_if<int64_t>(&*val))
            return *i;
        
        // Try to convert from string
        if (auto* str = std::get_if<std::string>(&*val))
        {
            try {
                return std::stoll(*str);
            } catch (...) {
                return std::nullopt;
            }
        }
        
        return std::nullopt;
    }

    inline std::optional<double> get_float(const document& doc, const std::string& path)
    {
        auto val = get_value(doc, path);
        if (!val) return std::nullopt;
        
        if (auto* d = std::get_if<double>(&*val))
            return *d;
        
        // Try to convert from string
        if (auto* str = std::get_if<std::string>(&*val))
        {
            try {
                return std::stod(*str);
            } catch (...) {
                return std::nullopt;
            }
        }
        
        return std::nullopt;
    }

    inline std::optional<bool> get_bool(const document& doc, const std::string& path)
    {
        auto val = get_value(doc, path);
        if (!val) return std::nullopt;
        
        if (auto* b = std::get_if<bool>(&*val))
            return *b;
        
        // Try to convert from string
        if (auto* str = std::get_if<std::string>(&*val))
        {
            std::string lower = detail::to_lower(*str);
            if (lower == "true" || lower == "yes" || lower == "1")
                return true;
            if (lower == "false" || lower == "no" || lower == "0")
                return false;
        }
        
        return std::nullopt;
    }
    
    template<typename T>
    inline std::optional<std::vector<T>> get_array(const document& doc, const std::string& path)
    {
        auto val = get_value(doc, path);
        if (!val) return std::nullopt;
        
        if (auto* arr = std::get_if<std::vector<T>>(&*val))
            return *arr;
        
        return std::nullopt;
    }
    
    inline std::optional<table_view> get_table(const document& doc, const std::string& path)
    {
        auto parts = detail::split_path(path);
        if (parts.empty()) return std::nullopt;
        
        const category* cat = detail::find_category_final(doc, parts);
        if (!cat) return std::nullopt;
        
        // Only return table_view if category has table columns defined
        if (cat->table_columns.empty()) return std::nullopt;
        
        return table_view(cat);
    }
    
    inline std::string category_path(const category* cat, const document& doc)
    {
        // This is a simplified implementation
        // A full implementation would need to walk the document tree
        // For now, just return the category name
        return cat ? cat->name : "";
    }
    
    inline std::string to_path(const row_view& row, const document& doc)
    {
        return category_path(row.source(), doc);
    }
    
} // namespace arf

#endif // ARF_QUERY_HPP