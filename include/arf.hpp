// arf.hpp - A Readable Format (Arf!)
// A human-readable hierarchical configuration and data format
// Version 0.1.0
//
// Usage:
//   #include "arf.hpp"
//   
//   arf::document doc = arf::parse(config_string);
//   std::string output = arf::serialize(doc);
//   auto value = arf::get(doc, "category.subcategory.key");

#ifndef ARF_HPP
#define ARF_HPP

#include <string>
#include <vector>
#include <map>
#include <variant>
#include <optional>
#include <memory>
#include <sstream>
#include <functional>
#include <algorithm>
#include <stdexcept>

namespace arf 
{
    //========================================================================
    // CORE DATA STRUCTURES
    //========================================================================
    
    enum class value_type 
    {
        string,
        integer,
        decimal,
        boolean,
        date,
        string_array,
        int_array,
        float_array
    };
    
    using value = std::variant
                  <
                      std::string,
                      int64_t,
                      double,
                      bool,
                      std::vector<std::string>,
                      std::vector<int64_t>,
                      std::vector<double>
                  >;
    
    struct column 
    {
        std::string name;
        value_type type;
        
        bool operator==(const column& other) const
        {
            return name == other.name && type == other.type;
        }
    };
    
    using table_row = std::vector<value>;
    
    struct category 
    {
        std::string name;
        std::map<std::string, std::string> key_values;
        std::vector<column> table_columns;
        std::vector<table_row> table_rows;
        std::map<std::string, std::unique_ptr<category>> subcategories;
    };
    
    struct document 
    {
        std::map<std::string, std::unique_ptr<category>> categories;
    };
    
    //========================================================================
    // PUBLIC API - C++ Interface
    //========================================================================
    
    // Parse Arf format text into document structure
    document parse(const std::string& input);
    
    // Serialize document back to Arf format text
    std::string serialize(const document& doc);
    
    // Query document by path (e.g., "server.version" or "monsters.goblins")
    std::optional<std::string> get_string(const document& doc, const std::string& path);
    std::optional<int64_t> get_int(const document& doc, const std::string& path);
    std::optional<double> get_float(const document& doc, const std::string& path);
    std::optional<bool> get_bool(const document& doc, const std::string& path);
    
    //========================================================================
    // IMPLEMENTATION DETAILS
    //========================================================================
    
    namespace detail 
    {
        //====================================================================
        // Parser Implementation
        //====================================================================
        
        class parser_impl 
        {
        public:
            document parse(const std::string& input) 
            {
                lines_ = split_lines(input);
                line_num_ = 0;
                doc_ = document{};
                category_stack_.clear();
                current_table_.clear();
                in_table_mode_ = false;
                table_mode_depth_ = 0;
                
                while (line_num_ < lines_.size()) 
                {
                    parse_line(lines_[line_num_]);
                    ++line_num_;
                }
                
                return std::move(doc_);
            }
            
        private:
            std::vector<std::string> lines_;
            size_t line_num_ = 0;
            document doc_;
            std::vector<category*> category_stack_;
            std::vector<column> current_table_;
            bool in_table_mode_ = false;
            size_t table_mode_depth_ = 0;
            
            std::vector<std::string> split_lines(const std::string& input) 
            {
                std::vector<std::string> result;
                std::istringstream stream(input);
                std::string line;
                while (std::getline(stream, line))
                    result.push_back(line);
                return result;
            }
            
            std::string trim(const std::string& str) 
            {
                size_t start = str.find_first_not_of(" \t\r\n");
                if (start == std::string::npos) return "";
                size_t end = str.find_last_not_of(" \t\r\n");
                return str.substr(start, end - start + 1);
            }
            
            void parse_line(const std::string& line) 
            {
                std::string trimmed = trim(line);
                
                if (trimmed.empty()) 
                {
                    if (in_table_mode_ && category_stack_.size() == table_mode_depth_)
                    {
                        in_table_mode_ = false;
                        table_mode_depth_ = 0;
                    }
                    return;
                }
                
                if (trimmed.size() >= 2 && trimmed[0] == '/' && trimmed[1] == '/')
                    return;
                
                if (!trimmed.empty() && trimmed.back() == ':' && trimmed[0] != ':' && trimmed[0] != '#') 
                {
                    parse_top_level_category(trimmed);
                    return;
                }
                
                if (trimmed[0] == ':') 
                {
                    parse_subcategory_open(trimmed);
                    return;
                }
                
                if (trimmed[0] == '/') 
                {
                    parse_subcategory_close(trimmed);
                    return;
                }
                
                if (trimmed[0] == '#') 
                {
                    parse_table_header(trimmed);
                    return;
                }
                
                if (trimmed.find('=') != std::string::npos && !in_table_mode_) 
                {
                    parse_key_value(trimmed);
                    return;
                }
                
                if (in_table_mode_) 
                {
                    parse_table_row(trimmed);
                    return;
                }
            }
            
