#include "windows_filesystem_watcher.h"

namespace reload
{
    namespace filesystem
    {
        std::size_t generic_filesystem_watcher::id_counter = 0;

        void windows_filesystem_watcher::initialize(const std::experimental::filesystem::path & path, bool recursive, const generic_filesystem_watcher_filter_predicate && filter, const generic_filesystem_watcher_callback && callback)
        {
            this->path = path;
            this->recursive = recursive;
            this->filter = std::move(filter);
            this->callback = std::move(callback);

            for (int i = 0; i < filesystem_notification_type_count; ++i)
            {
                notification_handles[i] = FindFirstChangeNotification(path.native().c_str(), recursive, filesystem_notification_types[i]);
                if (notification_handles[i] == INVALID_HANDLE_VALUE)
                {
                    throw std::runtime_error("FindFirstChangeNotification failed.");
                }
            }

            update_file_tree_cache();
        }

        void windows_filesystem_watcher::initialize(const std::experimental::filesystem::path & path, bool recursive, const generic_filesystem_watcher_callback && callback)
        {
            initialize(path, recursive, std::move([](const std::experimental::filesystem::path& path) { return true; }), std::move(callback));
        }

        void windows_filesystem_watcher::update()
        {

            bool file_change_detected = false;
            DWORD status[filesystem_notification_type_count];
            for (auto i = 0; i < filesystem_notification_type_count; i++)
            {
                status[i] = WaitForMultipleObjects(filesystem_notification_type_count, &notification_handles, false, 0);
                if (status[i] == WAIT_OBJECT_0 + i)
                {
                    file_change_detected = true;
                }

                if (FindNextChangeNotification(notification_handles[i]) == false)
                {
                    throw std::runtime_error("FindNextChangeNotification failed.");
                }
            }

            if (!file_change_detected)
            {
                return;
            }

            previous_tracked_files_cache = std::move(tracked_files_cache);
            this->update_file_tree_cache();

            for (auto i = 0; i < filesystem_notification_type_count; i++)
            {
                switch (status[i])
                {
                    case WAIT_FAILED:
                    {
                        status[i] = GetLastError();
                        throw std::runtime_error("WaitForMultipleObjects failed.");
                        break;
                    }

                    case WAIT_TIMEOUT:
                    {
                        break;
                    }

                    default:
                    {
                        if (status[i] == WAIT_OBJECT_0 + i)
                        {
                            /*
                            bool ok = false;
                            if (previous_file_tree_cache.size() == file_tree_cache.size())
                            {
                                for (int i = 0; i < previous_file_tree_cache.size(); i++)
                                {
                                    if (previous_file_tree_cache[i].path != file_tree_cache[i].path)
                                    {
                                        ok = true;
                                        break;
                                    }
                                }

                                if (!ok)
                                {
                                    int a = 0;
                                    std::cerr << "Probably broken..." << std::endl;
                                    a++;
                                }

                            }
                            //*/
                            store_event(static_cast<windows_filesystem_event_types>(i));
                            status[i] = WAIT_TIMEOUT;
                        }
                        break;
                    }
                }
            }

            while (!events.empty())
            {
                auto& event = events.back();

                filesystem_event_types type;

                switch (event.file_action_type)
                {

                    case FILE_ACTION_ADDED:
                    {
                        type = filesystem_event_types::file_added;
                        break;
                    }
                    case FILE_ACTION_RENAMED_NEW_NAME:
                    {
                        type = filesystem_event_types::file_renamed_to;
                        break;
                    }
                    case FILE_ACTION_RENAMED_OLD_NAME:
                    {
                        type = filesystem_event_types::file_renamed_from;
                        break;
                    }
                    case FILE_ACTION_REMOVED:
                    {
                        type = filesystem_event_types::file_removed;
                        break;
                    }

                    case FILE_ACTION_MODIFIED:
                    {
                        type = filesystem_event_types::file_modified;
                        break;
                    }
                };

                if (!event.file_data)
                {
                    throw "shouldn't be empty.";
                }

                invoke_event_callback(*event.file_data, type);
                events.pop_back();
            }

            events.clear();
        }

        const std::vector<const reload::filesystem::generic_file_data*> windows_filesystem_watcher::tracked_files_data()
        {
            std::vector<const reload::filesystem::generic_file_data*> result;
            for (auto& i : tracked_files_cache)
            {
                result.push_back(&i);
            }
            return std::move(result);
        }

        /*
        * https://cyberspock.com/2015/10/02/some-time_point-to-from-filetime-conversions/
        * FILETIME to std::chrono::system_clock::time_point (including milliseconds)
        */

        std::chrono::system_clock::time_point file_time_to_std_time_point(const FILETIME& ft)
        {
            SYSTEMTIME st = {0};
            if (!FileTimeToSystemTime(&ft, &st)) {
                std::cerr << "Invalid FILETIME" << std::endl;
                return std::chrono::system_clock::time_point((std::chrono::system_clock::time_point::min)());
            }

            // number of seconds 
            ULARGE_INTEGER ull;
            ull.LowPart = ft.dwLowDateTime;
            ull.HighPart = ft.dwHighDateTime;

            time_t secs = ull.QuadPart / 10000000ULL - 11644473600ULL;
            std::chrono::milliseconds ms((ull.QuadPart / 10000ULL) % 1000);

            auto tp = std::chrono::system_clock::from_time_t(secs);
            tp += ms;
            return std::move(tp);
        }

