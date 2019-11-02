
#include "solution.h"
#include "rapidjson/prettywriter.h" // for stringify JSON
#include "visual_studio.h"
#include "source_file.h"

#include <iostream>

namespace reload
{

  namespace runtime
  {


    auto update_file = [](const auto& path, const auto& event)
    {

    };

    void solution::update_source_tree()
    {
      filesystem_watcher.update();

      std::vector<reload::source_file*> dirty_source_files;
      for (auto& i : source_files)
      {
        if (i.is_dirty)
        {
          std::cerr << i.path << " is dirty." << std::endl;

          if (i.type == source_file_types::implementation)
          {
            dirty_source_files.push_back(&i);
          }

          auto dependents = i.extract_dependents_recursive();
          for (auto& j : dependents)
          {
            if (j->type == source_file_types::implementation)
            {
              dirty_source_files.push_back(j);
            }
          }
          i.is_dirty = false;
        }
      }

      if (dirty_source_files.size())
      {
        reload::compiler.compile(dirty_source_files, "");
        std::cerr << std::endl;
      }
      else
      {
        std::string line;
        std::getline(std::cin, line);
        reload::compiler.command(line);
      }
    }

    std::vector<reload::source_file>::iterator solution::find_source_file(const std::experimental::filesystem::path& path)
    {
      for (auto i = source_files.begin(); i < source_files.end(); i++)
      {
        if ((*i).path == path)
        {
          return i;
        }
      }

      return source_files.end();
    }

    void solution::filesystem_callback(const reload::filesystem::generic_file_data& file_data, const reload::filesystem_event_types event)
    {
      auto source_itr = find_source_file(file_data.path);
      static auto renamed_source_itr = source_files.end();

      if (source_itr != source_files.end())
      {
        auto& existing_source = *source_itr;
        switch (event)
        {
          case reload::filesystem_event_types::file_renamed_from:
          {
            renamed_source_itr = source_itr;
            std::cerr << file_data.path << " renamed from." << std::endl;
            break;
          }
          case reload::filesystem_event_types::file_removed:
          {
            source_files.erase(source_itr);
            std::cerr << file_data.path << " removed." << std::endl;
            break;
          }
          case reload::filesystem_event_types::file_modified:
          {
            existing_source.is_dirty = true;
            std::cerr << file_data.path << " modified." << std::endl;
            break;
          }
        }

        return;
      }
      else
      {
        source_file new_source(std::move(file_data));
        switch (event)
        {
          case reload::filesystem_event_types::file_added:
          {
            new_source.is_dirty = true;
            source_files.push_back(std::move(new_source));
            std::cerr << file_data.path << " added." << std::endl;
            break;
          }
          case reload::filesystem_event_types::file_renamed_to:
          {
            auto& renamed_source = *renamed_source_itr;
            renamed_source.path = file_data.path;
            std::cerr << file_data.path << " renamed to." << std::endl;
            break;
          }
        }
      }
    }

    void solution::initialize_include_graph()
    {
      /*
      #if PLATFORM_WINDOWS
      using rapidjson_auto_utf = rapidjson::UTF16<>;
      #else
      using rapidjson_auto_utf = rapidjson::UTF8<>;

      #endif

      rapidjson::GenericStringBuffer<rapidjson_auto_utf> sb;
      rapidjson::PrettyWriter<decltype(sb), rapidjson_auto_utf, rapidjson_auto_utf> writer(sb);

      writer.StartArray();
      for (auto& i : std::experimental::filesystem::recursive_directory_iterator(root_path))
      {
          source_file new_source(std::move(i.path()));
          if (new_source.type != source_file_types::unknown)
          {
              new_source.update_dependencies();
              new_source.serialize(writer);
              source_files.push_back(std::move(new_source));
          }
      }
      writer.EndArray();

      std::wcerr << sb.GetString() << std::endl;
      */

    }


    // safe to use at all times

    reload::runtime::solution & solution::get()
    {
      return reload::solution;
    }

    void solution::initialize()
    {
#include <chrono>
      using cpu_clock = std::clock_t;
      cpu_clock startcputime = std::clock();

      auto source_filter = [](auto& path)
      {
        auto is_source = reload::is_source_file(path);
        //auto is_relevant = path.native().find(reload::solution().root_path / "3rdParty") == std::string::npos;
        return reload::is_source_file(path);
      };
      auto filesystem_callback = [this](const auto& file_data, const auto event)
      {
        return this->filesystem_callback(file_data, event);
      };
      filesystem_watcher.initialize(root_dir, true, source_filter, filesystem_callback);
      

      auto source_files_data = filesystem_watcher.tracked_files_data();

      for (auto& i : source_files_data)
      {
        source_files.push_back(reload::source_file(*i));
      }

      /*

      using tchar = char;// std::experimental::filesystem::path::value_type;
      using tstring = std::string; //std::experimental::filesystem::path::string_type;

      if (std::experimental::filesystem::exists(source_tree_cache_path))
      {
        std::basic_ifstream<tchar, std::char_traits<tchar> > cache_file(source_tree_cache_path);
        if (cache_file.is_open())
        {
          cache_file.seekg(0, cache_file.end);
          int length = cache_file.tellg();
          cache_file.seekg(0, cache_file.beg);

          tstring content;
          content.resize(length);
          cache_file.read(&content.front(), length);

          // rapidjson process

          cache_file.close();
        }
      }
      else
      {
        for (auto& i : source_files)
        {
          i.extract_dependencies();
        }

        for (auto& i : source_files)
        {
          for (auto& j : source_files)
          {
            if (j.depends_on(i))
            {
              i.dependents.push_back(&j);
            }
          }
        }

        tstring content;
        // serialize

        // write to json
        source_tree_cache_path;
      }

      */


      double cpu_duration = (std::clock() - startcputime) / (double)CLOCKS_PER_SEC;
      std::cout << "Finished in " << cpu_duration << " seconds [CPU Clock] " << std::endl;

    }
}

  reload::runtime::solution solution;
}
