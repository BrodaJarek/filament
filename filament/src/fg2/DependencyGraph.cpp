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

#include "fg2/details/DependencyGraph.h"

namespace filament::fg2 {

DependencyGraph::DependencyGraph() noexcept {
    // Some reasonable defaults size for our vectors
    mNodes.reserve(8);
    mEdges.reserve(16);
}

uint32_t DependencyGraph::generateNodeId() noexcept {
    return mNodes.size();
}

void DependencyGraph::registerNode(Node* node, NodeID id) noexcept {
    // node* is not fully constructed here
    assert(id == mNodes.size());
    mNodes.push_back(node);
}

bool DependencyGraph::isEdgeValid(DependencyGraph::Edge const* edge) const noexcept {
    auto& nodes = mNodes;
    Node const* from = nodes[edge->from];
    Node const* to = nodes[edge->to];
    return !from->isCulled() && !to->isCulled();
}

void DependencyGraph::link(DependencyGraph::Edge* edge) noexcept {
    mEdges.push_back(edge);
}

DependencyGraph::EdgeContainer const& DependencyGraph::getEdges() const noexcept {
    return mEdges;
}


DependencyGraph::NodeContainer const& DependencyGraph::getNodes() const noexcept {
    return mNodes;
}

DependencyGraph::EdgeContainer DependencyGraph::getIncomingEdges(
        DependencyGraph::Node const* node) const noexcept {
    // TODO: we might need something more efficient
    EdgeContainer result;
    result.reserve(mEdges.size());
    for (auto* edge : mEdges) {
        if (edge->to == node->getId()) {
            result.push_back(edge);
        }
    }
    return result;
}

DependencyGraph::EdgeContainer DependencyGraph::getOutgoingEdges(
        DependencyGraph::Node const* node) const noexcept {
    // TODO: we might need something more efficient
    EdgeContainer result;
    result.reserve(mEdges.size());
    for (auto* edge : mEdges) {
        if (edge->from == node->getId()) {
            result.push_back(edge);
        }
    }
    return result;
}

DependencyGraph::Node const* DependencyGraph::getNode(DependencyGraph::NodeID id) const noexcept {
    return mNodes[id];
}

DependencyGraph::Node* DependencyGraph::getNode(DependencyGraph::NodeID id) noexcept {
    return mNodes[id];
}

void DependencyGraph::cull() noexcept {
    auto& nodes = mNodes;
    auto& edges = mEdges;

    // update reference counts
    for (Edge* const pEdge : edges) {
        Node* node = nodes[pEdge->from];
        node->mRefCount++;
    }

    // cull nodes with a 0 reference count
    std::vector<Node*> stack;
    stack.reserve(nodes.size());
    for (Node* const pNode : nodes) {
        if (pNode->getRefCount() == 0) {
            stack.push_back(pNode);
        }
    }
    while (!stack.empty()) {
        Node* const pNode = stack.back();
        stack.pop_back();
        EdgeContainer const& incoming = getIncomingEdges(pNode);
        for (Edge* edge : incoming) {
            Node* pLinkedNode = getNode(edge->from);
            if (--pLinkedNode->mRefCount == 0) {
                stack.push_back(pLinkedNode);
            }
        }
        pNode->onCulled(this);
    }
}

void DependencyGraph::clear() noexcept {
    mEdges.clear();
    mNodes.clear();
}

void DependencyGraph::export_graphviz(utils::io::ostream& out, char const* name) {
#ifndef NDEBUG
    const char* graphName = name ? name : "graph";
    out << "digraph \"" << graphName << "\" {\n";
    out << "rankdir = LR\n";
    out << "bgcolor = black\n";
    out << "node [shape=rectangle, fontname=\"helvetica\", fontsize=10]\n\n";

    auto const& nodes = mNodes;

    for (Node const* node : nodes) {
        uint32_t id = node->getId();
        const char* const nodeName = node->getName();
        uint32_t refCount = node->getRefCount();

        out << "\"N" << id << "\" [label=\"" << nodeName
            << "\\nrefs: " << refCount
            << "\\nseq: " << id
            << "\", style=filled, fillcolor="
            << (refCount ? "darkorange" : "darkorange4") << "]\n";
    }

    out << "\n";
    for (Node const* node : nodes) {
        uint32_t id = node->getId();

        // render the valid edges
        out << "N" << id << " -> { ";
        for (Edge const* edge : getOutgoingEdges(node)) {
            if (isEdgeValid(edge)) {
                Node const* ref = getNode(edge->to);
                out << "N" << ref->getId() << " ";
            }
        }
        out << "} [color=red2]\n";

        // render the invalid edges
        out << "N" << id << " -> { ";
        for (Edge const* edge : getOutgoingEdges(node)) {
            if (!isEdgeValid(edge)) {
                Node const* ref = getNode(edge->to);
                out << "N" << ref->getId() << " ";
            }
        }
        out << "} [color=red4 style=dashed]\n";
    }

    out << "}" << utils::io::endl;
#endif
}

// ------------------------------------------------------------------------------------------------

DependencyGraph::Node::Node(DependencyGraph& graph) noexcept : mId(graph.generateNodeId()) {
    graph.registerNode(this, mId);
}

DependencyGraph::Node::~Node() noexcept = default;

uint32_t DependencyGraph::Node::getRefCount() const noexcept {
    return (mRefCount & TARGET) ? 1u : mRefCount;
}

uint32_t DependencyGraph::Node::getId() const noexcept {
    return mId;
}

bool DependencyGraph::Node::isCulled() const noexcept {
    return mRefCount == 0;
}

void DependencyGraph::Node::makeTarget() noexcept {
    assert(mRefCount == 0);
    mRefCount = TARGET;
}

bool DependencyGraph::Node::isTarget() const noexcept {
    return mRefCount >= TARGET;
}

} // namespace filament::fg2