            void parse_top_level_category(const std::string& line) 
            {
                std::string name = line.substr(0, line.size() - 1);
                name = trim(name);
                
                category_stack_.clear();
                in_table_mode_ = false;
                current_table_.clear();
                table_mode_depth_ = 0;
                
                auto cat = std::make_unique<category>();
                cat->name = name;
                category* ptr = cat.get();
                doc_.categories[name] = std::move(cat);
                category_stack_.push_back(ptr);
            }
            
            void parse_subcategory_open(const std::string& line) 
            {
                if (category_stack_.empty()) return;
                
                std::string name = trim(line.substr(1));
                
                category* parent = category_stack_.back();
                auto subcat = std::make_unique<category>();
                subcat->name = name;
                
                if (in_table_mode_)
                    subcat->table_columns = current_table_;
                
                category* ptr = subcat.get();
                parent->subcategories[name] = std::move(subcat);
                category_stack_.push_back(ptr);
            }
            
            void parse_subcategory_close(const std::string& line) 
            {
                if (category_stack_.size() <= 1) return;
                
                std::string name = trim(line.substr(1));
                
                if (name.empty() || name == "subcategory" || category_stack_.back()->name == name) 
                {
                    category_stack_.pop_back();
                } 
                else 
                {
                    bool found = false;
                    for (auto it = category_stack_.rbegin(); it != category_stack_.rend(); ++it) 
                    {
                        if ((*it)->name == name) 
                        {
                            size_t pos = std::distance(category_stack_.begin(), it.base()) - 1;
                            category_stack_.erase(category_stack_.begin() + pos + 1, category_stack_.end());
                            found = true;
                            break;
                        }
                    }
                    if (!found && category_stack_.size() > 1)
                        category_stack_.pop_back();
                }
                
                if (in_table_mode_ && !category_stack_.empty())
                {
                    category* current = category_stack_.back();
                    if (current->table_columns.empty() && !current_table_.empty())
                        current->table_columns = current_table_;
                }
            }
            
            void parse_table_header(const std::string& line) 
            {
                current_table_.clear();
                std::string header = trim(line.substr(1));
                
                std::istringstream stream(header);
                std::string token;
                while (stream >> token) 
                {
                    column col;
                    size_t colon_pos = token.find(':');
                    if (colon_pos != std::string::npos) 
                    {
                        col.name = token.substr(0, colon_pos);
                        col.type = parse_type(token.substr(colon_pos + 1));
                    } 
                    else 
                    {
                        col.name = token;
                        col.type = value_type::string;
                    }
                    current_table_.push_back(col);
                }
                
                in_table_mode_ = true;
                table_mode_depth_ = category_stack_.size();
                
                if (!category_stack_.empty()) 
                    category_stack_.back()->table_columns = current_table_;
            }
            
            value_type parse_type(const std::string& type_str) 
            {
                if (type_str == "int") return value_type::integer;
                if (type_str == "float") return value_type::decimal;
                if (type_str == "bool") return value_type::boolean;
                if (type_str == "date") return value_type::date;
                if (type_str == "str[]") return value_type::string_array;
                if (type_str == "int[]") return value_type::int_array;
                if (type_str == "float[]") return value_type::float_array;
                return value_type::string;
            }
            
            void parse_key_value(const std::string& line) 
            {
                size_t eq_pos = line.find('=');
                if (eq_pos == std::string::npos) return;
                
                std::string key = trim(line.substr(0, eq_pos));
                std::string val = trim(line.substr(eq_pos + 1));
                
                if (!category_stack_.empty())
                    category_stack_.back()->key_values[key] = val;
            }
            
            void parse_table_row(const std::string& line) 
            {
                if (current_table_.empty() || category_stack_.empty()) return;
                
                std::vector<std::string> cells = split_table_cells(line);
                
                while (cells.size() < current_table_.size()) 
                    cells.push_back("");
                if (cells.size() > current_table_.size()) 
                    cells.resize(current_table_.size());
                
                table_row row;
                for (size_t i = 0; i < cells.size(); i++)
                    row.push_back(parse_value(cells[i], current_table_[i].type));
                
                category_stack_.back()->table_rows.push_back(std::move(row));
            }
            
