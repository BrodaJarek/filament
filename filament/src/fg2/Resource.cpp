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

#include "fg2/details/Resource.h"
#include "fg2/details/ResourceNode.h"

namespace filament::fg2 {

VirtualResource::~VirtualResource() noexcept = default;

void VirtualResource::addOutgoingEdge(ResourceNode* node, DependencyGraph::Edge* edge) noexcept {
    node->addOutgoingEdge(edge);
}

void VirtualResource::setIncomingEdge(ResourceNode* node, DependencyGraph::Edge* edge) noexcept {
    node->setIncomingEdge(edge);
}

} // namespace filament::fg2
