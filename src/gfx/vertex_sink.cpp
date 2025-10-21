#include <devkit/gfx/vertex_sink.h>

dk::gfx::VertexSink dk::gfx::VertexSink::create(VertexFlags flags)
{
    return VertexSink(VertexAttributes::get(flags));
}

void dk::gfx::VertexSink::clear()
{
    for (int i = 0; i < s_primitiveCount; ++i) {
        auto&     tdemux    = m_demux.at(i);
        if (!tdemux)
            continue;

        for (auto& vb : *tdemux->global())
            vb.second.clear();
    }
}

void dk::gfx::VertexSink::draw(Shader& shader, FrameBuffer& frameBuffer)
{
    for (int i = 0; i < s_primitiveCount; ++i) {
        Primitive primitive = Primitive(i);
        auto&     tdemux    = m_demux.at(i);
        if (!tdemux)
            continue;

        for (auto& vb : *tdemux->global()) {
            shader.layout(std::ref(vb.second));
            frameBuffer.render(shader, vb.second, primitive);
        }
    }
}

void dk::gfx::VertexSink::flush(Shader& shader, FrameBuffer& frameBuffer)
{
    draw(shader, frameBuffer);
    clear();
}

dk::gfx::VertexBuffer dk::gfx::VertexSink::createVertexBuffer(const VertexAttributes* attributes)
{
    return VertexBuffer::create(attributes);
}

dk::gfx::VertexSink::VertexSink(const VertexAttributes* vertexAttributes)
    : m_vertexAttributes(vertexAttributes)
    , m_demux()
{ 
    std::for_each(m_demux.begin(), m_demux.end(), 
        [&](auto& tdemuxPtr) { 
            tdemuxPtr = std::make_unique<common::ThreadDemuxContainer<VertexBuffer>>(std::bind_front(createVertexBuffer, vertexAttributes)); 
        });
}
