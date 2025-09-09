# Vertex Array Object Attribute Tracking

OpenGL Specification only states that there must be at least 16 vertex attributres avaliable in every implementation.

I doubt we'd ever reach the 16 limit considering what this is used for, but this is good for tracking anyway.

| Vertex Attribute Number | VAN Zero-Based | Type     | Data                 | C++ Type     | GLSL Type   | Location                                                                          |
| ----------------------- | -------------- | -------- | -------------------- | ------------ | ----------- | --------------------------------------------------------------------------------- |
| 1                       | 0              | Vertex   | Position             | `glm::fvec3` | `vec3`      | `OpenCAGELevelViewer::AllInOne::ContentManager::CMVertex::pos`                    |
| 2                       | 1              | Vertex   | Vertex Colour        | `glm::fvec4` | `vec4`      | `OpenCAGELevelViewer::AllInOne::ContentManager::CMVertex::col`                    |
| 3                       | 2              | Instance | Instance ID          | `uint32`     | `uint`      | `OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL::instanceId`     |
| 4                       | 3              | Instance | Renderable           | `bool`       | `bool`      | `OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL::isRenderable`   |
| 5, 6, 7, 8              | 4, 5, 6, 7     | Instance | Instance Matrix      | `glm::fmat4` | `mat4`      | `OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL::worldMatrix`    |
| 9                       | 8              | Instance | Model Colour         | `glm::fvec4` | `vec4`      | `OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL::materialColour` |
| 10                      | 9              | Instance | Colour Offset        | `glm::fvec4` | `vec4`      | `OpenCAGELevelViewer::AllInOne::ContentManager::ModelReferenceGL::colOffset`      |