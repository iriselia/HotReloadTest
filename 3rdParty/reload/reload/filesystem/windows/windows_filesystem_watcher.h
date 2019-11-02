#pragma once
#include <functional>
#include <filesystem>

#include "reload/common/type_traits.h"
#include "reload/filesystem/generic_filesystem_watcher.h"

#include "windows_filesystem_event.h"
#include "windows_filesystem_file_data.h"
#include "windows_filesystem_notification_handles.h"

namespace reload
{
    namespace filesystem
    {
        class windows_filesystem_watcher : public generic_filesystem_watcher
        {
        private:
            windows_filesystem_notification_handles notification_handles;
            std::vector<windows_file_data> tracked_files_cache;
            std::vector<windows_file_data> previous_tracked_files_cache;
            std::vector<windows_filesystem_event> events;

            void cache_file(const std::experimental::filesystem::path&& path);
            void update_file_tree_cache();
            void store_event(windows_filesystem_event_types event_type);

        public:
            windows_filesystem_watcher() : generic_filesystem_watcher()
            {
            }

            ~windows_filesystem_watcher() override
            {
            }

            void initialize(const std::experimental::filesystem::path& path, bool recursive,
                const generic_filesystem_watcher_filter_predicate&& filter, const generic_filesystem_watcher_callback&& callback) override;

            void initialize(const std::experimental::filesystem::path& path, bool recursive,
                const generic_filesystem_watcher_callback&& callback) override;

            void update() override;

            const std::vector<const reload::filesystem::generic_file_data*> tracked_files_data();

            const reload::filesystem::generic_file_data* find_tracked_file(const std::experimental::filesystem::path& path)
            {
                for (auto& i : tracked_files_cache)
                {
                    if (i.path == path)
                    {
                        return &i;
                    }
                }
            }
        };
        using filesystem_watcher = reload::filesystem::windows_filesystem_watcher;
    }
    using filesystem_watcher = reload::filesystem::filesystem_watcher;
}