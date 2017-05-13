/*
 * profiler.cpp
 *
 *  Created on: Nov 26, 2016
 *      Author: nullifiedcat
 */

#include "profiler.h"
#include "cvwrapper.h"
#include "logging.h"

ProfilerSection::ProfilerSection(std::string name, ProfilerSection* parent) {
	m_name = name;
	m_calls = 0;
	m_log = std::chrono::high_resolution_clock::now();
	m_min = std::chrono::nanoseconds::zero();
	m_max = std::chrono::nanoseconds::zero();
	m_sum = std::chrono::nanoseconds::zero();
	m_parent = parent;
}

static CatVar do_profiler_logging(CV_SWITCH, "profiler_log", "0", "Profiler Log");

void ProfilerSection::OnNodeDeath(ProfilerNode& node) {
	auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - node.m_start);
	if (m_min == std::chrono::nanoseconds::zero()) m_min = dur;
	else if (dur < m_min) m_min = dur;

	if (m_max == std::chrono::nanoseconds::zero()) m_max = dur;
	else if (dur > m_max) m_max = dur;
	m_sum += dur;
	m_calls++;

	if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - m_log).count() > 3) {
		if (do_profiler_logging)
			logging::Info("[P],'%-32s',%12llu,%12llu,%12llu,%12llu,%u", m_name.c_str(),
					std::chrono::duration_cast<std::chrono::nanoseconds>(m_sum).count(),
					std::chrono::duration_cast<std::chrono::nanoseconds>(m_sum).count() / (m_calls ? m_calls : 1),
					std::chrono::duration_cast<std::chrono::nanoseconds>(m_min).count(),
					std::chrono::duration_cast<std::chrono::nanoseconds>(m_max).count(),
					m_calls);
		m_log = std::chrono::high_resolution_clock::now();
		m_min = std::chrono::nanoseconds::zero();
		m_max = std::chrono::nanoseconds::zero();
		m_sum = std::chrono::nanoseconds::zero();
		m_calls = 0;
	}
}

ProfilerNode::ProfilerNode(ProfilerSection& section) : m_section(section) {
	m_start = std::chrono::high_resolution_clock::now();
}

ProfilerNode::~ProfilerNode() {
	m_section.OnNodeDeath(*this);
}
