// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Copyright (c) 2008-2022 NVIDIA Corporation. All rights reserved.


#ifndef UI_HELPERS_H
#define UI_HELPERS_H

#include "imgui.h"
#include "PxVec3.h"


static void ImGui_DragFloat3Dir(const char* label, float v[3])
{
    if (ImGui::Button("Normalize"))
    {
        ((physx::PxVec3*)v)->normalize();
    }
    ImGui::SameLine();
    ImGui::DragFloat3(label, v);
};


#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

template<int valuesCount = 90>
class PlotLinesInstance
{
public:
    PlotLinesInstance()
    {
        memset(m_values, 0, sizeof(float) * valuesCount);
    }

    void plot(const char* label, float newValue, const char* overlay_text, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0, 80))
    {
        for (; ImGui::GetTime() > m_time + 1.0f / 60.0f; m_time += 1.0f / 60.0f)
        {
            m_values[m_offset] = newValue;
            m_offset = (m_offset + 1) % valuesCount;
        }
        ImGui::PlotLines(label, m_values, valuesCount, m_offset, overlay_text, scale_min, scale_max, graph_size);
    }

private:
    float m_values[valuesCount];
    int m_offset;
    float m_time = ImGui::GetTime();
};

#endif //UI_HELPERS_H