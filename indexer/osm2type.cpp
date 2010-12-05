#include "osm2type.hpp"
#include "classificator.hpp"
#include "drawing_rules.hpp"
#include "feature_visibility.hpp"
#include "xml_element.hpp"

#include "../coding/parse_xml.hpp"
#include "../coding/file_reader.hpp"

#include "../base/assert.hpp"
#include "../base/string_utils.hpp"

#include "../std/fstream.hpp"
#include "../std/bind.hpp"
#include "../std/vector.hpp"
#include "../std/set.hpp"
#include "../std/algorithm.hpp"

#include "../base/start_mem_debug.hpp"

namespace ftype {

  namespace
  {
    /// get value of mark (1 == "yes", -1 == "no", 0 == not a "yes\no")
    static int get_mark_value(string const & k, string const & v)
    {
      static char const * aTrue[] = { "yes", "true", "1", "*" };
      static char const * aFalse[] = { "no", "false", "-1" };

      utils::TokenizeIterator it(v, "|");
      while (!it.end())
      {
        if (utils::IsInArray(aTrue, *it)) return 1;
        if (utils::IsInArray(aFalse, *it)) return -1;
        ++it;
      }

      // "~" means no this tag, so sometimes it means true,
      // and all other cases - false. Choose according to key.
      if (v == "~")
        return (k == "access" ? 1 : -1);

      return 0;
    }

    class OSMTypesStream
    {
      /// @name processing elements definitions
      //@{
      struct element_t
      {
        element_t() : pObj(0) {}

        string name;
        map<string, string> attr;

        ClassifObject * pObj;
      };

      vector<element_t> m_elements;
      element_t & current() { return m_elements.back(); }

      int m_priority;
      //@}

      /// check if element is a draw rule (commonly it's a leaf in xml)
      static bool is_draw_rule(string const & e)
      {
        static char const * rules[] = { "line", "tunnel", "area", "symbol", "caption", "text",
                                        "circle", "pathText", "wayMarker" };
        return utils::IsInArray(rules, e);
      }

      uint8_t get_rule_type()
      {
        int count = static_cast<int>(m_elements.size()) - 2;
        ASSERT ( count >= 0, (count) );

        string e;
        while (e.empty() && count >= 0)
        {
          e = m_elements[count].attr["e"];
          --count;
        }
        ASSERT ( !e.empty(), () );

        utils::TokenizeIterator it(e, "|");
        uint8_t ret = 0;
        while (!it.end())
        {
          string const & s = *it;
          if (s == "node")
            ret |= drule::node;
          else if (s == "way")
            ret |= drule::way;
          ++it;
        }

        ASSERT ( ret != 0, () );
        return static_cast<drule::rule_geo_t>(ret);
      }

      /// check if it's our element to parse
      static bool is_our_element(string const & e)
      {
        static char const * elems[] = { "rules", "rule", "else", "layer",
          // addclass appear in small scales (6-11)
          // don't skip it during parsing, but we don't process it like a rule
                                        "addclass" };
        return (utils::IsInArray(elems, e) || is_draw_rule(e));
      }

      /// check if it's processing key
      static bool is_valid_key(string const & k)
      {
        static char const * bad[] = { "osmarender:render", "osmarender:rendername",
                                      "osmarender:renderref", "addr:housenumber" };
        return (!k.empty() && !utils::IsInArray(bad, k));
      }

      static bool is_valid_value(string const & v)
      {
        return !v.empty();
      }

      /// check if key is a 'mark'
      static bool is_mark_key(string const & k)
      {
        static char const * mark[] = {  "bridge", "tunnel", "area", "lock", "oneway", "junction",
                                        "embankment", "cutting", "motorroad", "cycleway",
                                        "bicycle", "horse", "capital", "fee" };
        return utils::IsInArray(mark, k);
      }

      static bool process_feature_like_mark_from_root(string const & /*k*/, string const & v)
      {
        static char const * mark[] = { "turning_circle", "dyke", "dike", "levee", "embankment" };
        return utils::IsInArray(mark, v);
      }

      static bool process_feature_like_mark(string const & k, string const & v)
      {
        return (k == "highway" && (v == "construction" || v == "disused"));
      }

