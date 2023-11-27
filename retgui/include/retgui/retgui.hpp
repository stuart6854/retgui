#pragma once

#include <memory>
#include <string>
#include <type_traits>

namespace retgui
{
	//////////////////////////////////////////////////////////////////////////
	/// Utility
	//////////////////////////////////////////////////////////////////////////

#pragma region Utility

	template <typename E>
	struct FEnableBitmaskOperators
	{
		static constexpr bool enable = false;
	};

	template <typename E>
	std::enable_if_t<FEnableBitmaskOperators<E>::enable, E> operator|(E lhs, E rhs)
	{
		return static_cast<E>(static_cast<std::underlying_type_t<E>>(lhs) | static_cast<std::underlying_type_t<E>>(rhs));
	}
	template <typename E>
	std::enable_if_t<FEnableBitmaskOperators<E>::enable, E> operator&(E lhs, E rhs)
	{
		return static_cast<E>(static_cast<std::underlying_type_t<E>>(lhs) & static_cast<std::underlying_type_t<E>>(rhs));
	}
	template <typename E>
	std::enable_if_t<FEnableBitmaskOperators<E>::enable, E> operator^(E lhs, E rhs)
	{
		return static_cast<E>(static_cast<std::underlying_type_t<E>>(lhs) ^ static_cast<std::underlying_type_t<E>>(rhs));
	}
	template <typename E>
	std::enable_if_t<FEnableBitmaskOperators<E>::enable, E> operator~(E e)
	{
		return static_cast<E>(~static_cast<std::underlying_type_t<E>>(e));
	}
	template <typename E>
	std::enable_if_t<FEnableBitmaskOperators<E>::enable, E> operator|=(E lhs, E rhs)
	{
		return lhs = static_cast<E>(static_cast<std::underlying_type_t<E>>(lhs) | static_cast<std::underlying_type_t<E>>(rhs));
	}
	template <typename E>
	std::enable_if_t<FEnableBitmaskOperators<E>::enable, E> operator&=(E lhs, E rhs)
	{
		return lhs = static_cast<E>(static_cast<std::underlying_type_t<E>>(lhs) & static_cast<std::underlying_type_t<E>>(rhs));
	}
	template <typename E>
	std::enable_if_t<FEnableBitmaskOperators<E>::enable, E> operator^=(E lhs, E rhs)
	{
		return lhs = static_cast<E>(static_cast<std::underlying_type_t<E>>(lhs) ^ static_cast<std::underlying_type_t<E>>(rhs));
	}

#pragma endregion

	//////////////////////////////////////////////////////////////////////////
	/// Core
	//////////////////////////////////////////////////////////////////////////

#pragma region Core

	using f32 = float;
	using rgba32 = uint32_t;

	struct Rect
	{
		f32 x = 0.0f;
		f32 y = 0.0f;
		f32 width = 0.0f;
		f32 height = 0.0f;
	};

	enum class EWidgetFlags
	{
		None = 0,
		Clickable = 1 << 0,		 // Takes mouse events and hovering, responds to clicks and drags.
		ViewScroll = 1 << 1,	 // Mouse wheel scrolling to shift the "view offset".
		DrawText = 1 << 2,		 // Require text to be rendered.
		DrawBorder = 1 << 3,	 // Require border to be renderer.
		DrawBackground = 1 << 4, // Require background to be rendered.
		DrawDropShadow = 1 << 5, // Require drop shadow to be rendered.
		Clip = 1 << 6,
		HotAnimation = 1 << 7,	  // ??
		ActiveAnimation = 1 << 8, // ??
	};
	template <>
	struct FEnableBitmaskOperators<EWidgetFlags>
	{
		static constexpr bool enable = true;
	};

	using WidgetPtr = struct Widget*;
	struct Widget
	{
		// Tree links
		std::shared_ptr<Widget> First = nullptr;
		std::shared_ptr<Widget> Last = nullptr;
		std::shared_ptr<Widget> Next = nullptr;
		std::shared_ptr<Widget> Prev = nullptr;
		std::shared_ptr<Widget> Parent = nullptr;

		EWidgetFlags Flags = EWidgetFlags::None;
		std::string String;

		Rect rect;

		void Reparent(WidgetPtr newParent) {}

		void RemoveFromParent()
		{
			if (Parent)
				Parent->RemoveChild(this);
			Parent = nullptr;
		}

		auto Query(std::string queryStr) -> WidgetPtr;

	private:
		void RemoveChild(WidgetPtr childWidget)
		{
			auto widget = First;
			while (widget != nullptr)
			{
				if (widget.get() != childWidget)
				{
					widget = widget->Next;
					continue;
				}

				if (widget->Prev)
					widget->Prev->Next = widget->Next;

				break;
			}

			if (First.get() == childWidget)
				First = First->Next;
			if (Last.get() == childWidget)
				Last = Last->Prev;
		}
	};

	// class Screen;	??
	// class Viewport	??
	// class Context;	??

	class Viewport
	{
	public:
		auto CreateWidget() -> WidgetPtr {}

		auto Query(std::string queryStr) -> WidgetPtr;

	private:
		WidgetPtr Root = nullptr;
	};

#pragma endregion

	//////////////////////////////////////////////////////////////////////////
	/// Widgets
	//////////////////////////////////////////////////////////////////////////

#pragma region Widgets

	inline auto CreatePanel(Viewport& viewport) -> WidgetPtr
	{
		auto* widget = viewport.CreateWidget();
		widget->Flags |= EWidgetFlags::DrawBorder;
		widget->Flags |= EWidgetFlags::DrawBackground;

		return widget;
	}

	inline auto CreateButton(Viewport& viewport, const std::string& text) -> WidgetPtr
	{
		auto* widget = viewport.CreateWidget();
		widget->Flags |= EWidgetFlags::Clickable;
		widget->Flags |= EWidgetFlags::DrawText;
		widget->Flags |= EWidgetFlags::DrawBorder;
		widget->Flags |= EWidgetFlags::DrawBackground;
		widget->Flags |= EWidgetFlags::DrawDropShadow;
		widget->String = text;

		return widget;
	}

#pragma endregion

	//////////////////////////////////////////////////////////////////////////
	///
	//////////////////////////////////////////////////////////////////////////

	// Update Stages
	// 1) Auto-layout
	// 2) Consume input events
	// 3) Render frame

	/*
		Application loop
		retgui::Context ctx;
		while(true)
		{
			retgui::FrameState state;
			state.
			ctx.Tick(deltaTime, state);
			ctx.Render();
			RenderFunc(ctx.GetRenderData());
		}
	*/

} // namespace retgui