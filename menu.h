#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <fstream>
#include <vector>
#include <map>
#include <pugi/pugixml.hpp>

#include "mib/io/fmt-5"
#include "mib/typedefs"

using std::string;
using std::string_view;

using namespace pugi;

namespace ussd_menu
{
   enum class parse_mode { string, file };

   xml_document parse(string_view file)
   {
      xml_document doc;
      xml_parse_result rs = doc.load_file(file.data());
      if (!rs)
      {
         fmt::print_red("{} at {}\n", rs.description(), rs.offset);
         exit(-1);
      }
      return doc;
   }

   template <parse_mode mode = parse_mode::file>
   bool parse(xml_document& doc, string_view file)
   {
      xml_parse_result rs;
      if constexpr (mode == parse_mode::file)
      {
         rs = doc.load_file(file.data());
      }
      else if constexpr (mode == parse_mode::string)
      {
         rs = doc.load_string(file.data());
      }

      if (!rs)
      {
         fmt::print_red("{} at {}\n", rs.description(), rs.offset);
         return false;
      }
      return true;
   }

   bool parse(string_view data, Json::Value& root)
   {
      try
      {
         Json::CharReaderBuilder rbuilder;
         rbuilder["collectComments"] = false;
         std::unique_ptr<Json::CharReader> const reader(rbuilder.newCharReader());
         std::string errs;
         auto begin = data.data();
         auto end   = begin + data.size();

         bool ok = reader->parse(begin, end, &root, &errs);
         if (!ok)
         {
            fmt::print_red("{}. [ menu::parse error ]: {}\n{}\n", misc::time::now(), errs, data);
            return ok;
         }
         else
         {
            fmt::print_green("{}. [ menu::parse info ]: Parsed.\n", misc::time::now());
            return ok;
         }
      }
      catch(std::exception& e)
      {
         fmt::print_red("{}. [ menu::parse exception ]: {}\n", misc::time::now(), e.what());
      }
      return false;
   }


   struct type_t
   {
      constexpr static string_view input   = "input";
      constexpr static string_view display = "display";
   };

   struct menu_t
   {
      enum class menu_type_t { main, submenu };
      enum class dialog_t    { display, input };

      string   selection, value, caption, body;
      xml_node prev, current;
      static   xml_document doc;
      dialog_t dialog;

      menu_type_t menu_type;

      menu_t() { }
      menu_t(parse_mode mode, string_view file)
      {
         bool p;
         switch (mode)
         {
            case parse_mode::file:
            {
               p = parse<parse_mode::file>(doc, file.data());
            } break;

            case parse_mode::string:
            {
               p = parse<parse_mode::string>(doc, file.data());
            } break;
         }
         if (!p)
         {
            exit(-1);
         }
      }

      void init(string_view file = "xml/ussd-menu.xml")
      {
         bool p = parse(doc, file.data());
         if (!p)
         {
            exit(-1);
         }
      }

      string back()
      {
         xml_node menu = current.child("menu");
         string data, menutype = menu.attribute("type").value();
         caption = menu.child("caption").child_value();
         for (xml_node option : menu.children("option"))
         {
            xml_node text = option.child("text");
            if (!text.empty())
            {
               body += fmt::format("{}. {}\n", option.attribute("id").value(), text.child_value());
            }
         }
         data = fmt::format("{}\n{}\n:", caption, body);
         return data;
      }

      bool has_menu()
      {
         return !current.child("menu").empty();
      }

      bool process_mainmenu()
      {
         for (xml_node child : current)
         {
            if (selection == child.attribute("expects").value())
            {
               if (menu_type == menu_type_t::main)
                  menu_type = menu_type_t::submenu;

               current = child;
               body.clear();
               caption.clear();
#ifdef MENU_DEBUG_ENABLE
               fmt::print("[process_main]: selection: {}, name: {}, meta: {}\n", selection, current.name(), current.attribute("meta").value());
#endif // MENU_DEBUG_ENABLE
               return true;
            }
         }
         return false;
      }

