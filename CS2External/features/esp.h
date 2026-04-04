#pragma once
#include "../sdk.h"

namespace ESP {
    void Render(const EntityData* entities, int count, const Matrix4x4& viewMatrix, int screenWidth, int screenHeight);
}
