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

#include "BackendTest.h"

#include <private/filament/EngineEnums.h>

#include "ShaderGenerator.h"
#include "TrianglePrimitive.h"

#include <fstream>

#ifndef IOS
#include <imageio/ImageEncoder.h>
#include <image/ColorTransform.h>

using namespace image;
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
////////////////////////////////////////////////////////////////////////////////////////////////////

static std::string fullscreenVs = R"(#version 450 core
layout(location = 0) in vec4 mesh_position;
void main() {
    // Hack: move and scale triangle so that it covers entire viewport.
    gl_Position = vec4((mesh_position.xy + 0.5) * 5.0, 0.0, 1.0);
})";

static std::string downsampleFs = R"(#version 450 core
layout(location = 0) out vec4 fragColor;
uniform sampler2D tex;
void main() {
    vec2 texsize = textureSize(tex, 0);
    float sourceLod = 0.0;
    vec2 uv = (gl_FragCoord.xy + 0.5) / (texsize / 2.0);
    fragColor = textureLodOffset(tex, uv, sourceLod, ivec2(-1, -1));
})";

static std::string upsampleFs = R"(#version 450 core
layout(location = 0) out vec4 fragColor;
uniform sampler2D tex;
void main() {
    vec2 texsize = vec2(textureSize(tex, 0));
    float sourceLod = 1.0;
    vec2 uv = (gl_FragCoord.xy + 0.5) / texsize;
    fragColor = textureLodOffset(tex, uv, sourceLod, ivec2(-1, -1));
    fragColor.a = 0.5;
})";

static uint32_t goldenPixelValue = 0;

static const int kTexSize = 512;

