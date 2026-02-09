#ifndef ARF_TESTS_EDITOR__
#define ARF_TESTS_EDITOR__

#include "arf_test_harness.hpp"
#include "../include/arf_editor.hpp"
#include "../include/arf.hpp"

namespace arf::tests
{
using namespace arf;

inline bool update_single_typed_key()
{
    constexpr std::string_view src =
        "a:int = 42\n"
        "# x  y\n"
        "  1  2\n"
        "  3  4\n";

    auto ctx = load(src);
    auto & doc = ctx.document;
        
    EXPECT(ctx.errors.empty(), "error emitted");
    
    auto ed = editor(doc);

    auto key_view = doc.key("a");
    EXPECT(key_view.has_value(), "The key 'a' should exist");
    
    auto & val = key_view->value();
    ed.set_key_value(key_view->id(), 13);

    // Check document node consistency:
    {
        auto kn = ed._unsafe_access_internal_document_container(key_view->id());
        EXPECT (kn != nullptr, "Should find key by ID in key nodes");
        EXPECT(val.held_type() == kn->type, "The held type of the value and the node type should be the same");
        EXPECT(val.held_type() == kn->value.type, "The held type of the value and the value type should be the same");
    }

    // Check value consistency:
    EXPECT(val.held_type() == value_type::integer, "Node should be of integer type");
    EXPECT(std::get<int64_t>(val.val) == 13, "Node value should be 13");

    return true;
}

//============================================================================
// Test Runner
//============================================================================

inline void run_editor_tests()
{
    SUBCAT("Foobar");
    RUN_TEST(update_single_typed_key);
}

}

#endif