// Copyright 2019 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file foonathan_memory_helpers.hpp
 *
 */

#ifndef FASTRTPS_UTILS_COLLECTIONS_FOONATHAN_MEMORY_HELPERS_HPP_
#define FASTRTPS_UTILS_COLLECTIONS_FOONATHAN_MEMORY_HELPERS_HPP_

#include <foonathan/memory/memory_pool.hpp>
#include <foonathan/memory/detail/debug_helpers.hpp>

#include "ResourceLimitedContainerConfig.hpp"

namespace eprosima {
namespace fastrtps {

/**
 * A helper to calculate the block size of a memory pool given the side of the node and
 * a number of nodes.
 *
 * @tparam MemoryPool memory_pool specialization
 *
 * @param node_size   Size of the node for the memory pool
 * @param num_nodes   Number of nodes per block in the pool
 *
 * @return the block size to pass to the memory pool constructor
 */
template <typename MemoryPool>
constexpr std::size_t memory_pool_block_size(
        std::size_t node_size,
        std::size_t num_nodes,
        std::size_t padding = foonathan::memory::detail::memory_block_stack::implementation_offset)
{
    return num_nodes
           * ((node_size > MemoryPool::min_node_size ? node_size : MemoryPool::min_node_size) // Room for elements
           * (foonathan::memory::detail::debug_fence_size ? 3 : 1))                           // Room for debug info
           + padding;                                                                         // Room for padding
}

/**
 * A helper to calculate the block size of a memory pool given the side of the node and
 * a resource limits configuration.
 *
 * @tparam MemoryPool memory_pool specialization
 *
 * @param node_size   Size of the node for the memory pool
 * @param limits      Resource limits configuration of the container
 *
 * @return the block size to pass to the memory pool constructor
 */
template <typename MemoryPool>
std::size_t memory_pool_block_size(
        std::size_t node_size,
        const ResourceLimitedContainerConfig& limits)
{
    size_t num_elems = limits.increment > 0 ? limits.initial : limits.maximum;
    if (num_elems < 1u)
    {
        num_elems = 1u;
    }

    return memory_pool_block_size<MemoryPool>(node_size, num_elems);
}

/**
 * A helper to calculate the node size of a std::set<T>
 *
 * @tparam T   Type for which the std::set<T> node size is to be calculated
 *
 * @return The node size for std::set<T>
 */
template <typename T>
constexpr std::size_t memory_set_node_size()
{
#ifdef FOONATHAN_MEMORY_NO_NODE_SIZE
    return sizeof(std::set<T>::value_type) + std::integral_constant<std::size_t, 32>::value;
#else
    return foonathan::memory::set_node_size<std::set<T>::value_type>::value;
#endif
}

/**
 * A helper to calculate the node size of a std::map<K, V>
 *
 * @tparam K   Type for the key of the std::map
 * @tparam V   Type for the value of the std::map
 *
 * @return The node size for std::map<K, V>
 */
template <typename K, typename V>
constexpr std::size_t memory_map_node_size()
{
    using map_node_t = std::pair<std::size_t, std::map<K, V>::value_type>;
#ifdef FOONATHAN_MEMORY_NO_NODE_SIZE
    return sizeof(map_node_t) + std::integral_constant<std::size_t, 32>::value;
#else
    return foonathan::memory::map_node_size<map_node_t>::value;
#endif
}

/**
 * A helper to calculate the node size of a std::unordered_map<K, V>
 *
 * @tparam K   Type for the key of the std::unordered_map
 * @tparam V   Type for the value of the std::unordered_map
 *
 * @return The node size for std::unordered_map<K, V>
 */
template <typename K, typename V>
constexpr std::size_t memory_unordered_map_node_size()
{
#ifdef FOONATHAN_MEMORY_NO_NODE_SIZE
    return sizeof(std::unordered_map<K, V>::value_type) + std::integral_constant<std::size_t, 32>::value;
#else
    return foonathan::memory::unordered_map_node_size<std::unordered_map<K, V>::value_type>::value;
#endif
}

}  // namespace fastrtps
}  // namespace eprosima

#endif /* FASTRTPS_UTILS_COLLECTIONS_FOONATHAN_MEMORY_HELPERS_HPP_ */