      /// check if skip whole element by it's key
      static bool is_skip_element_by_key(string const & k)
      {
        static char const * skip[] = { "addr:housenumber", "fixme" };
        return utils::IsInArray(skip, k);
      }

      /// skip element and all it's sub-elements
      bool m_forceSkip;

    public:
      OSMTypesStream() : m_priority(0), m_forceSkip(false) {}

      bool Push(string const & name)
      {
        if (!m_forceSkip && is_our_element(name))
        {
          m_elements.push_back(element_t());
          current().name = name;
          return true;
        }

        return false;
      }

    public:
      void AddAttr(string name, string value)
      {
        // make lower case for equivalent string comparison
        utils::make_lower_case(name);
        utils::make_lower_case(value);

        if ((name == "k") && is_skip_element_by_key(value))
          m_forceSkip = true;
        else
          current().attr[name] = value;
      }

      ClassifObject * get_root() { return classif().GetMutableRoot(); }

      void Pop(string const & /*element*/)
      {
        if (!m_forceSkip)
          add_type_recursive(get_root(), 0, vector<string>());
        else
          m_forceSkip = false;

        m_elements.pop_back();
      }

    private:
      vector<string> make_concat(vector<string> const & v, int intV, string const & s)
      {
        if (intV == 1)
        {
          vector<string> vv;
          vv.reserve(v.size() + 1);
          bool inserted = false;
          for (size_t i = 0; i < v.size(); ++i)
          {
            if (!(v[i] < s) && !inserted)
            {
              inserted = true;
              vv.push_back(s);
            }
            vv.push_back(v[i]);
          }
          if (!inserted) vv.push_back(s);

          return vv;
        }
        else return v;
      }

      /// get parent of object (p) in created chain of elements
      ClassifObject * get_parent_of(size_t i, ClassifObject * p)
      {
        ASSERT ( i > 0, () );
        while (--i > 0)
          if (m_elements[i].pObj == p) break;

        ASSERT ( i > 0, () );
        while (--i > 0)
          if (m_elements[i].pObj)
            return m_elements[i].pObj;

        return get_root();
      }

      void clear_states(size_t start)
      {
        for (size_t i = start; i < m_elements.size(); ++i)
          m_elements[i].pObj = 0;
      }