        void windows_filesystem_watcher::cache_file(const std::experimental::filesystem::path&& path)
        {
            if (!filter(path))
            {
                return;
            }

            WIN32_FIND_DATA fd;
            {
                if (FindFirstFile((path.native()).c_str(), &fd) == INVALID_HANDLE_VALUE)
                {
                    //throw std::runtime_error("Couldn't list files.");
                    std::cerr << "Probably broken..." << std::endl;
                }

                if (fd.cFileName[0] != '.')
                {
                    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                    {
                        LARGE_INTEGER size;
                        size.LowPart = fd.nFileSizeLow;
                        size.HighPart = fd.nFileSizeHigh;
                        FILETIME creation_time;
                        creation_time = fd.ftCreationTime;
                        FILETIME last_write_time;
                        last_write_time = fd.ftLastWriteTime;

                        this->tracked_files_cache.push_back(windows_file_data(std::move(path), size.QuadPart,
                            file_time_to_std_time_point(creation_time), file_time_to_std_time_point(last_write_time)));
                    }
                    else
                    {
                        throw std::runtime_error("Shouldn't come across a directory.");
                    }
                }
            }
        }

        void windows_filesystem_watcher::update_file_tree_cache()
        {
            tracked_files_cache.clear();

            if (std::experimental::filesystem::is_directory(path))
            {
                if (recursive)
                {
                    for (auto& entry : std::experimental::filesystem::recursive_directory_iterator(path))
                    {
                        cache_file(std::move(entry.path()));
                    }
                }
                else
                {
                    for (auto& entry : std::experimental::filesystem::directory_iterator(path))
                    {
                        if (std::experimental::filesystem::is_directory(entry.path()))
                        {
                            cache_file(std::move(entry.path()));
                        }
                    }
                }
            }
            else
            {
                cache_file(std::move(path));
            }
        }

        void windows_filesystem_watcher::store_event(windows_filesystem_event_types event_type)
        {
            windows_filesystem_event event;
            event.id = id;
            event.event_type = event_type;

            for (auto& i : tracked_files_cache)
            {
                i.is_dirty = true;
            }

            for (auto& i : previous_tracked_files_cache)
            {
                i.is_dirty = true;
            }

            // First pass handles CHANGE_LAST_WRITE, CHANGE_CREATION, and CHANGE_SIZE
            // It also prepares for the 3 branches of CHANGE_FILE_NAME
            for (auto& i : previous_tracked_files_cache)
            {
                for (auto& j : tracked_files_cache)
                {
                    if (i.is_dirty && j.is_dirty)
                    {
                        if (i.path == j.path)
                        {
                            i.is_dirty = false;
                            j.is_dirty = false;

                            if (event_type == CHANGE_LAST_WRITE)
                            {
                                if (i.last_write_time != j.last_write_time)
                                {
                                    event.file_data = &i;
                                    event.file_action_type = FILE_ACTION_MODIFIED;
                                    i.last_write_time = j.last_write_time;
                                    events.push_back(std::move(event));
                                    return;
                                }
                            }
                            else if (event_type == CHANGE_CREATION)
                            {
                                if (i.creation_time != j.creation_time)
                                {
                                    event.file_data = &i;
                                    event.file_action_type = FILE_ACTION_ADDED;
                                    i.creation_time = j.creation_time;
                                    events.push_back(std::move(event));
                                    return;
                                }
                            }
                            else if (event_type == CHANGE_SIZE)
                            {
                                if (i.size!= j.size)
                                {
                                    event.file_data = &i;
                                    event.file_action_type = FILE_ACTION_MODIFIED;
                                    i.size = j.size;
                                    events.push_back(std::move(event));
                                    return;
                                }
                            }
                            else if (event_type == CHANGE_FILE_NAME)
                            {
                                break;
                            }
                            else
                            {
                                throw std::runtime_error("Unknown file change event.");
                            }
                        }
                    }
                }
            }

            // 3 branches of CHANGE_FILE_NAME: new file, delete file, rename file
            windows_file_data* deleted_file_ptr = nullptr;
            windows_file_data* renamed_file_ptr = nullptr;
            windows_file_data* new_file_ptr = nullptr;

            for (auto& i : previous_tracked_files_cache)
            {
                if (i.is_dirty)
                {
                    deleted_file_ptr = &i;
                }
            }

            for (auto& i : tracked_files_cache)
            {
                if (i.is_dirty)
                {
                    if (deleted_file_ptr != nullptr)
                    {
                        auto& deleted_file = *deleted_file_ptr;
                        if (i.creation_time == deleted_file.creation_time
                            && i.size == deleted_file.size
                            && i.last_write_time == deleted_file.last_write_time)
                        {
                            new_file_ptr = &i;
                        }
                    }
                    else
                    {
                        new_file_ptr = &i;
                    }

                }
            }

            if (new_file_ptr && deleted_file_ptr)
            {
                auto& new_file = *new_file_ptr;
                auto& deleted_file = *deleted_file_ptr;

                event.file_data = &new_file;
                event.file_action_type = FILE_ACTION_RENAMED_NEW_NAME;
                events.push_back(event);

                event.file_data = &deleted_file;
                event.file_action_type = FILE_ACTION_RENAMED_OLD_NAME;
                events.push_back(std::move(event));

                return;
            }
            else if (deleted_file_ptr)
            {
                auto& deleted_file = *deleted_file_ptr;
                event.file_data = &deleted_file;
                event.file_action_type = FILE_ACTION_REMOVED;
                events.push_back(std::move(event));

                return;
            }

            else if (new_file_ptr)
            {
                auto& new_file = *new_file_ptr;
                event.file_data = &new_file;
                event.file_action_type = FILE_ACTION_ADDED;
                events.push_back(std::move(event));

                return;
            }
        }

    }
}