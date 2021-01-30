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

#include "fg2/FrameGraph.h"
#include "fg2/details/PassNode.h"
#include "fg2/details/ResourceNode.h"
#include "fg2/details/DependencyGraph.h"

namespace filament::fg2 {

FrameGraph::Builder::Builder(FrameGraph& fg, PassNode& pass) noexcept
        : mFrameGraph(fg), mPass(pass) {
}

FrameGraph::Builder::~Builder() noexcept = default;

void FrameGraph::Builder::sideEffect() noexcept {
    // TODO: could be interesting to implement this by adding a "side effect" node
    mPass.makeTarget();
}

// ------------------------------------------------------------------------------------------------

FrameGraph::FrameGraph(ResourceAllocatorInterface& resourceAllocator)
        : mResourceAllocator(resourceAllocator),
          mArena("FrameGraph Arena", 131072)
{
}

FrameGraph::~FrameGraph() = default;

FrameGraph& FrameGraph::compile() noexcept {
    mGraph.cull();
    mGraph.export_graphviz(utils::slog.d);
    return *this;
}

void FrameGraph::execute(backend::DriverApi& driver) noexcept {
}

void FrameGraph::present(FrameGraphHandle input) {
    assert(isValid(input));

//    struct Empty{};
//    addPass<Empty>("Present",
//            [&](Builder& builder, auto& data) {
//                builder.read(input);
//                builder.sideEffect();
//            }, [](FrameGraphResources const& resources, auto const& data, backend::DriverApi&) {});

    getResourceNode(input)->makeTarget();
}

FrameGraphId<Texture> FrameGraph::import(char const* name, Texture::Descriptor const& desc,
        backend::Handle<backend::HwRenderTarget> target) {
    return FrameGraphId<Texture>();
}

FrameGraph::Builder FrameGraph::addPassInternal(char const* name, PassExecutor* base) noexcept {
    // record in our pass list and create the builder
    PassNode* node = new PassNode(*this, name, base);
    mPassNodes.emplace_back(node);
    return Builder(*this, *node);
}

FrameGraphHandle FrameGraph::addResourceInternal(VirtualResource* resource) noexcept {
    FrameGraphHandle handle(mResourceSlots.size());
    ResourceSlot& slot = mResourceSlots.emplace_back();
    slot.rid = mResources.size();
    slot.nid = mResourceNodes.size();
    mResources.emplace_back(resource);
    mResourceNodes.emplace_back(new ResourceNode(*this, handle));
    return handle;
}

FrameGraphHandle FrameGraph::writeInternal(FrameGraphHandle handle) noexcept {
    assert(isValid(handle));

    if (!getResourceNode(handle)->hasWriter()) {
        // this just means the resource was just created and was never written to.
        return handle;
    }

    // create a new handle with next version number
    FrameGraphHandle newHandle(handle);
    newHandle.version++;

    // update the slot with new ResourceNode index
    ResourceSlot& slot = mResourceSlots[handle.index];
    slot.nid = mResourceNodes.size();

    // create new ResourceNodes
    ResourceNode* node = new ResourceNode(*this, newHandle);
    mResourceNodes.emplace_back(node);

    // update version number in resource
    mResources[slot.rid]->version = newHandle.version;

    return newHandle;
}

} // namespace filament::fg2
