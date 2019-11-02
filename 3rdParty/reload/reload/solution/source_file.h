#pragma once

#include <regex>
#include <filesystem>
#include "generic_file_data.h"

namespace reload
{
    bool path_ends_with(const std::experimental::filesystem::path::string_type& lhs, const std::experimental::filesystem::path::string_type& rhs);
    bool is_implementation_file(const std::experimental::filesystem::path& path);
    bool is_declaration_file(const std::experimental::filesystem::path& path);;
    bool is_source_file(const std::experimental::filesystem::path& path);

    enum source_file_types
    {
        implementation,
        declaration,
        unknown
    };

    struct source_file : public reload::filesystem::generic_file_data
    {
    private:
        void initialize()
        {
            if (is_declaration_file(path))
            {
                type = source_file_types::declaration;
            }
            else if (is_implementation_file(path))
            {
                type = source_file_types::implementation;
            }

            last_write_time = std::experimental::filesystem::last_write_time(path);
        }

    public:
        source_file_types type = source_file_types::unknown;
        std::vector<source_file*> dependencies;
        std::vector<source_file*> dependents;

        bool is_dirty = false;

        source_file(const reload::filesystem::generic_file_data&& file_data, bool is_dirty = false)
            : generic_file_data(std::move(file_data)), is_dirty(is_dirty)
        {
            initialize();
        }

        source_file(const reload::filesystem::generic_file_data& file_data, bool is_dirty = false)
            : generic_file_data(file_data), is_dirty(is_dirty)
        {
            initialize();
        }

        bool depends_on(source_file& file)
        {
            for (auto i : dependencies)
            {
                if (i == &file)
                {
                    return true;
                }
            }

            return false;
        }

        void extract_dependencies();

        std::vector<source_file*> extract_dependents_recursive()
        {
            std::vector<source_file*> result;
            for (auto& i : dependents)
            {
                bool visited = false;
                for (auto& j : result)
                {
                    if (i == j)
                    {
                        visited = true;
                    }
                }
                if (!visited)
                {
                    result.push_back(i);
                }

                auto sub_result = i->extract_dependents_recursive();
                for (auto j : sub_result)
                {
                    result.push_back(j);
                }
            }

            return std::move(result);
        }

        void update_dependencies_mmap();

        template <typename Writer>
        void serialize(Writer& writer) const
        {
            writer.StartObject();
            writer.String(TEXT("dependencies"));
            writer.StartArray();
            for (auto& i : dependencies)
            {
                writer.String(i.path);
            }
            writer.EndArray();
            writer.EndObject();
        }
    };
}
