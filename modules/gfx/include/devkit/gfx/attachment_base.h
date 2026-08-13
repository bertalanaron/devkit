#pragma once
#include <devkit/common/utils.h>

namespace dk::gfx {

class FrameBuffer;

class AttachmentBase {
public:
	glm::ivec2 size() const;

	void resize(const glm::ivec2& size);

	float aspectRatio() const;

protected:
	glm::ivec2 m_size;
	bool       m_resized; // initialy set to true;

	AttachmentBase();
	AttachmentBase(const glm::ivec2& size);

protected:
	virtual void initializeOrUpdate() { assert(false); }
	virtual void attachAs(FrameBuffer& buffer, unsigned underlyingAttachmentIndex) { assert(false); }
	friend class FrameBuffer;
};

class NullAttachment : public AttachmentBase {
public:
	NullAttachment();

private:
	void initializeOrUpdate() override { }
	void attachAs(FrameBuffer& buffer, unsigned underlyingAttachmentIndex) override { }

	friend class FrameBuffer;
};

} // dk::gfx
