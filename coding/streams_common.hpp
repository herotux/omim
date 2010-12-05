#pragma once

#include "../base/base.hpp"
#include "../base/assert.hpp"

#include "../std/string.hpp"

namespace stream
{
  namespace detail
  {
    template <class TStream>
    void ReadString(TStream & s, string & t)
    {
      uint32_t count;
      s >> count;
      t.reserve(count);

      while (count > 0)
      {
        char c;
        s >> c;
        t.push_back(c);
        --count;
      }
    }

    template <class TStream, class TWriter>
    void WriteString(TStream & s, TWriter & w, string const & t)
    {
      uint32_t const count = t.size();
      s << count;
      if (count > 0)
        w.Write(t.c_str(), count);
    }

    template <class TStream>
    void ReadBool(TStream & s, bool & t)
    {
      char tt;
      s >> tt;
      ASSERT(tt == 0 || tt == 1, (tt));
      t = (tt != 0);
    }

    template <class TStream>
    void WriteBool(TStream & s, bool t)
    {
      s << char(t ? 1 : 0);
    }
  }
}