      void add_type_recursive(ClassifObject * pParent,
                              size_t start,
                              std::vector<string> const & marks)
      {
        for (size_t i = start; i < m_elements.size(); ++i)
        {
          element_t & e = m_elements[i];

          if (e.pObj) continue;

          if (e.name == "rule")
          {
            // process rule
            string k = e.attr["k"];
            if (!is_valid_key(k)) continue;

            string v = e.attr["v"];
            if (!is_valid_value(v)) continue;

            utils::TokenizeIterator iK(k, "|");
            if (iK.is_last())
            {
              // process one key
              ASSERT ( *iK == k, () );

              int intV = get_mark_value(k, v);
              if (is_mark_key(k) && (intV != 0))
              {
                // key is a mark, so save it and go futher
                add_type_recursive(pParent, i + 1, make_concat(marks, intV, k));
                clear_states(i);
              }
              else
              {
                // buildings assume as feature type
                bool lets_try = (k == "building" && intV == 1);

                // default access is yes. If "no" - make additional feature type
                if (!lets_try && (k == "access" && intV == -1))
                {
                  lets_try = true;
                  intV = 0;
                  v = "no-access";
                }

                if (!lets_try && intV != 0)
                {
                  // skip this keys, because they are dummy
                  continue;
                }
                else
                {
                  // add root or criterion
                  if (pParent == get_root())
                  {
                    pParent = pParent->Add(k);
                    e.pObj = pParent;

                    // use m_elements[1] to hold first parent of futher creation objects
                    // need for correct working "get_parent_of" function
                    m_elements[1].pObj = pParent;
                  }
                  else
                  {
                    // avoid recursion like this:
                    // <k = "x", v = "a|b|c">
                    //     <k = "x", v = "a">
                    //     <k = "x", v = "b">
                    //     <k = "x", v = "c">
                    ClassifObject * ppParent = get_parent_of(i, pParent);
                    if (k != ppParent->GetName())
                    {
                      // do not set criterion like base object
                      if (k != pParent->GetName() &&
                          !process_feature_like_mark(pParent->GetName(), k))
                        pParent->AddCriterion(k);
                    }
                    else
                      pParent = ppParent;
                  }

                  // process values
                  utils::TokenizeIterator iV(v, "|");
                  while (!iV.end())
                  {
                    bool const b1 = process_feature_like_mark_from_root(k, *iV);
                    if (b1 || process_feature_like_mark(k, *iV))
                    {
                      // process value like mark, so save it and go futher
                      add_type_recursive(
                          b1 ? get_root() : pParent, i + 1, make_concat(marks, 1, *iV));
                      clear_states(i);
                    }
                    else
                    {
                      ClassifObject * p = pParent;
                      if (intV == 0)
                        p = pParent->Add(*iV);
                      e.pObj = p;

                      add_type_recursive(p, i + 1, marks);
                      clear_states(i);
                    }

                    ++iV;
                  }
                }
              }
            }
            else
            {
              char const * aTry[] = { "natural", "landuse" };

              while (!iK.end())
              {
                // let's try to add root keys
                bool addMode = (pParent == get_root() && utils::IsInArray(aTry, *iK));

                ClassifObject * p = (addMode ? pParent->Add(*iK) : pParent->Find(*iK));
                if (p && (get_mark_value(*iK, v) == 0))
                {
                  if (p->IsCriterion()) p = pParent;

                  utils::TokenizeIterator iV(v, "|");
                  while (!iV.end())
                  {
                    ClassifObject * pp = (addMode ? p->Add(*iV) : p->Find(*iV));
                    if (pp)
                    {
                      e.pObj = pp;

                      add_type_recursive(pp, i + 1, marks);
                      clear_states(i);
                    }
                    ++iV;
                  }
                }
                ++iK;
              }
            }

            return; // processed to the end - exit
          }
          else if (is_draw_rule(e.name))
          {
            ASSERT ( i == m_elements.size()-1, ("drawing rules should be leavs") );

            // process draw rule
            if (pParent != get_root())
            {
              if (!marks.empty())
              {
                // make final mark string
                string res;
                for (size_t i = 0; i < marks.size(); ++i)
                {
                  if (!res.empty()) res += '-';
                  res += marks[i];
                }

                pParent = pParent->Add(res);
              }

              vector<drule::Key> keys;
              drule::rules().CreateRules(e.name, get_rule_type(), e.attr, keys);

              // if no "layer" tag, then atoi returns 0 - it's ok for us
              // 1000 - is a base count of rules for layer
              int const layer = atoi(e.attr["layer"].c_str()) * drule::layer_base_priority;
              for (size_t i = 0; i < keys.size(); ++i)
                keys[i].SetPriority(layer + m_priority++);

              for_each(keys.begin(), keys.end(), bind(&ClassifObject::AddDrawRule, pParent, _1));
            }
          }
        }
      }
    };
  }

  void ParseOSMTypes(char const * fPath, int scale)
  {
    drule::rules().SetParseFile(fPath, scale);

    FileReader reader(fPath);
    ReaderSource<FileReader> source(reader);
    OSMTypesStream stream;
    ParseXML(source, stream);
  }

  namespace
  {
    bool is_skip_tag(string const & k, string const & /*v*/)
    {
      // skip "cycleway's" tags because they interfer to set a valid types like "highway's"
      return (k == "created_by" || k == "description" || k == "cycleway" || k == "embankment");
    }

    template <class ToDo> typename ToDo::result_type for_each_tag(XMLElement * p, ToDo toDo)
    {
      typedef typename ToDo::result_type res_t;

      for (size_t i = 0; i < p->childs.size(); ++i)
      {
        if (p->childs[i].name == "tag")
        {
          string const & k = p->childs[i].attrs["k"];
          string const & v = p->childs[i].attrs["v"];

          if (is_skip_tag(k, v)) continue;

          // this means "no"
          //if (get_mark_value(k, v) == -1)
          //  continue;

          res_t res = toDo(k, v);
          if (res) return res;
        }
      }
      return res_t();
    }

    bool is_name_tag(string const & k)
    {
      return (string::npos != k.find("name"));
    }

    class do_print
    {
      ostream & m_s;
    public:
      typedef bool result_type;

