#pragma once

#include "types.hpp"

#include <cmath>
#include <vector>
#include <memory>
#include <cstdint>

namespace ruic
{
    // ----------------------------------------------------
    // Forward Declarations
    // ----------------------------------------------------
    struct RuicContext;
    struct RuicDrawData;

    // ----------------------------------------------------
    // Context creation and access
    // ----------------------------------------------------
    auto create_context() -> RuicContext*;
    void destroy_context(RuicContext* ctx = nullptr);
    auto get_current_context() -> RuicContext*;
    void set_current_context(RuicContext* ctx);

    class Element;

    template <typename T>
    using ElementPtr = std::shared_ptr<T>;
    using ElementBasePtr = ElementPtr<Element>;

    void add_to_root(const ElementBasePtr& element);
    void remove_from_root(const ElementBasePtr& element);

    void set_root_size(std::uint32_t width, std::uint32_t height);

    void set_dirty();

    bool render();  // Returns TRUE if render data changed
    auto get_draw_data() -> const RuicDrawData*;

}