            std::vector<std::string> split_table_cells(const std::string& line) 
            {
                std::vector<std::string> cells;
                std::string current;
                int spaces = 0;
                
                for (char c : line) 
                {
                    if (c == ' ') 
                    {
                        ++spaces;
                        if (spaces >= 2 && !current.empty()) 
                        {
                            cells.push_back(trim(current));
                            current.clear();
                            spaces = 0;
                        }
                    } 
                    else 
                    {
                        if (spaces == 1) current += ' ';
                        spaces = 0;
                        current += c;
                    }
                }
                
                if (!current.empty())
                    cells.push_back(trim(current));
                
                return cells;
            }
            
            value parse_value(const std::string& str, value_type type) 
            {
                switch (type) 
                {
                    case value_type::integer:
                        return static_cast<int64_t>(std::stoll(str));
                    case value_type::decimal:
                        return std::stod(str);
                    case value_type::boolean:
                        return str == "true" || str == "yes" || str == "1";
                    case value_type::string_array:
                        return split_array<std::string>(str, [](const std::string& s) { return s; });
                    case value_type::int_array:
                        return split_array<int64_t>(str, [](const std::string& s) { return std::stoll(s); });
                    case value_type::float_array:
                        return split_array<double>(str, [](const std::string& s) { return std::stod(s); });
                    default:
                        return str;
                }
            }
            
            template<typename T, typename F>
            std::vector<T> split_array(const std::string& str, F converter) 
            {
                std::vector<T> result;
                std::string current;
                
                for (char c : str) 
                {
                    if (c == '|') 
                    {
                        if (!current.empty()) 
                        {
                            result.push_back(converter(trim(current)));
                            current.clear();
                        }
                    } 
                    else 
                    {
                        current += c;
                    }
                }
                
                if (!current.empty())
                    result.push_back(converter(trim(current)));
                
                return result;
            }
        };
        
        //====================================================================
        // Serializer Implementation
        //====================================================================
        
        class serializer_impl 
        {
        public:
            std::string serialize(const document& doc) 
            {
                std::ostringstream out;
                
                bool first_category = true;
                for (const auto& [name, cat] : doc.categories)
                {
                    if (!first_category)
                        out << "\n";
                    first_category = false;
                    
                    serialize_category(out, *cat, 0);
                }
                
                return out.str();
            }
            
        private:
            void serialize_category(std::ostringstream& out, const category& cat, int depth) 
            {
                std::string indent(depth * 2, ' ');
                
                // Category name
                out << indent << cat.name << ":\n";
                
                // Table
                if (!cat.table_columns.empty())
                {
                    serialize_table(out, cat, depth + 1);
                }
                
                // Key-values
                if (!cat.key_values.empty())
                {
                    if (!cat.table_columns.empty())
                        out << "\n";
                    
                    for (const auto& [key, val] : cat.key_values)
                        out << indent << "  " << key << " = " << val << "\n";
                }
                
                // Subcategories
                for (const auto& [name, subcat] : cat.subcategories)
                {
                    out << "\n" << indent << ":" << name << "\n";
                    
                    // If subcategory has same table structure, just serialize rows
                    if (subcat->table_columns == cat.table_columns && !subcat->table_rows.empty())
                    {
                        serialize_table_rows(out, *subcat, depth + 1);
                    }
                    else
                    {
                        // Different structure - serialize table if present
                        if (!subcat->table_columns.empty())
                            serialize_table(out, *subcat, depth + 2);
                        
                        // Key-values
                        for (const auto& [key, val] : subcat->key_values)
                            out << indent << "    " << key << " = " << val << "\n";
                    }
                    
                    out << indent << "/" << name << "\n";
                }
                
                // Close category
                out << "/" << cat.name << "\n";
            }
            
            void serialize_table(std::ostringstream& out, const category& cat, int depth)
            {
                std::string indent(depth * 2, ' ');
                
                // Table header
                out << indent << "#";
                for (size_t i = 0; i < cat.table_columns.size(); ++i)
                {
                    out << " " << cat.table_columns[i].name << ":" << type_to_string(cat.table_columns[i].type);
                }
                out << "\n";
                
                // Table rows
                serialize_table_rows(out, cat, depth);
            }
            
            void serialize_table_rows(std::ostringstream& out, const category& cat, int depth)
            {
                std::string indent(depth * 2, ' ');
                
                for (const auto& row : cat.table_rows)
                {
                    out << indent;
                    for (size_t i = 0; i < row.size(); ++i)
                    {
                        if (i > 0) out << "  ";
                        serialize_value(out, row[i]);
                    }
                    out << "\n";
                }
            }
            
