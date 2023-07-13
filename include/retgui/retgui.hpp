#pragma once

#include "types.hpp"

#include <cmath>
#include <vector>
#include <memory>
#include <cstdint>

namespace retgui
{
    // ----------------------------------------------------
    // Forward Declarations
    // ----------------------------------------------------
    struct RetGuiContext;
    struct RetGuiDrawData;

    // ----------------------------------------------------
    // Context creation and access
    // ----------------------------------------------------
    auto create_context() -> RetGuiContext*;
    void destroy_context(RetGuiContext* ctx = nullptr);
    auto get_current_context() -> RetGuiContext*;
    void set_current_context(RetGuiContext* ctx);

    class Element;

    template <typename T>
    using ElementPtr = std::shared_ptr<T>;
    using ElementBasePtr = ElementPtr<Element>;

    void add_to_root(const ElementBasePtr& element);
    void remove_from_root(const ElementBasePtr& element);

    void set_root_size(std::uint32_t width, std::uint32_t height);

    void set_dirty();

    bool render();  // Returns TRUE if render data changed
    auto get_draw_data() -> const RetGuiDrawData*;

}