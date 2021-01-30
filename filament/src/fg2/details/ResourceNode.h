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

#ifndef TNT_FILAMENT_FG2_RESOURCENODE_H
#define TNT_FILAMENT_FG2_RESOURCENODE_H

#include "fg2/details/DependencyGraph.h"
#include "fg2/details/Utilities.h"

namespace filament::fg2 {

class FrameGraph;

class ResourceNode : public DependencyGraph::Node {
public:
    ResourceNode(FrameGraph& fg, FrameGraphHandle h) noexcept;
    ~ResourceNode() noexcept override;

    ResourceNode(ResourceNode const&) = delete;
    ResourceNode& operator=(ResourceNode const&) = delete;

    void addOutgoingEdge(DependencyGraph::Edge* edge) noexcept;
    void setIncomingEdge(DependencyGraph::Edge* edge) noexcept;

    // constants
    const FrameGraphHandle resourceHandle;

    bool hasWriter() const noexcept {
        return mWriter != nullptr;
    }

private:
    // virtuals from DependencyGraph::Node
    char const* getName() const override;
    void onCulled(DependencyGraph* graph) override;

    FrameGraph& mFrameGraph;
    std::vector<DependencyGraph::Edge *> mReaders;
    DependencyGraph::Edge* mWriter = nullptr;
};

} // namespace filament::fg2

#endif //TNT_FILAMENT_FG2_RESOURCENODE_H
