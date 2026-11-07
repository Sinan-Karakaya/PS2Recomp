#include "runtime/gs/ps2_gif_arbiter.h"
#include <algorithm>
#include <cstring>

GifArbiter::GifArbiter(ProcessPacketFn processFn)
    : m_processFn(std::move(processFn))
{
}

bool GifArbiter::isImagePacket(const uint8_t *data, uint32_t sizeBytes)
{
    if (!data || sizeBytes < 16u)
        return false;

    uint64_t tagLo = 0;
    std::memcpy(&tagLo, data, sizeof(tagLo));
    const uint8_t flg = static_cast<uint8_t>((tagLo >> 58) & 0x3u);
    return flg == 2u;
}

void GifArbiter::submit(GifPathId pathId, const uint8_t *data, uint32_t sizeBytes, bool path2DirectHl)
{
    if (!data || sizeBytes < 16 || !m_processFn)
        return;

    // drain() sorts PATH1 ahead of every PATH2 and PATH3 packet it holds, and
    // nothing submitted later can move ahead of it, so processing it now gives
    // the same order as queueing it until the next drain. DQ8's VU1 kicks
    // thousands of these a frame; copying and sorting each one was measurable.
    if (pathId == GifPathId::Path1)
    {
        m_processFn(data, sizeBytes);
        return;
    }

    GifArbiterPacket pkt;
    pkt.pathId = pathId;
    pkt.path2DirectHl = (pathId == GifPathId::Path2) && path2DirectHl;
    pkt.path3Image = (pathId == GifPathId::Path3) && isImagePacket(data, sizeBytes);
    pkt.offset = static_cast<uint32_t>(m_bytes.size());
    pkt.size = sizeBytes;
    m_bytes.insert(m_bytes.end(), data, data + sizeBytes);
    m_queue.push_back(pkt);
}

void GifArbiter::drain()
{
    if (!m_processFn)
        return;

    std::stable_sort(m_queue.begin(), m_queue.end(),
                     [](const GifArbiterPacket &a, const GifArbiterPacket &b)
                     {
                         // DIRECTHL cannot preempt PATH3 IMAGE transfers.
                         if (a.path2DirectHl != b.path2DirectHl || a.path3Image != b.path3Image)
                         {
                             if (a.path3Image && b.path2DirectHl)
                                 return true;
                             if (a.path2DirectHl && b.path3Image)
                                 return false;
                         }
                         return pathPriority(a.pathId) < pathPriority(b.pathId);
                     });

    // A packet may submit more while it is processed; those belong to the
    // next drain, so the queue is taken before anything runs.
    std::vector<GifArbiterPacket> queue;
    queue.swap(m_queue);
    std::vector<uint8_t> bytes;
    bytes.swap(m_bytes);
    for (const auto &pkt : queue)
    {
        if (pkt.size != 0u)
            m_processFn(bytes.data() + pkt.offset, pkt.size);
    }
    // Hand the storage back so the next frame reuses the capacity.
    queue.clear();
    bytes.clear();
    if (m_queue.capacity() < queue.capacity())
        m_queue.swap(queue);
    if (m_bytes.capacity() < bytes.capacity())
        m_bytes.swap(bytes);
}

uint8_t GifArbiter::pathPriority(GifPathId id)
{
    return static_cast<uint8_t>(id);
}
