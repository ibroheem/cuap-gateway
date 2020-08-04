#ifndef CONFIG_H
#define CONFIG_H

#include <fstream>
#include <string_view>

using std::getline;
using std::fstream;
using std::string;
using std::string_view;
using std::map;
using std::vector;

namespace config
{
   enum class config_type { json, toml };

   struct url_t
   {
      string protocol, host, port, path;
   };

   struct config_t
   {
      struct app_t
      {
         string mode    = "gateway"; // single: display welcome page only, multi: talks to http backend. gateway
         uint   threads = 4;
      } app;

      struct client_t
      {
         string host, port, url;
         struct error_t
         {
            string could_not_fetch = "Your message could not be processed at this time. Please try again later. [err=could-not-fetch]",
                   invalid_data    = "Your message could not be processed at this time. Please try again later. [err=invalid-data]",
                   request_failed  = "Your message could not be processed at this time. Please try again later. [err=request-failed]",
                   could_not_represent = "Your message could not be processed at this time. Please try again later. [err=could-not-represent]";
         } error;

      } http;

      struct cuap_t
      {
         string host, system_id, password, system_type, interface_version, welcome_page;
         unsigned short  port;
         client_t client;
      };

      auto& operator[](const string& key) { return root[key]; }

      cuap_t gateway;
      map<string, cuap_t> gateways;

      template <config_type = config_type::json>
      bool parse(const string&);
      void read(string_view);

      template <config_type = config_type::json>
      void read_then_parse(string_view);
      void write(string_view data);

      string data;
      Json::Value  root;
   };

   url_t parse_url(string_view url)
   {
      url_t  url_ret;
      size_t found = url.find_first_of(":");
      url_ret.protocol = url.substr(0, found);

      string_view url_wo_protocol = url.substr(found + 3); //url_new is the url excluding the http part
      size_t found1    = url_wo_protocol.find_first_of(":");
      url_ret.host = url_wo_protocol.substr(0, found1);

      size_t found2 = url_wo_protocol.find_first_of("/");
      url_ret.port  =  url_wo_protocol.substr(found1 + 1, found2 - found1 - 1);
      url_ret.path  =  url_wo_protocol.substr(found2);

      return std::move(url_ret);
   }

   template <config_type cfg_type>
   [[maybe_unused]] bool config_t::parse(const string& text)
   {
      try
      {
         Json::Reader reader;

         bool parsed = reader.parse(text, root);
         if (!parsed)
         {
            fmt::print_red("{}. [ config::parse error ]: {}\n", misc::current_time(), reader.getFormattedErrorMessages());
            return false;
         }

         auto use_json = [&]()
         {
            app.mode    = root["app"]["mode"].asString();
            app.threads = root["app"]["threads"].asUInt();

            gateway.host = root["gateway"]["host"].asString();
            gateway.port = root["gateway"]["port"].asUInt();

            gateway.system_id    = root["gateway"]["system-id"].asString();
            gateway.password     = root["gateway"]["password"].asString();
            gateway.system_type  = root["gateway"]["system-type"].asString();

            gateway.interface_version  = root["gateway"]["interface-version"].asString();
            gateway.welcome_page       = root["gateway"]["welcome-page"].asString();

            gateway.client.url         = root["gateway"]["client"]["url"].asString();

            gateway.client.error.could_not_fetch     = root["gateway"]["client"]["error"]["could-not-fetch"].asString();
            gateway.client.error.invalid_data        = root["gateway"]["client"]["error"]["invalid-data"].asString();
            gateway.client.error.request_failed      = root["gateway"]["client"]["error"]["request-failed"].asString();
            gateway.client.error.could_not_represent = root["gateway"]["client"]["error"]["could-not-fetch"].asString();

         };

         use_json();

         return true;
      }

      catch (std::exception& e)
      {
         fmt::print_red("{}. [ config::parse exception ]: {}\n", misc::current_time(), e.what());
         return false;
      }
   }

   void config_t::read(string_view  file)
   {
      try
      {
         fstream stream_in(file.data(), fstream::in);
         string tmp;
         data.clear();
         while(getline(stream_in, tmp))
         {
            data += tmp;
         }

         if (data.empty())
         {
            fmt::print_red("{}. [ config::read error ]: Empty configuration file, aborting...\n", misc::current_time());
            exit(0);
            return ;
         }
      }
      catch (std::exception& e)
      {
         fmt::print_red("{}. [ config::read exception ]: {}\n", misc::current_time(), e.what());
      }
   }

   template <config_type cfg_ype>
   void config_t::read_then_parse(string_view  file)
   {
      fstream stream_in(file.data(), fstream::in);
      string tmp;
      data.clear();
      while(getline(stream_in, tmp))
      {
         data += tmp;
      }
      if (data.empty())
         return ;

      bool parsed = parse<cfg_ype>(data);
   }

   void config_t::write(string_view data)
   {
      fstream stream_out;
   }

}

namespace config::tests
{
   void url_parse()
   {
      constexpr string_view text = "http://qwert.mjgug.ouhnbg:5678/path1/path2.html?get=5060?you=all";
      url_t url = parse_url(text);
      fmt::print_cyan("Url: {}\n", text);
      fmt::print("Protocol: {} -> {}\nHost: {}\nPort: {}\nPath: {}\n",
         url.protocol, url.protocol == "http" ? "Success" : "Failure",
         url.host, url.port, url.path
      );
   }

   void test_read()
   {
      config_t cfg;
   }

   void config_test()
   {
      config_t cfg;
      cfg.read_then_parse<config_type::json>("config.json");

      fmt::print_green("app.mode: {}\n",      cfg.app.mode);
      fmt::print_green("app.threads: {}\n\n", cfg.app.threads);

      fmt::print_green("gateway.host:port: {}:{}\n", cfg.gateway.host, cfg.gateway.port);
      fmt::print_green("gateway.system-id: {}\n",    cfg.gateway.system_id);
      fmt::print_green("gateway.password: {}\n",     cfg.gateway.password);
      fmt::print_green("gateway.system-type: {}\n",  cfg.gateway.system_type);
      fmt::print_green("gateway.interface-version: {}\n", cfg.gateway.interface_version);
      fmt::print_green("gateway.welcome-page: {}\n\n",    cfg.gateway.welcome_page);

      fmt::print_green("gateway.client.url: {}\n",   cfg.gateway.client.url);
   }

   void main(int argc, char* argv[])
   {
      config_test(); NL
   }
}

#endif // CONFIG_H