      do_print(ostream & s) : m_s(s) {}
      bool operator() (string const & k, string const & v) const
      {
        m_s << k << " <---> " << v << endl;
        return false;
      }
    };

    class do_find_name
    {
      size_t & m_count;
      string & m_name;
      int32_t & m_layer;
    public:
      typedef bool result_type;

      do_find_name(size_t & count, string & name, int32_t & layer)
        : m_count(count), m_name(name), m_layer(layer)
      {
        m_count = 0;
        m_layer = 0;
      }
      bool operator() (string const & k, string const & v)
      {
        ++m_count;

        // do not call is_name_tag(k), but exactly "name" tag
        if (k == "name" && m_name.empty())
          m_name = v;

        if (k == "layer" && m_layer == 0)
          m_layer = atoi(v.c_str());

        return false;
      }
    };

    class do_find_obj
    {
      ClassifObject const * m_parent;
      bool m_isKey;

    public:
      typedef ClassifObjectPtr result_type;

      do_find_obj(ClassifObject const * p, bool isKey) : m_parent(p), m_isKey(isKey) {}
      ClassifObjectPtr operator() (string const & k, string const & v) const
      {
        if (!is_name_tag(k))
        {
          ClassifObjectPtr p = m_parent->BinaryFind(m_isKey ? k : v);
          if (p) return p;
        }
        return ClassifObjectPtr(0, 0);
      }
    };

    class do_find_root_obj : public do_find_obj
    {
      typedef do_find_obj base_type;

      set<string> const & m_skipTags;

    public:
      do_find_root_obj(set<string> const & skipTags)
        : base_type(classif().GetRoot(), true), m_skipTags(skipTags)
      {
      }
      ClassifObjectPtr operator() (string const & k, string const & v) const
      {
        if (m_skipTags.find(k) == m_skipTags.end())
          return base_type::operator() (k, v);

        return ClassifObjectPtr(0, 0);
      }
    };

    typedef vector<ClassifObjectPtr> path_type;
  }

  ClassifObjectPtr find_object(ClassifObject const * parent, XMLElement * p, bool isKey)
  {
    return for_each_tag(p, do_find_obj(parent, isKey));
  }

  size_t find_name_and_count(XMLElement * p, string & name, int32_t & layer)
  {
    size_t count;
    for_each_tag(p, do_find_name(count, name, layer));
    return count;
  }

//#ifdef DEBUG
//  class debug_find_string
//  {
//    string m_comp;
//  public:
//    debug_find_string(string const & comp) : m_comp(comp) {}
//    typedef bool result_type;
//    bool operator() (string const & k, string const & v) const
//    {
//      return (k == m_comp || v == m_comp);
//    }
//  };
//#endif

  bool GetNameAndType(XMLElement * p, vector<uint32_t> & types, string & name, int32_t & layer)
  {
//#ifdef DEBUG
//    // code to set a breakpoint
//    if (for_each_tag(p, debug_find_string("bridge")))
//    {
//      int break_here = 0;
//    }
//#endif

    // maybe an empty feature
    if (find_name_and_count(p, name, layer) == 0)
      return false;

    set<string> skipRootKeys;

    do
    {
      path_type path;

      // find first root object by key
      do_find_root_obj doFindRoot(skipRootKeys);
      ClassifObjectPtr pRoot = for_each_tag(p, doFindRoot);

      // find path from root
      ClassifObjectPtr pObj = pRoot;
      while (pObj)
      {
        path.push_back(pObj);

        // next objects trying to find by value first
        pObj = find_object(path.back().get(), p, false);
        if (!pObj)
        {
          // if no - try find object by key (in case of k = "area", v = "yes")
          pObj = find_object(path.back().get(), p, true);
        }
      }

      size_t const count = path.size();
      if (count >= 1)
      {
        // assign type
        uint32_t t = ftype::GetEmptyValue();

        for (size_t i = 0; i < count; ++i)
          ftype::PushValue(t, path[i].GetIndex());

        // use features only with drawing rules
        if (feature::IsDrawableAny(t))
          types.push_back(t);
      }

      if (pRoot)
      {
        // save this root to skip, and try again
        skipRootKeys.insert(pRoot->GetName());
      }
      else
        break;

    } while (true);

    return !types.empty();
  }
}
