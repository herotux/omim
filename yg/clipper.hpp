#pragma once

#include "../geometry/rect2d.hpp"
#include "renderer.hpp"

namespace yg
{
  namespace gl
  {
    class Clipper : public Renderer
    {
    private:

      typedef Renderer base_t;

      bool m_isClippingEnabled;
      m2::RectI m_clipRect;

    public:

      Clipper();

      void beginFrame();
      void endFrame();

      /// Working with clip rect
      /// @{
      /// enabling/disabling and querying clipping state
      void enableClipRect(bool flag);
      bool clipRectEnabled() const;

      /// Setting and querying clipRect
      void setClipRect(m2::RectI const & rect);
      m2::RectI const & clipRect() const;
      /// @}

    };
  }
}
