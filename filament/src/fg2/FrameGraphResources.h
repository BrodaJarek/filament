/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TNT_FILAMENT_FG2_FRAMEGRAPHRESOURCES_H
#define TNT_FILAMENT_FG2_FRAMEGRAPHRESOURCES_H

#include "fg2/FrameGraphId.h"

namespace filament::fg2 {

/**
 * Used to retrieve the concrete resources in the execute phase.
 */
class FrameGraphResources {
public:
    FrameGraphResources(FrameGraphResources const&) = delete;
    FrameGraphResources& operator=(FrameGraphResources const&) = delete;

    /**
     * Return the name of the pass being executed
     * @return a pointer to a null terminated string. The caller doesn't get ownership.
     */
    const char* getPassName() const noexcept;

    /**
     * Retrieves the concrete resource for a given handle to a virtual resource.
     * @tparam RESOURCE Type of the resource
     * @param handle    Handle to a virtual resource
     * @return          Reference to the concrete resource
     */
    template<typename RESOURCE>
    RESOURCE const& get(FrameGraphId<RESOURCE> handle) const noexcept;

    /**
     * Retrieves the descriptor associated to a resource
     * @tparam RESOURCE Type of the resource
     * @param handle    Handle to a virtual resource
     * @return          Reference to the descriptor
     */
    template<typename RESOURCE>
    typename RESOURCE::Descriptor const& getDescriptor(FrameGraphId<RESOURCE> handle) const;
};

} // namespace filament::fg2

#endif //TNT_FILAMENT_FG2_FRAMEGRAPHRESOURCES_H