            void serialize_value(std::ostringstream& out, const value& val)
            {
                std::visit([&out](const auto& v) 
                {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, std::string>)
                        out << v;
                    else if constexpr (std::is_same_v<T, int64_t>)
                        out << v;
                    else if constexpr (std::is_same_v<T, double>)
                        out << v;
                    else if constexpr (std::is_same_v<T, bool>)
                        out << (v ? "true" : "false");
                    else if constexpr (std::is_same_v<T, std::vector<std::string>>)
                    {
                        for (size_t i = 0; i < v.size(); ++i)
                        {
                            out << v[i];
                            if (i < v.size() - 1) out << "|";
                        }
                    }
                    else if constexpr (std::is_same_v<T, std::vector<int64_t>>)
                    {
                        for (size_t i = 0; i < v.size(); ++i)
                        {
                            out << v[i];
                            if (i < v.size() - 1) out << "|";
                        }
                    }
                    else if constexpr (std::is_same_v<T, std::vector<double>>)
                    {
                        for (size_t i = 0; i < v.size(); ++i)
                        {
                            out << v[i];
                            if (i < v.size() - 1) out << "|";
                        }
                    }
                }, val);
            }
            
            std::string type_to_string(value_type type)
            {
                switch (type)
                {
                    case value_type::string: return "str";
                    case value_type::integer: return "int";
                    case value_type::decimal: return "float";
                    case value_type::boolean: return "bool";
                    case value_type::date: return "date";
                    case value_type::string_array: return "str[]";
                    case value_type::int_array: return "int[]";
                    case value_type::float_array: return "float[]";
                    default: return "str";
                }
            }
        };
        
        //====================================================================
        // Query Helpers
        //====================================================================
        
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
                        parts.push_back(current);
                        current.clear();
                    }
                }
                else
                {
                    current += c;
                }
            }
            
            if (!current.empty())
                parts.push_back(current);
            
            return parts;
        }
        
        inline const category* find_category(const document& doc, const std::vector<std::string>& path)
        {
            if (path.empty()) return nullptr;
            
            auto it = doc.categories.find(path[0]);
            if (it == doc.categories.end()) return nullptr;
            
            const category* current = it->second.get();
            
            for (size_t i = 1; i < path.size() - 1; ++i)
            {
                auto sub_it = current->subcategories.find(path[i]);
                if (sub_it == current->subcategories.end()) return nullptr;
                current = sub_it->second.get();
            }
            
            return current;
        }
        
    } // namespace detail
    
    //========================================================================
    // PUBLIC API IMPLEMENTATION
    //========================================================================
    
    inline document parse(const std::string& input)
    {
        detail::parser_impl parser;
        return parser.parse(input);
    }
    
    inline std::string serialize(const document& doc)
    {
        detail::serializer_impl serializer;
        return serializer.serialize(doc);
    }
    
    inline std::optional<std::string> get_string(const document& doc, const std::string& path)
    {
        auto parts = detail::split_path(path);
        if (parts.size() < 2) return std::nullopt;
        
        const category* cat = detail::find_category(doc, parts);
        if (!cat) return std::nullopt;
        
        const std::string& key = parts.back();
        auto it = cat->key_values.find(key);
        if (it == cat->key_values.end()) return std::nullopt;
        
        return it->second;
    }
    
    inline std::optional<int64_t> get_int(const document& doc, const std::string& path)
    {
        auto str_val = get_string(doc, path);
        if (!str_val) return std::nullopt;
        
        try {
            return std::stoll(*str_val);
        } catch (...) {
            return std::nullopt;
        }
    }
    
    inline std::optional<double> get_float(const document& doc, const std::string& path)
    {
        auto str_val = get_string(doc, path);
        if (!str_val) return std::nullopt;
        
        try {
            return std::stod(*str_val);
        } catch (...) {
            return std::nullopt;
        }
    }
    
    inline std::optional<bool> get_bool(const document& doc, const std::string& path)
    {
        auto str_val = get_string(doc, path);
        if (!str_val) return std::nullopt;
        
        std::string lower = *str_val;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        if (lower == "true" || lower == "yes" || lower == "1")
            return true;
        if (lower == "false" || lower == "no" || lower == "0")
            return false;
        
        return std::nullopt;
    }
    
} // namespace arf

//============================================================================
// C COMPATIBILITY LAYER
//============================================================================

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle types
typedef struct arf_document_t arf_document_t;

// Parse Arf format from C string
arf_document_t* arf_parse(const char* input);

// Serialize document to newly allocated C string (caller must free)
char* arf_serialize(const arf_document_t* doc);

// Query functions returning newly allocated strings (caller must free)
char* arf_get_string(const arf_document_t* doc, const char* path);

