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

#ifndef TNT_FILAMENT_FG2_RESOURCE_H
#define TNT_FILAMENT_FG2_RESOURCE_H

#include "fg2/FrameGraphId.h"
#include "fg2/details/DependencyGraph.h"
#include "fg2/details/PassNode.h"
#include "fg2/details/ResourceNode.h"

#include <vector>

namespace filament {
class ResourceAllocatorInterface;
} // namespace::filament

namespace filament::fg2 {

/*
 * The generic parts of virtual resources.
 */
class VirtualResource {
public:
    // constants
    const char* const name;
    const uint16_t id; // for debugging and graphing
    bool imported = false;

    // updated by builder
    FrameGraphHandle::Version version = 0;

    // computed during compile()
    PassNode* first = nullptr;  // pass that needs to instantiate the resource
    PassNode* last = nullptr;   // pass that can destroy the resource

    VirtualResource(const char* name, uint16_t id) noexcept : name(name), id(id) { }
    VirtualResource(VirtualResource const&) = delete;
    VirtualResource& operator=(VirtualResource const&) = delete;
    virtual ~VirtualResource() noexcept;

    /*
     * Called during FrameGraph::compile(), this gives an opportunity for this resource to
     * calculate its effective usage flags.
     */
    virtual void resolveUsage(DependencyGraph& graph,
            DependencyGraph::Edge const* const* edges, size_t count) noexcept = 0;

    /* Instantiate the concrete resource */
    virtual void devirtualize(ResourceAllocatorInterface& resourceAllocator) noexcept = 0;

    /* Destroy the concrete resource */
    virtual void destroy(ResourceAllocatorInterface& resourceAllocator) noexcept = 0;

    /** Destroy an Edge instantiated by this resource */
    virtual void destroyEdge(DependencyGraph::Edge* edge) noexcept = 0;

protected:
    void addOutgoingEdge(ResourceNode* node, DependencyGraph::Edge* edge) noexcept;
    void setIncomingEdge(ResourceNode* node, DependencyGraph::Edge* edge) noexcept;
};

// ------------------------------------------------------------------------------------------------

/*
 * Resource specific parts of a VirtualResource
 */
template<typename RESOURCE>
class Resource : public VirtualResource {
    using Usage = typename RESOURCE::Usage;

    // valid only after devirtualize() has been called
    RESOURCE resource{};

    // valid only after resolveUsage() has been called
    Usage usage{};

public:
    using Descriptor = typename RESOURCE::Descriptor;
    using SubResourceDescriptor = typename RESOURCE::SubResourceDescriptor;

    // our concrete (sub)resource descriptors -- used to create it.
    Descriptor descriptor;
    SubResourceDescriptor subResourceDescriptor;

    // An Edge with added data from this resource
    class ResourceEdge : public DependencyGraph::Edge {
    public:
        Usage usage;
        ResourceEdge(DependencyGraph& graph,
                DependencyGraph::Node* from, DependencyGraph::Node* to, Usage usage) noexcept
                : DependencyGraph::Edge(graph, from, to), usage(usage) {
        }
    };

    Resource(const char* name, Descriptor const& desc, uint16_t id) noexcept
        : VirtualResource(name, id), descriptor(desc) {
    }

    ~Resource() noexcept = default;

    // pass Node to resource Node edge (a write to)
    void connect(DependencyGraph& graph,
            PassNode* passNode, ResourceNode* resourceNode, Usage u) noexcept {
        auto* edge = new ResourceEdge(graph, passNode, resourceNode, u);
        setIncomingEdge(resourceNode, edge);
    }

    // resource Node to pass Node edge (a read from)
    void connect(DependencyGraph& graph,
            ResourceNode* resourceNode, PassNode* passNode, Usage u) noexcept {
        auto* edge = new ResourceEdge(graph, resourceNode, passNode, u);
        addOutgoingEdge(resourceNode, edge);
    }

private:
    /*
     * The virtual below must be in a header file as RESOURCE is only known at compile time
     */

    void resolveUsage(DependencyGraph& graph, DependencyGraph::Edge const* const* edges,
            size_t count) noexcept override {
        for (size_t i = 0; i < count; i++) {
            if (graph.isEdgeValid(edges[i])) {
                // this Edge is guaranteed to be a ResourceEdge<RESOURCE> by construction
                ResourceEdge const* const edge = static_cast<ResourceEdge const*>(edges[i]);
                usage |= edge->usage;
            }
        }
    }

    void destroyEdge(DependencyGraph::Edge* edge) noexcept override {
        // this Edge is guaranteed to be a ResourceEdge<RESOURCE> by construction
        delete static_cast<ResourceEdge *>(edge);
    }

    void devirtualize(ResourceAllocatorInterface& resourceAllocator) noexcept override {
        resource.create(resourceAllocator, descriptor, usage);
    }

    void destroy(ResourceAllocatorInterface& resourceAllocator) noexcept override {
        resource.destroy(resourceAllocator);
    }
};


} // namespace filament::fg2

#endif //TNT_FILAMENT_FG2_RESOURCE_H
