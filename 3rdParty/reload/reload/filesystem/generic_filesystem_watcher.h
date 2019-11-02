#pragma once
#include <functional>
#include <filesystem>
#include "generic_file_data.h"

namespace reload
{
    namespace filesystem
    {
        enum filesystem_event_types
        {
            file_added,
            file_renamed_from,
            file_removed,
            file_renamed_to,
            file_modified
        };

        using generic_filesystem_watcher_filter_predicate = std::function<bool(const std::experimental::filesystem::path& path)>;
        using generic_filesystem_watcher_callback = std::function<void(const reload::filesystem::generic_file_data& file_data, const filesystem_event_types type)>;

        class generic_filesystem_watcher
        {
        private:
            static std::size_t id_counter;

        public:
            template<typename File_data, typename Event_type,
                typename std::enable_if<!is_invocable<generic_filesystem_watcher_callback, const File_data&, const Event_type>::value, int>::type = 0>
                void invoke_event_callback(const File_data& file_data, const Event_type type)
            {
                static_assert(false, "Callback signature is incompatible. Refer to the wiki for further instructions.");
            }

            template<typename File_data, typename Event_type,
                typename std::enable_if<is_invocable<generic_filesystem_watcher_callback, const File_data&, const Event_type>::value, int>::type = 0>
                void invoke_event_callback(const File_data& file_data, const Event_type type)
            {
                callback(std::move(file_data), type);
            }

            const std::size_t id;
            std::experimental::filesystem::path path;
            generic_filesystem_watcher_filter_predicate filter;
            generic_filesystem_watcher_callback callback;
            bool recursive;

            generic_filesystem_watcher()
                : id(id_counter++), recursive(false)
            {
            }

            generic_filesystem_watcher(const std::experimental::filesystem::path& path, bool recursive,
                const generic_filesystem_watcher_filter_predicate&& filter, const generic_filesystem_watcher_callback&& callback)
                : id(id_counter++), path(path), callback(callback), filter(filter), recursive(recursive)
            {
            }

            virtual ~generic_filesystem_watcher()
            {
            }

            virtual void initialize(const std::experimental::filesystem::path& path, bool recursive,
                const generic_filesystem_watcher_filter_predicate&& filter, const generic_filesystem_watcher_callback&& callback)
            {
            }

            virtual void initialize(const std::experimental::filesystem::path& path, bool recursive,
                const generic_filesystem_watcher_callback&& callback)
            {
            }

            virtual void update()
            {
            }
        };
    }
    using filesystem_event_types = reload::filesystem::filesystem_event_types;
}