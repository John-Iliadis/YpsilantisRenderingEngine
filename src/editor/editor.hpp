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
#include <imgui/ImGuizmo.h>
#include "../utils/utils.hpp"
#include "../resource/resource_manager.hpp"
#include "../scene_graph/scene_graph.hpp"
#include "../scene_graph/light_node.hpp"
#include "../scene_graph/mesh_node.hpp"
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
    void cameraPanel();
    void rendererPanel();
    void console();
    void debugPanel();

    void sceneGraphPanel();
    void sceneGraphPopup();
    void sceneNodeRecursive(SceneNode* node);
    void checkPayloadType(const char* type);
    SceneNode* createModelGraph(std::shared_ptr<Model> model, const Model::Node& modelNode, SceneNode* parent);
    void createEmptyNode();
    void createDirectionalLight();
    void createSpotLight();
    void createPointLight();

    void deleteSelectedNode();
    void deleteSelectedModel();
    void deleteSelectedMaterial();
    void deleteSelectedTexture();

    void assetPanel();
    void displayModels();
    void displayMaterials();
    void displayTextures();

    void inspectorPanel();
    void modelInspector(uuid64_t modelID);
    void materialInspector(uuid64_t materialID);
    bool materialTextureInspector(index_t& textureIndex, std::string label);
    void textureInspector(uuid64_t textureID);
    void sceneNodeInspector(SceneNode* node);
    void emptyNodeInspector(SceneNode* node);
    void meshNodeInspector(SceneNode* node);
    void dirLightInspector(SceneNode* node);
    void pointLightInspector(SceneNode* node);
    void spotLightInspector(SceneNode* node);
    void nodeTransform(SceneNode* node);
    void lightOptions(LightBase* light);
    void lightShadowOptions(LightBase* light);

    void viewPort();

    void sceneNodeDragDropSource(SceneNode* node);
    void sceneNodeDragDropTarget(SceneNode* node);
    void modelDragDropSource(uuid64_t modelID);
    void modelDragDropTarget();
    void textureDragDropSource(uuid64_t textureID);
    bool textureDragDropTarget(index_t& textureIndex);

    bool nodeSelected();

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
