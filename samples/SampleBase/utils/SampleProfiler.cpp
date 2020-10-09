// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2020 NVIDIA Corporation. All rights reserved.


#include "SampleProfiler.h"
#include <map>
#include <iostream>
#include <fstream>
#include <stack>

using namespace std::chrono;

struct ProfileData
{
	steady_clock::time_point start;

	microseconds time;
	microseconds prevTime;
	microseconds maxTime;
	uint32_t calls;
	uint32_t prevCalls;

	ProfileData() : time(0), prevTime(0), maxTime(0), calls(0), prevCalls(0)
	{}
};

struct Node
{
	ProfileData					data;
	std::map<const char*, Node>	childs;
	Node*						parent;
};

static std::map<const char*, Node> s_roots;
static Node*                       s_currentNode;
static bool                        s_beginEndMismatch;
static microseconds                s_overhead;
static microseconds                s_prevOverhead;

void SampleProfilerInit()
{
	s_roots.clear();
	s_currentNode = nullptr;
	s_beginEndMismatch = false;
	s_overhead = microseconds();
}

void SampleProfilerBegin(const char* name)
{
	auto start = steady_clock::now();
	{
		Node* parent = s_currentNode;
		if (s_currentNode == nullptr)
		{
			s_currentNode = &s_roots[name];
		}
		else
		{
			s_currentNode = &s_currentNode->childs[name];
		}
		s_currentNode->parent = parent;
		s_currentNode->data.calls++;
		s_currentNode->data.start = steady_clock::now();
	}
	s_overhead += duration_cast<microseconds>(steady_clock::now() - start);
}

void SampleProfilerEnd()
{
	auto start = steady_clock::now();
	{
		if (s_currentNode)
		{
			auto& data = s_currentNode->data;
			data.time += duration_cast<microseconds>(steady_clock::now() - data.start);
			data.maxTime = data.time > data.maxTime ? data.time : data.maxTime;
			s_currentNode = s_currentNode->parent;
		}
		else
		{
			s_beginEndMismatch = true;
		}
	}
	s_overhead += duration_cast<microseconds>(steady_clock::now() - start);
}

struct SampleProfilerTreeIteratorImpl final : public SampleProfilerTreeIterator
{
	struct StackNode 
	{
		Node* node;
		const char* name;
	};

	SampleProfilerTreeIteratorImpl(std::map<const char*, Node>& roots)
	{
		for (auto& root : roots)
		{
			m_stack.emplace(StackNode { &root.second, root.first });
		}
		
		next();
	}

	virtual const Data* data() const override
	{
		return m_valid ? &m_data : nullptr;
	}

	Node* node()
	{
		return m_node;
	}
	
	virtual bool isDone() const
	{
		return !m_valid;
	}

	virtual void next()
	{
		if (!m_stack.empty())
		{
			auto& e = m_stack.top();
			m_stack.pop();
			m_node = e.node;
			m_data.depth = 0;
			m_data.hash = (uint64_t)m_node;
			for (const Node* p = m_node; p != nullptr; p = p->parent)
			{
				m_data.depth++;
			}
			m_data.name = e.name;
			m_data.calls = m_node->data.prevCalls;
			m_data.time = m_node->data.prevTime;
			m_data.maxTime = m_node->data.maxTime;
			m_data.hasChilds = !m_node->childs.empty();

			for (auto it = m_node->childs.rbegin(); it != m_node->childs.rend(); ++it)
			{
				m_stack.emplace(StackNode { &(*it).second, (*it).first });
			}
			m_valid = true;
		}
		else
		{
			m_valid = false;
		}
	}
	
	virtual void release()
	{
		delete this;
	}

	bool m_valid;
	Data m_data;
	Node* m_node;
	std::stack<StackNode > m_stack;
};

void SampleProfilerReset()
{
	for (SampleProfilerTreeIteratorImpl it(s_roots); !it.isDone(); it.next())
	{
		auto& data = it.node()->data;
		data.prevTime = data.time;
		data.prevCalls = data.calls;
		data.time = microseconds();
		data.calls = 0;
	}
	s_currentNode = nullptr;
	s_beginEndMismatch = false;
	s_prevOverhead = s_overhead;
	s_overhead = microseconds();
}

bool SampleProfilerIsValid()
{
	return !s_beginEndMismatch;
}

microseconds SampleProfilerGetOverhead()
{
	return s_prevOverhead;
}

SampleProfilerTreeIterator* SampleProfilerCreateTreeIterator()
{
	return SampleProfilerIsValid() ? new SampleProfilerTreeIteratorImpl(s_roots) : nullptr;
}

void SampleProfilerDumpToFile(const char* path)
{
	std::ofstream myfile(path, std::ios_base::out);
	if (myfile.is_open())
	{
		if (s_beginEndMismatch)
		{
			myfile << "Error: Begin/End Mismatch.\n";
		}
		else
		{
			myfile << "[Root]\n";
			for(SampleProfilerTreeIteratorImpl it(s_roots); !it.isDone(); it.next())
			{
				auto data = it.data();
				for (uint32_t i = 0; i < data->depth; ++i)
					myfile << "\t";
				myfile << data->name << " --> calls: " << data->calls << ", total: " << data->time.count() * 0.001 << "ms\n";
			}
		}

		myfile.close();
	}
}
