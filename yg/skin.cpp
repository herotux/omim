#include "skin.hpp"

#include "texture.hpp"
#include "skin_page.hpp"
#include "resource_style.hpp"
#include "resource_manager.hpp"

#include "../platform/platform.hpp"
#include "../base/logging.hpp"
#include "../base/profiler.hpp"
#include "../base/ptr_utils.hpp"

#include "../std/iterator.hpp"
#include "../std/bind.hpp"
#include "../std/numeric.hpp"

#include "../base/start_mem_debug.hpp"
#include "internal/opengl.hpp"

namespace yg
{
  Skin::Skin(shared_ptr<ResourceManager> const & resourceManager,
             Skin::TSkinPages const & pages)
    : m_pages(pages),
      m_resourceManager(resourceManager)
  {
    m_startDynamicPage = m_pages.size();
    m_currentDynamicPage = m_pages.size();
    /// adding at least two for proper double buffering,
    /// although even 1 will work.
    addDynamicPages(3);
  }

  void Skin::addDynamicPages(int count)
  {
    m_pages.reserve(m_pages.size() + count);

    addClearPageFn(bind(&Skin::clearPageHandles, this, _1), 0);

    for (size_t i = 0; i < count; ++i)
    {
      uint8_t pageID = (uint8_t)m_pages.size();
      m_pages.push_back(make_shared_ptr(new SkinPage(m_resourceManager, pageID)));
      m_pages.back()->addOverflowFn(bind(&Skin::onOverflow, this, pageID), 0);
    }
  }

  Skin::~Skin()
  {}

  pair<uint8_t, uint32_t> Skin::unpackID(uint32_t id) const
  {
    uint8_t pageID = (id & 0xFF000000) >> 24;
    uint32_t h = (id & 0x00FFFFFF);
    return make_pair<uint8_t, uint32_t>(pageID, h);
  }

  uint32_t Skin::packID(uint8_t pageID, uint32_t handle) const
  {
    uint32_t pageIDMask = (uint32_t)pageID << 24;
    uint32_t h = (handle & 0x00FFFFFF);
    return (uint32_t)(pageIDMask | h);
  }

  ResourceStyle const * Skin::fromID(uint32_t id)
  {
    id_pair_t p = unpackID(id);
    return m_pages[p.first]->fromID(p.second);
  }

  uint32_t Skin::mapSymbol(char const * symbolName)
  {
    for (uint8_t i = 0; i < m_pages.size(); ++i)
    {
      uint32_t res = m_pages[i]->find(symbolName);
      if (res != invalidPageHandle())
        return packID(i, res);
    }
    return invalidHandle();
  }

  uint32_t Skin::mapColor(Color const & c)
  {
    uint32_t res = invalidPageHandle();

    for (uint8_t i = 0; i < m_pages.size(); ++i)
    {
      res = m_pages[i]->find(c);
      if (res != invalidPageHandle())
        return packID(i, res);
    }

    if (!m_pages[m_currentDynamicPage]->hasRoom(c))
      changeCurrentDynamicPage();

    return packID(m_currentDynamicPage, m_pages[m_currentDynamicPage]->Map(c));
  }

  uint32_t Skin::mapPenInfo(PenInfo const & penInfo)
  {
    uint32_t res = invalidPageHandle();
    for (uint8_t i = 0; i < m_pages.size(); ++i)
    {
      res = m_pages[i]->find(penInfo);
      if (res != invalidPageHandle())
        return packID(i, res);
    }

    if (!m_pages[m_currentDynamicPage]->hasRoom(penInfo))
      changeCurrentDynamicPage();

    return packID(m_currentDynamicPage, m_pages[m_currentDynamicPage]->Map(penInfo));
  }

  FontInfo const & Skin::getFont(int size) const
  {
    int lastFont = -1;

    for (uint8_t i = 0; i < m_pages.size(); ++i)
    {
      if (m_pages[i]->fonts().size() != 0)
      {
        if (lastFont == -1)
          lastFont = i;
        if (size >= m_pages[i]->fonts()[0].m_fontSize)
          lastFont = i;
        else
          return m_pages[lastFont]->fonts()[0];
      }
    }

    return m_pages[lastFont]->fonts()[0];
  }

  Skin::TSkinPages const & Skin::pages() const
  {
    return m_pages;
  }

  void Skin::addClearPageFn(clearPageFn fn, int priority)
  {
    m_clearPageFns.push(std::pair<size_t, clearPageFn>(priority, fn));
  }

  void Skin::callClearPageFns(uint8_t pageID)
  {
    clearPageFns handlersCopy = m_clearPageFns;
    while (!handlersCopy.empty())
    {
      handlersCopy.top().second(pageID);
      handlersCopy.pop();
    }
  }

  void Skin::addOverflowFn(overflowFn fn, int priority)
  {
    m_overflowFns.push(std::pair<size_t, overflowFn>(priority, fn));
  }

  void Skin::callOverflowFns(uint8_t pageID)
  {
    overflowFns handlersCopy = m_overflowFns;
    while (!handlersCopy.empty())
    {
      handlersCopy.top().second(pageID);
      handlersCopy.pop();
    }
  }

  void Skin::clearPageHandles(uint8_t pageID)
  {
    m_pages[pageID]->clearHandles();
  }

  /// Called from the skin page on handles overflow.
  /// Never called on texture overflow, as this situation
  /// are explicitly checked in the mapXXX() functions.
  void Skin::onOverflow(uint8_t pageID)
  {
    LOG(LINFO, ("Called from SkinPage as a reaction on handles overflow for pageID=", (uint32_t)pageID));
    changeCurrentDynamicPage();
  }

  void Skin::changeCurrentDynamicPage()
  {
    /// 1. flush screen(through overflowFns)
    callOverflowFns(m_currentDynamicPage);
    /// page should be frozen after flushing(activeCommands > 0)

    /// 2. choose next dynamic page
    if (m_currentDynamicPage == m_pages.size() - 1)
      m_currentDynamicPage = m_startDynamicPage;
    else
      ++m_currentDynamicPage;

    /// 3. clear it if necessary and notify observers about this event(through clearPageFns)
    callClearPageFns(m_currentDynamicPage);
  }

  uint32_t Skin::invalidHandle() const
  {
    return 0xFFFFFFFF;
  }

  uint32_t Skin::invalidPageHandle() const
  {
    return 0x00FFFFFF;
  }
}
