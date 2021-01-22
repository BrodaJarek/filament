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

#ifndef TNT_FILAMENT_FG2_TEXTURE_H
#define TNT_FILAMENT_FG2_TEXTURE_H

#include "fg2/FrameGraphId.h"

#include <backend/DriverEnums.h>
#include <backend/Handle.h>

namespace filament {
class ResourceAllocatorInterface;
} // namespace::filament

namespace filament::fg2 {

/**
 * A FrameGraph resource is a structure that declares at least:
 *      struct Descriptor;
 *      struct SubResourceDescriptor;
 *      a Usage bitmask
 * And declares and define:
 *      void create(ResourceAllocatorInterface&, Descriptor const&, Usage) noexcept;
 *      void destroy(ResourceAllocatorInterface&) noexcept;
 */
struct Texture {
    backend::Handle<backend::HwTexture> texture;

    /** describes a Texture resource */
    struct Descriptor {
        uint32_t width = 1;     // width of resource in pixel
        uint32_t height = 1;    // height of resource in pixel
        uint32_t depth = 1;     // # of images for 3D textures
        uint8_t levels = 1;     // # of levels for textures
        uint8_t samples = 0;    // 0=auto, 1=request not multisample, >1 only for NOT SAMPLEABLE
        backend::SamplerType type = backend::SamplerType::SAMPLER_2D;     // texture target type
        backend::TextureFormat format = backend::TextureFormat::RGBA8;    // resource internal format
    };

    /** Describes a Texture sub-resource */
    struct SubResourceDescriptor {
        uint8_t level = 0;      // resource's mip level
        uint8_t layer = 0;      // resource's layer or face
    };

    /** Usage for read and write */
    using Usage = backend::TextureUsage;

    /**
     * Create the concrete resource
     * @param resourceAllocator resource allocator for textures and such
     * @param descriptor Descriptor to the resource
     */
    void create(ResourceAllocatorInterface& resourceAllocator,
            Descriptor const& descriptor, Usage usage) noexcept;

    /**
     * Destroy the concrete resource
     * @param resourceAllocator
     */
    void destroy(ResourceAllocatorInterface& resourceAllocator) noexcept;
};

} // namespace filament::fg2

#endif //TNT_FILAMENT_FG2_TEXTURE_H
