/*
* Copyright (c) 2008-2015, NVIDIA CORPORATION.  All rights reserved.
*
* NVIDIA CORPORATION and its licensors retain all intellectual property
* and proprietary rights in and to this software, related documentation
* and any modifications thereto.  Any use, reproduction, disclosure or
* distribution of this software and related documentation without an express
* license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

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