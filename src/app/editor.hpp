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
#include <imgui/imoguizmo.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "../utils/utils.hpp"
#include "../scene_graph/scene_graph.hpp"
#include "../scene_graph/mesh_node.hpp"
#include "../renderer/renderer.hpp"
#include "../renderer/model.hpp"
#include "../renderer/camera.hpp"

enum class CopyFlags;

class Editor
{
public:
    Editor(Renderer& renderer, SaveData& saveData);
    ~Editor();

    void update(float dt);

private:
    void imguiEvents();
    void mainMenuBar();
    void cameraPanel();
    void assetPanel();
    void sceneGraphPanel();
    void rendererPanel();
    void inspectorPanel();
    void viewPort();
    void debugPanel();

    void sceneNodeRecursive(GraphNode* node);
    void createModelGraph(Model& model);
    GraphNode* createModelGraphRecursive(Model& model, const SceneNode& sceneNode, GraphNode* parent);
    GraphNode* createEmptyNode(GraphNode* parent, const std::string& name);
    GraphNode* createMeshNode(Model& model, const SceneNode& sceneNode, GraphNode* parent);
    void addDirLight();
    void addPointLight();
    void addSpotLight();
    void copyNode(uuid32_t nodeID);
    void cutNode(uuid32_t nodeID);
    void pasteNode(GraphNode* parent);
    void duplicateNode(GraphNode* node);

    void modelInspector(uuid32_t modelID);
    void matTexInspector(const char* label, const Texture& texture);
    void sceneNodeInspector(GraphNode* node);
    void emptyNodeInspector(GraphNode* node);
    void meshNodeInspector(GraphNode* node);
    void dirLightInspector(GraphNode* node);
    void pointLightInspector(GraphNode* node);
    void spotLightInspector(GraphNode* node);
    bool nodeTransform(GraphNode* node);

    void deleteSelectedObject();
    void deleteSelectedNode();
    void deleteSelectedModel();
    void deleteNode(GraphNode* node);

    void sceneNodeDragDropSource(GraphNode* node);
    void sceneNodeDragDropTarget(GraphNode* node);
    void sceneNodeDragDropTargetWholeWindow(bool nodeHovered);
    void modelDragDropSource(uuid32_t modelID);
    void modelDragDropTargetWholeWindow();

    void sharedPopups();
    void assetPanelPopup();
    void sceneGraphPopup();
    void skyboxImportPopup();
    void modelImportPopup();

    bool transformGizmo();
    void drawAxisGizmo();
    void gizmoOptions();
    void gizmoOpIcon(ImTextureID iconDs, ImGuizmo::OPERATION op, const char* tooltip);
    void gizmoModeIcon(ImTextureID iconDs, ImGuizmo::MODE mode, const char* tooltip);

    void keyboardShortcuts();

    void importModel(const ModelImportData& importData);
    std::filesystem::path selectModelFileDialog();
    void checkPayloadType(const char* type);
    void resetBuffer();
    void helpMarker(const char* desc);
    GraphNode* copyGraphNode(GraphNode* node);
    std::optional<std::string> renameDialog();
    std::optional<uuid32_t> selectModel();
    glm::vec3 spawnPos();
    bool lastItemClicked(uuid32_t id);
    bool isNodeSelected();
    bool isModelSelected();
    void countFPS();
    void updateFrametimeStats();
    void plotPerformanceGraphs();

private:
    Renderer& mRenderer;
    SaveData& mSaveData;
    std::shared_ptr<SceneGraph> mSceneGraph;

    bool mShowViewport = true;
    bool mShowAssetPanel = true;
    bool mShowSceneGraph = true;
    bool mShowCameraPanel = true;
    bool mShowInspectorPanel = true;
    bool mShowRendererPanel = true;
    bool mShowDebugPanel = true;
    bool mModelImportPopup = false;
    bool mViewGizmoControls = true;
    bool mViewAxisGizmo = true;

    ImVec2 mViewportSize;
    uuid32_t mSelectedObjectID;
    float mDt;
    uint32_t mFPS;
    float mFrametimeMs;
    std::array<char, 256> mBuffer;
    CopyFlags mCopyFlag;
    uuid32_t mCopiedNodeID;
    ImGuizmo::MODE mGizmoMode = ImGuizmo::WORLD;
    ImGuizmo::OPERATION mGizmoOp = ImGuizmo::TRANSLATE;
};

enum class CopyFlags
{
    None,
    Copy,
    Cut
};

#endif //VULKANRENDERINGENGINE_EDITOR_HPP
