#ifndef MODEL_H
#define MODEL_H
#include <LWCore/LWTypes.h>
#include <LWCore/LWMatrix.h>
#include <LWCore/LWVector.h>
#include <LWCore/LWSMatrix.h>
#include <LWCore/LWSVector.h>
#include <LWETypes.h>
#include <LWEGLTFParser.h>
#include <vector>

struct AnimationInstance;

class Renderer;

class Camera;

struct Primitive {
	uint32_t m_Offset;
	uint32_t m_Count;
};

struct Bone {
	static const uint32_t MaxNameLen = 32;
	char m_Name[MaxNameLen];
	LWSMatrix4f m_InvBindMatrix;
	LWSMatrix4f m_Transform;
	uint32_t m_NameHash;
	uint32_t m_NextBoneID = -1;
	uint32_t m_ChildBoneID = -1;
	uint32_t m_Pad0;

	Bone() = default;
};

struct MeshGeometry {
	char *m_Data = nullptr;
	uint32_t m_ID = 0;
	uint32_t m_BufferType = 0;
	uint32_t m_TypeSize = 0;
	uint32_t m_Count = 0;

	bool UploadData(Renderer *R, LWAllocator &Allocator, bool CopyOut);

	MeshGeometry(char *Data, uint32_t BufferType, uint32_t TypeSize, uint32_t Count);

	MeshGeometry() = default;

	~MeshGeometry();
};

class Mesh {
public:
	static const uint32_t MeshHeaderID = 0xF9F8F7F6;
	static const uint32_t MeshVersionID = 0x1;
	static const uint32_t MaxBones = 32;

	Mesh &MakeGLTFMesh(LWEGLTFParser &P, LWEGLTFNode *Source, LWEGLTFMesh *GMesh, LWAllocator &Allocator);

	Mesh &MakeGLTFSkin(LWEGLTFParser &P, LWEGLTFNode *Source, LWEGLTFSkin *Skin, LWAllocator &Allocator);

	Mesh &BuildAABB(const LWSMatrix4f &Transform, const LWSMatrix4f *BoneMatrixs);

	//Constructs a tight 2d aabb of the model as it appears on the screen, and the 3D bounding volume in BoundsMin, and BoundsMax.  Possible performance issues with hp models.  screen bounds is min(xy)+max(zw) bounds.
	LWVector4i BuildBounds(const LWSMatrix4f &Transform, const LWSMatrix4f *BoneMatrix, const LWVector2f &WndSize, const LWSMatrix4f &ProjViewMatrix, LWSVector4f &BoundsMin, LWSVector4f &BoundsMax);

	Mesh &PushPrimitive(const Primitive &P);

	Mesh &TransformBounds(const LWSMatrix4f &Transform, LWSVector4f &MinBoundsRes, LWSVector4f &MaxBoundsRes);

	Mesh &ApplyRotationTransformToBone(uint32_t BoneID, const LWSMatrix4f &Transform, LWSMatrix4f *TransformMatrixs);

	Mesh &ApplyTransformToBone(uint32_t BoneID, const LWSMatrix4f &Transform, LWSMatrix4f *TransformMatrixs);

	uint32_t BuildBindTransforms(LWSMatrix4f *TransformMatrixs);

	uint32_t BuildAnimationTransform(AnimationInstance &Instance, LWSMatrix4f *TransformMatrixs);
	
	uint32_t BuildAnimationTransform(const LWSMatrix4f *AnimTransforms, LWSMatrix4f *TransformMatrixs);

	uint32_t BuildRenderMatrixs(const LWSMatrix4f *TransformMatrixs, LWSMatrix4f *RenderMatrixs);

	MeshGeometry &GetVertices(void);

	MeshGeometry &GetIndices(void);

	LWSVector4f GetMinBounds(void) const;

	LWSVector4f GetMaxBounds(void) const;

	Primitive &GetPrimitive(uint32_t i);

	Bone &GetBone(uint32_t i);

	uint32_t FindBone(uint32_t NameHash) const;

	uint32_t FindBone(const char *Name) const;

	uint32_t GetBoneCount(void) const;

	uint32_t GetPrimitiveCount(void) const;

	Mesh() = default;

	Mesh(uint32_t PrimitiveCount, uint32_t BoneCount, const LWSVector4f &MinBounds, const LWSVector4f &MaxBounds);

private:
	std::vector<Primitive> m_PrimitiveList;
	Bone m_BoneList[MaxBones];
	MeshGeometry m_Vertices;
	MeshGeometry m_Indices;
	LWSVector4f m_MinBounds;
	LWSVector4f m_MaxBounds;
	uint32_t m_BoneCount = 0;
};


#endif