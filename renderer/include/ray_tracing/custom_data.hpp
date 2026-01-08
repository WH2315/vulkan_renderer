#pragma once

#include "resources/descriptor/storage_buffer.hpp"
#include "core/log.hpp"
#include <map>

namespace wen {

using ClassHashCode = decltype(typeid(int).hash_code());
template <class T>
ClassHashCode getClassHashCode() {
    return typeid(std::remove_reference_t<T>).hash_code();
}

template <class CustomDataCreateInfo>
struct CustomDataRegister {
    struct CustomDataWrapper {
        uint32_t size = 0;
        std::function<void*()> create;
        std::function<void(void*)> destroy;
        std::function<void*(void*)> getData;
        std::function<uint32_t(void*)> getSize;
        std::function<void(void*, uint32_t)> alignSize;
        std::function<std::shared_ptr<StorageBuffer>(uint32_t)> createStorageBuffer;
    };

    struct GroupInfo {
        uint32_t offset = 0;
        uint32_t count = 0;
        std::vector<CustomDataCreateInfo> custom_data_cis;
        std::map<ClassHashCode, void*> custom_data_map;
    };

    CustomDataRegister() = default;
    ~CustomDataRegister() {
        custom_data_wrapper_map.clear();
        groups.clear();
        custom_data_buffer_map.clear();
    }

    GroupInfo createGroup() {
        std::map<ClassHashCode, void*> custom_data_map;
        for (auto& [hash_code, wrapper] : custom_data_wrapper_map) {
            custom_data_map.insert(std::make_pair(hash_code, wrapper.create()));
        }
        return {
            .offset = 0,
            .count = 0,
            .custom_data_cis = {},
            .custom_data_map = std::move(custom_data_map),
        };
    }

    uint32_t buildGroup() {
        uint32_t count = 0;
        for (auto& [group_id, group_info] : groups) {
            auto [hash_code, ptr] = *group_info.custom_data_map.begin();
            group_info.offset = count;
            group_info.count = custom_data_wrapper_map.at(hash_code).getSize(ptr);
            count += group_info.count;
        }

        for (auto& [hash_code, wrapper] : custom_data_wrapper_map) {
            custom_data_buffer_map.insert(std::make_pair(hash_code, wrapper.createStorageBuffer(count)));
        }

        for (auto& [group_id, group_info] : groups) {
            for (auto& [hash_code, ptr] : group_info.custom_data_map) {
                auto& wrapper = custom_data_wrapper_map.at(hash_code);
                memcpy(
                    static_cast<uint8_t*>(custom_data_buffer_map.at(hash_code)->map()) + wrapper.size * group_info.offset,
                    wrapper.getData(ptr),
                    wrapper.getSize(ptr) * wrapper.size
                );
                custom_data_buffer_map.at(hash_code)->unmap();
            }
        }

        for (auto& [group_id, group_info] : groups) {
            for (auto& [hash_code, ptr] : group_info.custom_data_map) {
                custom_data_wrapper_map.at(hash_code).destroy(ptr);
            }
            group_info.custom_data_map.clear();
        }

        return count;
    }

    template <class CustomData>
    void registerCustomData() {
        auto hash_code = getClassHashCode<CustomData>();
        using type_t = std::remove_reference_t<CustomData>;
        using vector_t = std::vector<type_t>;
        if (custom_data_wrapper_map.find(hash_code) != custom_data_wrapper_map.end()) {
            WEN_ERROR("CustomDataRegister: CustomData {} has been registered", typeid(type_t).name())
            return;
        }
        custom_data_wrapper_map[hash_code] = {
            .size = sizeof(type_t),
            .create = [] {
                auto* ptr = new vector_t();
                return static_cast<void*>(ptr);
            },
            .destroy = [](void* ptr) {
                auto* data_ptr = static_cast<vector_t*>(ptr);
                data_ptr->clear();
                delete data_ptr;
            },
            .getData = [](void* ptr) {
                auto* data_ptr = static_cast<vector_t*>(ptr);
                return static_cast<void*>(data_ptr->data());
            },
            .getSize = [](void* ptr) {
                auto* data_ptr = static_cast<vector_t*>(ptr);
                return static_cast<uint32_t>(data_ptr->size());
            },
            .alignSize = [](void* ptr, uint32_t size) {
                auto* data_ptr = static_cast<vector_t*>(ptr);
                while (data_ptr->size() < size) {
                    data_ptr->emplace_back();
                }
            },
            .createStorageBuffer = [](uint32_t count) {
                return std::make_shared<StorageBuffer>(
                    sizeof(type_t) * count,
                    vk::BufferUsageFlagBits::eStorageBuffer,
                    VMA_MEMORY_USAGE_CPU_TO_GPU,
                    VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                );
            }
        };
    }

    template <class... CustomDatas>
    void addInstance(uint32_t id, const CustomDataCreateInfo& ci, CustomDatas&&... datas) {
        if (groups.find(id) == groups.end()) {
            groups.insert(std::make_pair(id, createGroup()));
        }

        auto& group_info = groups.at(id);
        group_info.custom_data_cis.emplace_back(ci);

        if constexpr (sizeof...(datas) > 0) {
            addCustomData<CustomDatas...>(group_info, std::forward<CustomDatas>(datas)...);
        }

        for (auto& [hash_code, ptr] : group_info.custom_data_map) {
            custom_data_wrapper_map.at(hash_code).alignSize(ptr, group_info.custom_data_cis.size());
        }
    }

    template <class T, class... Args>
    uint32_t addCustomData(GroupInfo& group, T&& arg, Args&&... args) {
        auto hash = getClassHashCode<T>();
        auto it = group.custom_data_map.find(hash);
        if (it == group.custom_data_map.end() || it->second == nullptr) {
            WEN_ERROR("CustomDataRegister: custom data not registered")
            return 0;
        }

        auto* ptr = static_cast<std::vector<std::remove_reference_t<T>>*>(it->second);
        ptr->push_back(std::forward<std::remove_reference_t<T>>(arg));
        if constexpr (sizeof...(args) > 0) {
            return addCustomData<Args...>(group, std::forward<Args>(args)...);
        } else {
            return ptr->size();
        }
    }

    template <class CustomData>
    std::shared_ptr<StorageBuffer> getCustomDataBuffer() {
        return custom_data_buffer_map.at(getClassHashCode<CustomData>());
    }

    void multiThreadUpdate(uint32_t id, const std::function<void(uint32_t, uint32_t, uint32_t)>& update) {
        auto group = groups.at(id);
        uint32_t begin = group.offset;
        uint32_t end = begin + group.count;
        
        uint32_t index = 0;
        std::vector<std::thread> threads;
        while (begin < end) {
            uint32_t batch_end = std::min(begin + 250, end);
            threads.emplace_back([=]() {
                update(index, begin, batch_end);
            });
            index += 250;
            begin = batch_end;
        }
        for (auto& thread : threads) {
            thread.join();
        }
    }

    std::map<uint32_t, GroupInfo> groups;
    std::map<ClassHashCode, CustomDataWrapper> custom_data_wrapper_map;
    std::map<ClassHashCode, std::shared_ptr<StorageBuffer>> custom_data_buffer_map;
};

} // namespace wen