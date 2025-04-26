#include "pipeline/pipeline_pad.h"
#include "pipeline/pipeline.h"

namespace lexus2k::pipeline
{
    bool IPad::pushPacket(std::shared_ptr<IPacket> packet, uint32_t timeout) noexcept
    {
        if (m_padType != PadType::INPUT)
        {
            if (m_linkedPad != nullptr)
            {
                return m_linkedPad->pushPacket(packet, timeout);
            }
            else
            {
                return false; // No linked pad available
            }
        }
        return queuePacket(packet, timeout);
    }

    INode& IPad::then(IPad& pad) noexcept
    {
        if (m_padType == PadType::UNDEFINED) {
            m_padType = PadType::OUTPUT;
        }
        if (pad.m_padType == PadType::UNDEFINED) {
            pad.m_padType = PadType::INPUT;
        }
        m_linkedPad = &pad;
        return pad.node();
    }

    bool IPad::processPacket(std::shared_ptr<IPacket> packet, uint32_t timeout) noexcept
    {
        return (node()).processPacket(packet, *this); // Pass reference instead of pointer
    }
}