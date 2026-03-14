#include "renderer.h"

void Renderer::Init(int width, int height) {
    target = LoadRenderTexture(width, height);
    hasPostShader = false;
}

void Renderer::BeginFrame() {
    // Could render to target for post-processing
}

void Renderer::EndFrame(int width, int height) {
    // Post-processing pass would go here
}

void Renderer::Shutdown() {
    UnloadRenderTexture(target);
}

void Renderer::DrawSkybox() {
    // Simple gradient sky drawn as background
}
