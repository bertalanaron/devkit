#include <devkit/gfx/vertex_sink.h>

void dk::gfx::VertexSink::clear()
{
    for (int i = 0; i < s_primitiveCount; ++i) {
        auto& tdemux    = m_demux.at(i);
        if (!tdemux)
            continue;

        for (auto& vb : *tdemux->global())
            vb.second.modify().clear();
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
            shader.layout(vb.second);
            frameBuffer.render(shader, vb.second, primitive);
        }
    }
}

void dk::gfx::VertexSink::flush(Shader& shader, FrameBuffer& frameBuffer)
{
    draw(shader, frameBuffer);
    clear();
}
