# Vertex Array Object Attribute Tracking

OpenGL Specification only states that there must be at least 16 vertex attributres avaliable in every implementation.

I doubt we'd ever reach the 16 limit considering what this is used for, but this is good for tracking anyway.

| Vertex Attribute Number | Type     | Data              | C++ Type  | GLSL Type |
| ----------------------- | -------- | ----------------- | --------- | --------- |
| 1                       | Vertex   | Position          | fvec3     | vec3      |
| 2                       | Vertex   | Vertex Colour     | fvec3     | vec3      |
| 3                       | Instance | Instance Position | fmat4     | mat4      |