// Query functions with direct return values
int arf_get_int(const arf_document_t* doc, const char* path, int64_t* out_value);
int arf_get_float(const arf_document_t* doc, const char* path, double* out_value);
int arf_get_bool(const arf_document_t* doc, const char* path, int* out_value);

// Memory management
void arf_free_document(arf_document_t* doc);
void arf_free_string(char* str);

#ifdef __cplusplus
}

// C API implementation (inline in header for single-file distribution)
#include <cstring>
namespace arf 
{
    namespace c_api 
    {
        inline arf_document_t* arf_parse(const char* input)
        {
            if (!input) return nullptr;
            
            try {
                auto* doc = new arf::document(arf::parse(std::string(input)));
                return reinterpret_cast<arf_document_t*>(doc);
            } catch (...) {
                return nullptr;
            }
        }
        
        inline char* arf_serialize(const arf_document_t* doc)
        {
            if (!doc) return nullptr;
            
            try {
                const auto* cpp_doc = reinterpret_cast<const arf::document*>(doc);
                std::string result = arf::serialize(*cpp_doc);
                
                char* c_str = static_cast<char*>(malloc(result.size() + 1));
                if (c_str) {
                    std::memcpy(c_str, result.c_str(), result.size() + 1);
                }
                return c_str;
            } catch (...) {
                return nullptr;
            }
        }
        
        inline char* arf_get_string(const arf_document_t* doc, const char* path)
        {
            if (!doc || !path) return nullptr;
            
            try {
                const auto* cpp_doc = reinterpret_cast<const arf::document*>(doc);
                auto result = arf::get_string(*cpp_doc, std::string(path));
                
                if (!result) return nullptr;
                
                char* c_str = static_cast<char*>(malloc(result->size() + 1));
                if (c_str) {
                    memcpy(c_str, result->c_str(), result->size() + 1);
                }
                return c_str;
            } 
            catch (...) {
                return nullptr;
            }
        }
        
        inline int arf_get_int(const arf_document_t* doc, const char* path, int64_t* out_value)
        {
            if (!doc || !path || !out_value) return 0;
            
            try {
                const auto* cpp_doc = reinterpret_cast<const arf::document*>(doc);
                auto result = arf::get_int(*cpp_doc, std::string(path));
                
                if (!result) return 0;
                
                *out_value = *result;
                return 1;
            } catch (...) {
                return 0;
            }
        }
        
        inline int arf_get_float(const arf_document_t* doc, const char* path, double* out_value)
        {
            if (!doc || !path || !out_value) return 0;
            
            try {
                const auto* cpp_doc = reinterpret_cast<const arf::document*>(doc);
                auto result = arf::get_float(*cpp_doc, std::string(path));
                
                if (!result) return 0;
                
                *out_value = *result;
                return 1;
            } catch (...) {
                return 0;
            }
        }
        
        inline int arf_get_bool(const arf_document_t* doc, const char* path, int* out_value)
        {
            if (!doc || !path || !out_value) return 0;
            
            try {
                const auto* cpp_doc = reinterpret_cast<const arf::document*>(doc);
                auto result = arf::get_bool(*cpp_doc, std::string(path));
                
                if (!result) return 0;
                
                *out_value = *result ? 1 : 0;
                return 1;
            } catch (...) {
                return 0;
            }
        }
        
        inline void arf_free_document(arf_document_t* doc)
        {
            if (doc) {
                delete reinterpret_cast<arf::document*>(doc);
            }
        }
        
        inline void arf_free_string(char* str)
        {
            if (str) {
                free(str);
            }
        }
    }
}

// Wire up C API to implementation
inline arf_document_t* arf_parse(const char* input) { return arf::c_api::arf_parse(input); }
inline char* arf_serialize(const arf_document_t* doc) { return arf::c_api::arf_serialize(doc); }
inline char* arf_get_string(const arf_document_t* doc, const char* path) { return arf::c_api::arf_get_string(doc, path); }
inline int arf_get_int(const arf_document_t* doc, const char* path, int64_t* out_value) { return arf::c_api::arf_get_int(doc, path, out_value); }
inline int arf_get_float(const arf_document_t* doc, const char* path, double* out_value) { return arf::c_api::arf_get_float(doc, path, out_value); }
inline int arf_get_bool(const arf_document_t* doc, const char* path, int* out_value) { return arf::c_api::arf_get_bool(doc, path, out_value); }
inline void arf_free_document(arf_document_t* doc) { arf::c_api::arf_free_document(doc); }
inline void arf_free_string(char* str) { arf::c_api::arf_free_string(str); }

#endif // __cplusplus

#endif // ARF_HPP