namespace test {

using namespace filament;
using namespace filament::backend;

static void dumpScreenshot(DriverApi& dapi, Handle<HwRenderTarget> rt) {
    const size_t size = kTexSize * kTexSize * 4;
    void* buffer = calloc(1, size);
    auto cb = [](void* buffer, size_t size, void* user) {
        int w = kTexSize, h = kTexSize;
        uint32_t* texels = (uint32_t*) buffer;
        goldenPixelValue = texels[0];
        #ifndef IOS
        LinearImage image(w, h, 4);
        image = toLinearWithAlpha<uint8_t>(w, h, w * 4, (uint8_t*) buffer);
        std::ofstream pngstrm("feedback.png", std::ios::binary | std::ios::trunc);
        ImageEncoder::encode(pngstrm, ImageEncoder::Format::PNG, image, "", "feedback.png");
        #endif
    };
    PixelBufferDescriptor pb(buffer, size, PixelDataFormat::RGBA, PixelDataType::UBYTE, cb);
    dapi.readPixels(rt, 0, 0, kTexSize, kTexSize, std::move(pb));
}

TEST_F(BackendTest, FeedbackLoops) {
    // The test is executed within this block scope to force destructors to run before
    // executeCommands().
    {
        // Create a platform-specific SwapChain and make it current.
        auto swapChain = createSwapChain();
        getDriverApi().makeCurrent(swapChain, swapChain);

        // Create programs.
        ProgramHandle downsampleProgram;
        {
            ShaderGenerator shaderGen(fullscreenVs, downsampleFs, sBackend, sIsMobilePlatform);
            Program prog = shaderGen.getProgram();
            Program::Sampler psamplers[] = { utils::CString("tex"), 0, false };
            prog.setSamplerGroup(0, psamplers, sizeof(psamplers) / sizeof(psamplers[0]));
            downsampleProgram = getDriverApi().createProgram(std::move(prog));
        }
        ProgramHandle upsampleProgram;
        {
            ShaderGenerator shaderGen(fullscreenVs, upsampleFs, sBackend, sIsMobilePlatform);
            Program prog = shaderGen.getProgram();
            Program::Sampler psamplers[] = { utils::CString("tex"), 0, false };
            prog.setSamplerGroup(0, psamplers, sizeof(psamplers) / sizeof(psamplers[0]));
            upsampleProgram = getDriverApi().createProgram(std::move(prog));
        }

        TrianglePrimitive triangle(getDriverApi());

        auto defaultRenderTarget = getDriverApi().createDefaultRenderTarget(0);

        // Create a Texture with two miplevels.
        auto usage = TextureUsage::COLOR_ATTACHMENT | TextureUsage::SAMPLEABLE;
        Handle<HwTexture> texture = getDriverApi().createTexture(
                    SamplerType::SAMPLER_2D,            // target
                    2,                                  // levels
                    TextureFormat::RGBA8,               // format
                    1,                                  // samples
                    kTexSize,                           // width
                    kTexSize,                           // height
                    1,                                  // depth
                    usage);                             // usage

        // Create a RenderTarget for each miplevel.
        Handle<HwRenderTarget> renderTargets[2];
        for (uint8_t level = 0; level < 2; level++) {
            renderTargets[level] = getDriverApi().createRenderTarget(
                    TargetBufferFlags::COLOR,
                    kTexSize / 2,                              // width of miplevel
                    kTexSize / 2,                              // height of miplevel
                    1,                                         // samples
                    { texture, level, 0 },                     // color level
                    {},                                        // depth
                    {});                                       // stencil
        }

        // Fill the base level of the texture with interesting colors.
        const size_t size = kTexSize * kTexSize * 4;
        uint8_t* buffer = (uint8_t*) malloc(size);
        for (int r = 0, i = 0; r < kTexSize; r++) {
            for (int c = 0; c < kTexSize; c++, i += 4) {
                buffer[i + 0] = 0x10;
                buffer[i + 1] = 0xff * r / (kTexSize - 1);
                buffer[i + 2] = 0xff * c / (kTexSize - 1);
                buffer[i + 3] = 0xf0;
            }
         }
        auto cb = [](void* buffer, size_t size, void* user) { free(buffer); };
        PixelBufferDescriptor pb(buffer, size, PixelDataFormat::RGBA, PixelDataType::UBYTE, cb);

        // Upload texture data.
        getDriverApi().update2DImage(texture, 0, 0, 0, kTexSize, kTexSize, std::move(pb));

        RenderPassParams params = {};
        params.viewport.left = 0;
        params.viewport.bottom = 0;
        params.flags.clear = TargetBufferFlags::COLOR;
        params.clearColor = {1.f, 0.f, 0.f, 1.f};
        params.flags.discardStart = TargetBufferFlags::ALL;
        params.flags.discardEnd = TargetBufferFlags::NONE;

        PipelineState state;
        state.rasterState.colorWrite = true;
        state.rasterState.depthWrite = false;
        state.rasterState.depthFunc = RasterState::DepthFunc::A;
        state.rasterState.culling = CullingMode::NONE;

        backend::SamplerGroup samplers(1);
        backend::SamplerParams sparams = {};
        sparams.filterMag = SamplerMagFilter::LINEAR;
        sparams.filterMin = SamplerMinFilter::LINEAR_MIPMAP_NEAREST;
        samplers.setSampler(0, texture, sparams);
        auto sgroup = getDriverApi().createSamplerGroup(samplers.getSize());
        getDriverApi().updateSamplerGroup(sgroup, std::move(samplers.toCommandStream()));

        getDriverApi().makeCurrent(swapChain, swapChain);
        getDriverApi().beginFrame(0, 0);
        getDriverApi().bindSamplers(0, sgroup);    

        // Downsample pass.
        state.rasterState.disableBlending();
        params.viewport.width = kTexSize / 2;
        params.viewport.height = kTexSize / 2;
        state.program = downsampleProgram;
        // getDriverApi().setMinMaxLevels(texture, 0, 0);
        getDriverApi().beginRenderPass(renderTargets[1], params);
        getDriverApi().draw(state, triangle.getRenderPrimitive());
        getDriverApi().endRenderPass();

        // Upsample pass.
        state.rasterState.blendFunctionSrcRGB = BlendFunction::SRC_ALPHA;
        state.rasterState.blendFunctionDstRGB = BlendFunction::ONE_MINUS_SRC_ALPHA;
        params.viewport.width = kTexSize;
        params.viewport.height = kTexSize;
        state.program = upsampleProgram;
        // getDriverApi().setMinMaxLevels(texture, 1, 1);
        getDriverApi().beginRenderPass(renderTargets[0], params);
        getDriverApi().draw(state, triangle.getRenderPrimitive());
        getDriverApi().endRenderPass();

        // Read back the current render target.
        dumpScreenshot(getDriverApi(), renderTargets[0]);

        getDriverApi().flush();
        getDriverApi().commit(swapChain);
        getDriverApi().endFrame(0);

        getDriverApi().destroyProgram(downsampleProgram);
        getDriverApi().destroyProgram(upsampleProgram);
        getDriverApi().destroySwapChain(swapChain);
        getDriverApi().destroyRenderTarget(renderTargets[0]);
        getDriverApi().destroyRenderTarget(renderTargets[1]);
        getDriverApi().destroyRenderTarget(defaultRenderTarget);
    }

    getDriverApi().finish();
    executeCommands();
    getDriver().purge();

    const uint32_t expected = 0x80007f87;
    printf("Pixel value is %8.8x, Expected %8.8x\n", goldenPixelValue, expected);
    EXPECT_EQ(goldenPixelValue, expected);
}

} // namespace test
