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
#include <glm/gtx/matrix_decompose.hpp>
#include "../utils/utils.hpp"
#include "../scene_graph/scene_graph.hpp"
#include "../scene_graph/light_node.hpp"
#include "../scene_graph/mesh_node.hpp"
#include "../renderer/renderer.hpp"
#include "../renderer/model.hpp"
#include "camera.hpp"

// todo: resize camera
class Editor
{
public:
    Editor(Renderer& renderer);
    ~Editor();

    void update(float dt);

private:
    void imguiEvents();
    void mainMenuBar();
    void cameraPanel();
    void rendererPanel();
    void console();
    void debugPanel();

    void sceneGraphPanel();
    void sceneGraphPopup();
    void sceneNodeRecursive(GraphNode* node);
    void createModelGraph(Model& model);
    void createEmptyNode();
    void createDirectionalLight();
    void createSpotLight();
    void createPointLight();

    void assetPanel();
    void assetPanelPopup();

    void inspectorPanel();
    void modelInspector(uuid32_t modelID);
    void sceneNodeInspector(GraphNode* node);
    void emptyNodeInspector(GraphNode* node);
    void meshNodeInspector(GraphNode* node);
    void dirLightInspector(GraphNode* node);
    void pointLightInspector(GraphNode* node);
    void spotLightInspector(GraphNode* node);
    void nodeTransform(GraphNode* node);
    void lightOptions(LightBase* light);
    void lightShadowOptions(LightBase* light);

    void viewPort();

    void deleteSelectedObject();
    void deleteSelectedNode();
    void deleteSelectedModel();

    void sceneNodeDragDropSource(GraphNode* node);
    void sceneNodeDragDropTarget(GraphNode* node);
    void modelDragDropSource(uuid32_t modelID);
    void modelDragDropTarget();

    void importModel();
    void checkPayloadType(const char* type);
    void resetBuffer();
    std::optional<std::string> renameDialog();
    std::optional<uuid32_t> selectModel();
    bool lastItemClicked(uuid32_t id);
    bool isNodeSelected();
    bool isModelSelected();
    
private:
    Renderer& mRenderer;

    Camera mCamera;
    SceneGraph mSceneGraph;

    uuid32_t mSelectedObjectID;
    float mDt{};
    std::array<char, 256> mBuffer;

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