      bool process_menu()
      {
         static string_view prev_menu = "prev";

         xml_node mn = current.child("menu");
#ifdef MENU_DEBUG_ENABLE
         fmt::print_green("[ process::current ]: selection: {}, name: {}, meta: {}, child-value: {}\n",
            selection, current.name(), current.attribute("meta").value(), current.child("option").child_value()
         );
#endif
         for (xml_node child : mn.children("option"))
         {
            if (selection == child.attribute("expects").value())
            {
               if (menu_type == menu_type_t::main)
                  menu_type = menu_type_t::submenu;

               if (prev_menu == child.attribute("action").value()) //if user wants to go back, make current as the parent page
               {
                  current = current.parent().parent();
               }
               else
               {
                  prev    = current;
                  current = child;
               }
               body.clear();
               caption.clear();
               value = child.attribute("value").value();
#ifdef MENU_DEBUG_ENABLE
               fmt::print_cyan("[ process::child ]: selection: {}, name: {}, meta: {}, child-value: {}, has menu: {}\n",
                  selection, child.name(), child.attribute("meta").value(), value, has_menu()
               );
#endif
               return true;
            }
         }
         return false;
      }

      bool process()
      {
         if (menu_type == menu_type_t::main)
         {
            return process_mainmenu();
         }
         else
         {
            return process_menu();
         }
      }

      bool process(const std::string& input)
      {
         selection = input;
         return process();
      }

      string render_mainmenu()
      {
         string data;
         if (menu_type == menu_type_t::main)
         {
            current = doc.child("menu");
            caption = current.child("caption").child_value();

            for (xml_node option : current.children("option"))
            {
               xml_node text = option.child("text");
               if (!text.empty())
               {
                  body += fmt::format("{}. {}\n", option.attribute("id").value(), text.child_value());
               }
            }
            data = fmt::format("{}\n{}", caption, body);
            return data;
         }
         if (data.empty())
         {
            data = fmt::format("{}\n{}", "Invalid Option.", body);
         }
         return data;
      }

      string render_menu()
      {
         xml_node menu = current.child("menu");
         string data, menutype = menu.attribute("type").value();
         caption = menu.child("caption").child_value();
         for (xml_node option : menu.children("option"))
         {
            xml_node text = option.child("text");
            if (!text.empty())
            {
               if (type_t::display == option.attribute("type").value())
               {
                  dialog =  dialog_t::display;
                  body   += fmt::format("{}\n", text.child_value());
                  //body   += option.attribute("value").value();
               }
               else
                  body += fmt::format("{}. {}\n", option.attribute("id").value(), text.child_value());
            }
         }
         data = fmt::format("{}\n{}", caption, body);
         return data;
      }

      // Returns the <option> node
      xml_node selected_node()
      {
         return current;
      }

      string render()
      {
         if (menu_type == menu_type_t::main)
         {
            return render_mainmenu();
         }
         else
         {
            return render_menu();
         }
      }

      string render_err()
      {
         return fmt::format("{}\n{}", "Invalid Option.", body);
      }

   };

   struct value_menu_t
   {
      enum class menu_type_t { main, submenu, unknown };
      enum class dialog_t    { display, input };

      string   selection, value, caption, body;
      xml_node prev, current;
      xml_document doc;
      dialog_t dialog;

      menu_type_t menu_type = menu_type_t::unknown; // Menu not init, doc isn't parsed into

      value_menu_t() { }
      value_menu_t(parse_mode mode, string_view file)
      {
         init(mode, file);
      }

      value_menu_t(value_menu_t& rhs) : selection(rhs.selection), value(rhs.value), caption(rhs.caption), body(rhs.body), dialog(rhs.dialog),
                            menu_type(rhs.menu_type), prev(rhs.prev), current(rhs.current), doc(std::move(rhs.doc))
      {

      }

      value_menu_t(value_menu_t&& rhs) : selection(rhs.selection), value(rhs.value), caption(rhs.caption), body(rhs.body), dialog(rhs.dialog),
                             menu_type(rhs.menu_type), prev(rhs.prev), current(rhs.current), doc(std::move(rhs.doc))
      {

      }

      bool init(string_view file)
      {
         return parse(doc, file.data());
      }

      bool init(parse_mode mode, string_view file)
      {
         bool p;
         switch (mode)
         {
            case parse_mode::file:
            {
               p = parse<parse_mode::file>(doc, file.data());
            } break;

            case parse_mode::string:
            {
               p = parse<parse_mode::string>(doc, file.data());
            } break;
         }
         return p;
      }

