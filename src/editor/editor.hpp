//
// Created by Gianni on 4/02/2025.
//

#ifndef VULKANRENDERINGENGINE_EDITOR_HPP
#define VULKANRENDERINGENGINE_EDITOR_HPP

#include <glfw/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include "../utils/utils.hpp"
#include "../resource/resource_manager.hpp"
#include "../scene_graph/scene_graph.hpp"
#include "../renderer/model.hpp"
#include "camera.hpp"

class Renderer;
class ResourceManager;

// todo: disable modifications to default resources
// todo: resize camera
// todo: when the selected model gets deleted, assign 0 to mSelectedObjectID
class Editor
{
public:
    Editor(std::shared_ptr<Renderer> renderer, std::shared_ptr<ResourceManager> resourceManager);
    ~Editor();

    void update(float dt);

private:
    void mainMenuBar();
    void assetPanel();
    void sceneGraphPanel();
    void cameraPanel();
    void rendererPanel();
    void console();
    void debugPanel();

    void displayModels();
    void displayMaterials();
    void displayTextures();

    void inspectorPanel();
    void modelInspector(uuid64_t modelID);
    void materialInspector(uuid64_t materialID);
    bool materialTextureInspector(index_t& textureIndex, std::string label);
    void textureInspector(uuid64_t textureID);

    void sceneNodeRecursive(SceneNode* node);
    void checkPayloadType(const char* type);
    SceneNode* createModelGraph(std::shared_ptr<Model> model, const Model::Node& modelNode, SceneNode* parent);

    void viewPort();

    void sceneNodeDragDropSource(SceneNode* node);
    void sceneNodeDragDropTarget(SceneNode* node);
    void modelDragDropSource(uuid64_t modelID);
    void modelDragDropTarget();
    void textureDragDropSource(uuid64_t textureID);
    bool textureDragDropTarget(index_t& textureIndex);

    std::optional<uuid64_t> textureCombo(uuid64_t selectedTextureID);

private:
    std::shared_ptr<Renderer> mRenderer;
    std::shared_ptr<ResourceManager> mResourceManager;

    Camera mCamera;
    SceneGraph mSceneGraph;

    uuid64_t mSelectedObjectID;
    float mDt{};

    bool mShowViewport;
    bool mShowAssetPanel;
    bool mShowSceneGraph;
    bool mShowCameraPanel;
    bool mShowInspectorPanel;
    bool mShowRendererPanel;
    bool mShowConsole;
    bool mShowDebugPanel;
};

#endif //VULKANRENDERINGENGINE_EDITOR_HPP
