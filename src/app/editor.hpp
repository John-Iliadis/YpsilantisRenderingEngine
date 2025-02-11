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
#include "../scene_graph/mesh_node.hpp"
#include "../renderer/renderer.hpp"
#include "../renderer/model.hpp"
#include "../renderer/camera.hpp"

enum class CopyFlags;

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
    void debugPanel();

    void sceneGraphPanel();
    void sceneGraphPopup();
    void sceneNodeRecursive(GraphNode* node);
    void createModelGraph(Model& model);
    void createEmptyNode();
    void copyNode(uuid32_t nodeID);
    void cutNode(uuid32_t nodeID);
    void pasteNode(GraphNode* parent);
    void duplicateNode(GraphNode* node);

    void assetPanel();
    void assetPanelPopup();

    void inspectorPanel();
    void modelInspector(uuid32_t modelID);
    void matTexInspector(const char* label, const Texture& texture);
    void sceneNodeInspector(GraphNode* node);
    void emptyNodeInspector(GraphNode* node);
    void meshNodeInspector(GraphNode* node);
    void nodeTransform(GraphNode* node);

    void viewPort();

    void deleteSelectedObject();
    void deleteSelectedNode();
    void deleteSelectedModel();

    void sceneNodeDragDropSource(GraphNode* node);
    void sceneNodeDragDropTarget(GraphNode* node);
    void sceneNodeDragDropTargetWholeWindow(GraphNode* node, bool nodeHovered);
    void modelDragDropSource(uuid32_t modelID);
    void modelDragDropTarget();

    void importModel();
    void checkPayloadType(const char* type);
    void resetBuffer();
    GraphNode* copyGraphNode(GraphNode* node);
    std::optional<std::string> renameDialog();
    std::optional<uuid32_t> selectModel();
    bool lastItemClicked(uuid32_t id);
    bool isNodeSelected();
    bool isModelSelected();
    void countFPS();

private:
    Renderer& mRenderer;
    SceneGraph mSceneGraph;

    bool mShowViewport;
    bool mShowAssetPanel;
    bool mShowSceneGraph;
    bool mShowCameraPanel;
    bool mShowInspectorPanel;
    bool mShowRendererPanel;
    bool mShowDebugPanel;

    ImVec2 mViewportSize;
    uuid32_t mSelectedObjectID;
    float mDt;
    uint32_t mFPS;
    std::array<char, 256> mBuffer;
    CopyFlags mCopyFlag;
    uuid32_t mCopiedNodeID;
};

enum class CopyFlags
{
    None,
    Copy,
    Cut
};

#endif //VULKANRENDERINGENGINE_EDITOR_HPP
