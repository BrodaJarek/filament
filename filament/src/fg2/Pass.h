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

#ifndef TNT_FILAMENT_FG2_PASS_H
#define TNT_FILAMENT_FG2_PASS_H

#include "private/backend/DriverApiForward.h"

#include "fg2/FrameGraphResources.h"

#include <utils/Allocator.h>

namespace filament::fg2 {

class PassExecutor {
    friend class FrameGraph;
    virtual void execute(FrameGraphResources const& resources, backend::DriverApi& driver) noexcept = 0;
public:
    PassExecutor();
    virtual ~PassExecutor();
    PassExecutor(PassExecutor const&) = delete;
    PassExecutor& operator = (PassExecutor const&) = delete;
};

template<typename Data, typename Execute>
class Pass : private PassExecutor {
    friend class FrameGraph;

    // allow our allocators to instantiate us
    template<typename, typename, typename>
    friend class utils::Arena;

    explicit Pass(Execute&& execute) noexcept
            : PassExecutor(), mExecute(std::move(execute)) {
    }

    void execute(FrameGraphResources const& resources, backend::DriverApi& driver) noexcept final {
        mExecute(resources, mData, driver);
    }

    Execute mExecute;
    Data mData;

public:
    Data const& getData() const noexcept { return mData; }
    Data& getData() noexcept { return mData; }
};

} // namespace filament::fg2

#endif //TNT_FILAMENT_FG2_PASS_H
