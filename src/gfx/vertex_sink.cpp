#include <devkit/gfx/vertex_sink.h>

dk::gfx::VertexSink dk::gfx::VertexSink::create(VertexFlags flags)
{
    return VertexSink(VertexAttributes::get(flags));
}

void dk::gfx::VertexSink::clear()
{
    for (auto& storage : m_storages) {
        for (auto& buffer : storage.second) {
            if (!buffer)
                continue;
            buffer->clear();
        }
    }
}

void dk::gfx::VertexSink::draw(Shader& shader, FrameBuffer& frameBuffer)
{
    for (auto& storage : m_storages) {
        for (int i = 0; i < s_primitiveCount; ++i) {
            Primitive primitive = magic_enum::enum_cast<Primitive>(i).value();
            auto&     buffer    = storage.second.at(i);

            if (!buffer)
                continue;

            shader.layout(std::ref(*buffer));
            frameBuffer.render(shader, *buffer, primitive);
        }
    }
}

void dk::gfx::VertexSink::flush(Shader& shader, FrameBuffer& frameBuffer)
{
    draw(shader, frameBuffer);
    clear();
}

dk::gfx::VertexSink::VertexSink(const VertexAttributes* vertexAttributes)
    : m_vertexAttributes(vertexAttributes)
    , m_creatorThread(std::this_thread::get_id())
{ }