      string back()
      {
         xml_node menu = current.child("menu");
         string data, menutype = menu.attribute("type").value();
         caption = menu.child("caption").child_value();
         for (xml_node option : menu.children("option"))
         {
            xml_node text = option.child("text");
            if (!text.empty())
            {
               body += fmt::format("{}. {}\n", option.attribute("id").value(), text.child_value());
            }
         }
         data = fmt::format("{}\n{}\n:", caption, body);
         return data;
      }

      bool has_menu()
      {
         return !current.child("menu").empty();
      }

      bool process_mainmenu()
      {
         for (xml_node child : current)
         {
            if (selection == child.attribute("expects").value())
            {
               if (menu_type == menu_type_t::main)
                  menu_type = menu_type_t::submenu;

               current = child;
               body.clear();
               caption.clear();
#ifdef MENU_DEBUG_ENABLE
               fmt::print("[process_main]: selection: {}, name: {}, meta: {}\n", selection, current.name(), current.attribute("meta").value());
#endif // MENU_DEBUG_ENABLE
               return true;
            }
         }
         return false;
      }

      bool process_menu()
      {
         static string_view prev_menu = "prev";

         xml_node mn = current.child("menu");
#ifdef MENU_DEBUG_ENABLE
         fmt::print_green("[ process::current ]: selection: {}, name: {}, meta: {}, child-value: {}\n",
            selection, current.name(), current.attribute("meta").value(), current.child("option").child_value()
         );
#endif
         for (xml_node child : mn.children("option"))
         {
            if (selection == child.attribute("expects").value())
            {
               if (menu_type == menu_type_t::main)
                  menu_type = menu_type_t::submenu;

               if (prev_menu == child.attribute("action").value()) //if user wants to go back, make current as the parent page
               {
                  current = current.parent().parent();
               }
               else
               {
                  prev    = current;
                  current = child;
               }
               body.clear();
               caption.clear();
               value = child.attribute("value").value();
#ifdef MENU_DEBUG_ENABLE
               fmt::print_cyan("[ process::child ]: selection: {}, name: {}, meta: {}, child-value: {}, has menu: {}\n",
                  selection, child.name(), child.attribute("meta").value(), value, has_menu()
               );
#endif
               return true;
            }
         }
         return false;
      }

      bool process()
      {
         if (menu_type == menu_type_t::main)
         {
            return process_mainmenu();
         }
         else
         {
            return process_menu();
         }
      }

      bool process(const std::string& input)
      {
         selection = input;
         return process();
      }

      string render_mainmenu()
      {
         string data;
         if (menu_type == menu_type_t::main)
         {
            current = doc.child("menu");
            caption = current.child("caption").child_value();

            for (xml_node option : current.children("option"))
            {
               xml_node text = option.child("text");
               if (!text.empty())
               {
                  body += fmt::format("{}. {}\n", option.attribute("id").value(), text.child_value());
               }
            }
            data = fmt::format("{}\n{}", caption, body);
            return data;
         }
         if (data.empty())
         {
            data = fmt::format("{}\n{}", "Invalid Option.", body);
         }
         return data;
      }

      string render_menu()
      {
         xml_node menu = current.child("menu");
         string data, menutype = menu.attribute("type").value();
         caption = menu.child("caption").child_value();
         for (xml_node option : menu.children("option"))
         {
            xml_node text = option.child("text");
            if (!text.empty())
            {
               if (type_t::display == option.attribute("type").value())
               {
                  dialog =  dialog_t::display;
                  body   += fmt::format("{}\n", text.child_value());
                  //body   += option.attribute("value").value();
               }
               else
                  body += fmt::format("{}. {}\n", option.attribute("id").value(), text.child_value());
            }
         }
         data = fmt::format("{}\n{}", caption, body);
         return data;
      }

      // Returns the <option> node
      xml_node selected_node()
      {
         return current;
      }

      string render()
      {
         if (menu_type == menu_type_t::main)
         {
            return render_mainmenu();
         }
         else
         {
            return render_menu();
         }
      }

      string render_err()
      {
         return fmt::format("{}\n{}", "Invalid Option.", body);
      }

   };

   xml_document menu_t::doc;

   string to_xml_text(string_view text)
   {
      string data;

      return data;
   }
}
