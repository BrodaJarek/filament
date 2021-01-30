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

namespace filament::fg2 {

PassNode::PassNode(FrameGraph& fg, const char* name, PassExecutor* base) noexcept
        : DependencyGraph::Node(fg.getGraph()), name(name), base(base, fg.getArena()) {
}

PassNode::PassNode(PassNode&& rhs) noexcept = default;

PassNode::~PassNode() = default;

void PassNode::onCulled(DependencyGraph* graph) {
}

} // namespace filament::fg2
