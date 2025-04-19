#include "pipeline/pipeline.h"

namespace lexus2k::pipeline
{
    Pipeline::~Pipeline()
    {
        stop();
    }

    void Pipeline::start() noexcept
    {
        for (auto & node : m_nodes)
        {
            node->_start();
        }
        for (auto & node : m_nodes)
        {
            node->start();
        }
    }

    void Pipeline::stop() noexcept
    {
        for (auto& node: m_nodes)
        {
            node->stop();
        }
        
        for (auto & node : m_nodes)
        {
            node->_stop();
        }
    }
}