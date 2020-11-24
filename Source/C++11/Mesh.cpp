#include <LWCore/LWByteBuffer.h>
#include <LWPlatform/LWFileStream.h>
#include <LWVideo/LWVideoBuffer.h>
#include "Mesh.h"
#include "Renderer.h"
#include "Material.h"
#include "Animation.h"
#include <LWEGLTFParser.h>
#include <LWESGeometry3D.h>
#include <cassert>
#include "Logger.h"
#include "Camera.h"

//Primitive

//BoneW
LWUTF8Iterator Bone::GetName(void) const {
	return m_Name;
}

Bone::Bone(const LWUTF8Iterator &Name, const LWSMatrix4f &InvBindMatrix, const LWSMatrix4f &Transform) : m_InvBindMatrix(InvBindMatrix), m_Transform(Transform) {
	Name.Copy(m_Name, sizeof(m_Name));
	m_NameHash = GetName().Hash();
}

//MeshGeometry

bool MeshGeometry::UploadData(Renderer *R, LWAllocator &Allocator, bool CopyOut) {
	if (!m_Count) return true;
	uint32_t r = R->PushPendingGeometry(m_ID, m_BufferType, m_Data, m_Count, m_TypeSize, Allocator, CopyOut);
	if (!r) return false;
	m_ID = r;
	if (!CopyOut) m_Data = nullptr;
	return true;
}

MeshGeometry::MeshGeometry(char *Data, uint32_t BufferType, uint32_t TypeSize, uint32_t Count) : m_Data(Data), m_BufferType(BufferType), m_TypeSize(TypeSize), m_Count(Count) {}

MeshGeometry::~MeshGeometry(){
	LWAllocator::Destroy(m_Data);
}


//Meshs
Mesh &Mesh::MakeGLTFMesh(LWEGLTFParser &P, LWEGLTFNode *Source, LWEGLTFMesh *Mesh, LWAllocator &Allocator) {
	uint32_t TotalVertices = 0;
	uint32_t TotalIndices = 0;
	uint32_t VerticeSize = sizeof(GStaticVertice);
	uint32_t IndiceSize = 0;
	for (auto &&Prim : Mesh->m_Primitives) {
		if (Prim.FindAttributeAccessor(LWEGLTFAttribute::JOINTS_0) != -1) VerticeSize = sizeof(GSkeletonVertice);
		LWEGLTFAccessor *PosAccessor = P.GetAccessor(Prim.FindAttributeAccessor(LWEGLTFAttribute::POSITION));
		LWEGLTFAccessor *IdxAccessor = P.GetAccessor(Prim.m_IndiceID);
		if (PosAccessor) TotalVertices += PosAccessor->m_Count;
		if (IdxAccessor) TotalIndices += IdxAccessor->m_Count;
	}
	IndiceSize = TotalVertices <= 0xFFFF ? sizeof(uint16_t) : sizeof(uint32_t);
	char *Verts = nullptr;
	char *Idxs = nullptr;
	if (TotalVertices) {
		if (VerticeSize == sizeof(GStaticVertice)) Verts = (char*)Allocator.Allocate<GStaticVertice>(TotalVertices);
		else Verts = (char*)Allocator.Allocate<GSkeletonVertice>(TotalVertices);
	}
	if (TotalIndices) Idxs = Allocator.Allocate<char>(IndiceSize*TotalIndices);
	uint32_t v = 0;
	uint32_t o = 0;
	for (auto &&Prim : Mesh->m_Primitives) {
		Primitive Pm = { Idxs ? o : v, 0 };
		uint32_t VertCnt = 0;
		uint32_t IndiceCnt = 0;
		char *V = Verts + (VerticeSize*v);
		char *I = Idxs + (IndiceSize*o);
		LWEGLTFAccessorView Position;
		LWEGLTFAccessorView TexCoord;
		LWEGLTFAccessorView Tangent;
		LWEGLTFAccessorView Normal;
		LWEGLTFAccessorView BoneWeight;
		LWEGLTFAccessorView BoneIndices;
		LWEGLTFAccessorView Indice;
		if (P.CreateAccessorView(Position, Prim.FindAttributeAccessor(LWEGLTFAttribute::POSITION))) {
			Position.ReadValues<float>((float*)(V + offsetof(GSkeletonVertice, m_Position)), VerticeSize, Position.m_Count);
			VertCnt = Pm.m_Count = Position.m_Count;
		}
		if (P.CreateAccessorView(TexCoord, Prim.FindAttributeAccessor(LWEGLTFAttribute::TEXCOORD_0))) {
			TexCoord.ReadValues<float>((float*)(V + offsetof(GSkeletonVertice, m_TexCoord)), VerticeSize, TexCoord.m_Count);
		}
		if (P.CreateAccessorView(Normal, Prim.FindAttributeAccessor(LWEGLTFAttribute::NORMAL))) {
			Normal.ReadValues<float>((float*)(V + offsetof(GSkeletonVertice, m_Normal)), VerticeSize, Normal.m_Count);
		}
		if (P.CreateAccessorView(Tangent, Prim.FindAttributeAccessor(LWEGLTFAttribute::TANGENT))) {
			Tangent.ReadValues<float>((float*)(V + offsetof(GSkeletonVertice, m_Tangent)), VerticeSize, Tangent.m_Count);
		} else {
			LogWarn("Model has no tangents, attempting to generate them.");
			for (uint32_t i = 0; i < VertCnt; i++) {
				GStaticVertice *Vt = (GStaticVertice*)(V + (VerticeSize*i));
				LWVector3f R;
				LWVector3f U;
				Vt->m_Normal.xyz().Othogonal(R, U);
				Vt->m_Tangent = LWVector4f(R, 1.0f);
			}
		}
		if (P.CreateAccessorView(BoneWeight, Prim.FindAttributeAccessor(LWEGLTFAttribute::WEIGHTS_0))) {
			BoneWeight.ReadValues<float>((float*)(V + offsetof(GSkeletonVertice, m_BoneWeights)), VerticeSize, BoneWeight.m_Count);
		}
		if (P.CreateAccessorView(BoneIndices, Prim.FindAttributeAccessor(LWEGLTFAttribute::JOINTS_0))) {
			BoneIndices.ReadValues<int32_t>((int32_t*)(V + offsetof(GSkeletonVertice, m_BoneIndices)), VerticeSize, BoneIndices.m_Count);
		}
		if (P.CreateAccessorView(Indice, Prim.m_IndiceID)) {
			if (IndiceSize == sizeof(uint16_t)) {
				Indice.ReadValues<uint16_t>((uint16_t*)I, IndiceSize, Indice.m_Count);
				for (uint32_t n = 0; n < Indice.m_Count; n++) {
					*(uint16_t*)(I + (n*IndiceSize)) += v;
				}
			} else {
				Indice.ReadValues<uint32_t>((uint32_t*)I, IndiceSize, Indice.m_Count);
				for (uint32_t n = 0; n < Indice.m_Count; n++) {
					*(uint32_t*)(I + (n*IndiceSize)) += v;
				}
			}
			IndiceCnt = Pm.m_Count = Indice.m_Count;
		}
		v += VertCnt;
		o += IndiceCnt;
		PushPrimitive(Pm);
	}
	new (&m_Vertices) MeshGeometry(Verts, LWVideoBuffer::Vertex, VerticeSize, TotalVertices);
	new (&m_Indices) MeshGeometry(Idxs, (uint32_t)(IndiceSize == sizeof(uint16_t) ? LWVideoBuffer::Index16 : LWVideoBuffer::Index32), IndiceSize, TotalIndices);
	return *this;
}

Mesh &Mesh::MakeGLTFSkin(LWEGLTFParser &P, LWEGLTFNode *Source, LWEGLTFSkin *Skin, LWAllocator &Allocator) {
	if (!Skin->m_JointList.size()) return *this;
	if (Skin->m_JointList.size() > MaxBones) LogWarn(LWUTF8I::Fmt<128>("Error importing model with more bones than supported: {} ({})", (uint32_t)Skin->m_JointList.size(), MaxBones));
	LWMatrix4f InvBindMatrixs[MaxBones];
	m_BoneCount = std::min<uint32_t>((uint32_t)Skin->m_JointList.size(), MaxBones);
	LWEGLTFAccessorView InvBindView;
	if (P.CreateAccessorView(InvBindView, Skin->m_InverseBindMatrices)) {
		InvBindView.ReadValues<float>(&InvBindMatrixs[0].m_Rows[0].x, sizeof(LWMatrix4f), m_BoneCount);
	}

	auto MapIDToList = [](std::vector<uint32_t> &List, uint32_t ID)->uint32_t {
		for (uint32_t i = 0; i < (uint32_t)List.size(); i++) {
			if (List[i] == ID) return i;
		}
		return -1;
	};

	std::function<void(LWEGLTFNode *, LWEGLTFSkin*)> ParseJointNode = [this, &P, &InvBindMatrixs, &ParseJointNode, &MapIDToList](LWEGLTFNode *N, LWEGLTFSkin *Skin) {
		uint32_t ID = MapIDToList(Skin->m_JointList, N->m_NodeID);
		if (!*N->m_Name) N->SetName(LWUTF8I::Fmt<64>("Bone_{}", ID));
		m_BoneList[ID] = Bone(N->GetName(), LWSMatrix4f(InvBindMatrixs[ID]), LWSMatrix4f(N->m_TransformMatrix));
		return;
	};

	std::function<void(LWEGLTFNode*, LWEGLTFSkin*)> ParseJointChildren = [this, &P, &ParseJointChildren, &MapIDToList](LWEGLTFNode *N, LWEGLTFSkin *Skin) {
		uint32_t ID = MapIDToList(Skin->m_JointList, N->m_NodeID);
		uint32_t pID = -1;
		for (auto &&Iter : N->m_Children) {
			uint32_t cID = MapIDToList(Skin->m_JointList, Iter);

			if (pID != -1) {
				m_BoneList[pID].m_NextBoneID = cID;
			} else m_BoneList[ID].m_ChildBoneID = cID;
			pID = cID;
		}
	};
	for (auto &&Iter : Skin->m_JointList) ParseJointNode(P.GetNode(Iter), Skin);
	for (auto &&Iter : Skin->m_JointList) ParseJointChildren(P.GetNode(Iter), Skin);

	return *this;
}

Mesh &Mesh::BuildAABB(const LWSMatrix4f &Transform, const LWSMatrix4f *BoneMatrixs) {
	LWSMatrix4f BoneMats[MaxBones];
	if (!m_Vertices.m_Data) return *this;
	if (!m_Vertices.m_Count) return *this;
	auto BlendMatrix = [this](const LWVector4f &BoneWeight, const LWVector4i &BoneIdxs, const LWSMatrix4f *BoneMatrixs) -> LWSMatrix4f {
		LWSMatrix4f Mat = BoneMatrixs[BoneIdxs.x]*BoneWeight.x +
			BoneMatrixs[BoneIdxs.y]*BoneWeight.y +
			BoneMatrixs[BoneIdxs.z]*BoneWeight.z +
			BoneMatrixs[BoneIdxs.w]*BoneWeight.w;
		return Mat;
	};
	if (!BoneMatrixs) {
		BuildBindTransforms(BoneMats);
		BuildRenderMatrixs(BoneMats, BoneMats);
	} else BuildRenderMatrixs(BoneMatrixs, BoneMats);

	GSkeletonVertice *Vt = (GSkeletonVertice*)m_Vertices.m_Data;
	LWSVector4f P = LWSVector4f(Vt->m_Position);
	if (m_BoneCount) P = P * BlendMatrix(Vt->m_BoneWeights, Vt->m_BoneIndices, BoneMats);
	P = P * Transform;
	m_MinBounds = m_MaxBounds = P;
	for (uint32_t i = 1; i < m_Vertices.m_Count; i++) {
		Vt = (GSkeletonVertice *)(m_Vertices.m_Data + m_Vertices.m_TypeSize*i);
		P = LWSVector4f(Vt->m_Position);
		if (m_BoneCount) P = P * BlendMatrix(Vt->m_BoneWeights, Vt->m_BoneIndices, BoneMats);
		P = P * Transform;
		m_MinBounds = m_MinBounds.Min(P);
		m_MaxBounds = m_MaxBounds.Max(P);
	}
	return *this;
}


LWVector4i Mesh::BuildBounds(const LWSMatrix4f &Transform, const LWSMatrix4f *BoneMatrixs, const LWVector2f &WndSize, const LWSMatrix4f &ProjViewMatrix, LWSVector4f &BoundsMin, LWSVector4f &BoundsMax){
	LWSMatrix4f BoneMats[MaxBones];
	if (!m_Vertices.m_Data) return LWVector4i(0);
	if (!m_Vertices.m_Count) return LWVector4i(0);
	auto BlendMatrix = [this](const LWVector4f &BoneWeight, const LWVector4i &BoneIdxs, const LWSMatrix4f *BoneMatrixs) -> LWSMatrix4f {
		LWSMatrix4f Mat = BoneMatrixs[BoneIdxs.x] * BoneWeight.x +
			BoneMatrixs[BoneIdxs.y] * BoneWeight.y +
			BoneMatrixs[BoneIdxs.z] * BoneWeight.z +
			BoneMatrixs[BoneIdxs.w] * BoneWeight.w;
		return Mat;
	};
	auto Project = [this](const LWSVector4f &Pnt, const LWSMatrix4f &ViewProjMatrix, const LWVector2f &WndSize, LWSVector4f &Res)->bool {
		LWSVector4f P = Pnt * ViewProjMatrix;
		float w = P.w;
		if (fabs(w) <= std::numeric_limits<float>::epsilon()) return false;
		w = 1.0f / w;
		P *= w;
		Res = (P * LWSVector4f(0.5f, 0.5f, 1.0f, 1.0f) + LWSVector4f(0.5f, 0.5f, 1.0f, 0.0f)) * LWSVector4f(WndSize.x, WndSize.y, 0.5f, 1.0f);
		return true;
	};

	if (!BoneMatrixs) {
		BuildBindTransforms(BoneMats);
		BuildRenderMatrixs(BoneMats, BoneMats);
	} else BuildRenderMatrixs(BoneMatrixs, BoneMats);

	LWSVector4f Min, Max;
	GSkeletonVertice *Vt = (GSkeletonVertice*)m_Vertices.m_Data;
	LWSVector4f P = LWSVector4f(Vt->m_Position);
	if (m_BoneCount) P = P * BlendMatrix(Vt->m_BoneWeights, Vt->m_BoneIndices, BoneMats);
	P = P * Transform;
	Project(P, ProjViewMatrix, WndSize, Min);
	BoundsMin = BoundsMax = P;
	Max = Min;
	
	for (uint32_t i = 1; i < m_Vertices.m_Count; i++) {
		Vt = (GSkeletonVertice *)(m_Vertices.m_Data + m_Vertices.m_TypeSize * i);
		P = LWSVector4f(Vt->m_Position);
		if (m_BoneCount) P = P * BlendMatrix(Vt->m_BoneWeights, Vt->m_BoneIndices, BoneMats);
		P = P * Transform;
		LWSVector4f Res;
		Project(P, ProjViewMatrix, WndSize, Res);
		BoundsMin = BoundsMin.Min(P);
		BoundsMax = BoundsMax.Max(P);
		Min = Min.Min(Res);
		Max = Max.Max(Res);
	}
	return LWVector4i(Min.AsVec4().xy().CastTo<int32_t>(), Max.AsVec4().xy().CastTo<int32_t>());
}

Mesh &Mesh::PushPrimitive(const Primitive &P) {
	m_PrimitiveList.push_back(P);
	return *this;
}

Mesh &Mesh::TransformBounds(const LWSMatrix4f &Transform, LWSVector4f &MinBoundsRes, LWSVector4f &MaxBoundsRes) {
	LWETransformAABB(m_MinBounds, m_MaxBounds, Transform, MinBoundsRes, MaxBoundsRes);
	return *this;
}

Mesh &Mesh::ApplyTransformToBone(uint32_t BoneID, const LWSMatrix4f &Transform, LWSMatrix4f *TransformMatrixs) {
	std::function<void( uint32_t)> DoTransform = [this, &DoTransform, &TransformMatrixs, &Transform](uint32_t i) {
		if (i == -1) return;
		Bone &B = m_BoneList[i];
		TransformMatrixs[i] *= Transform;
		DoTransform(B.m_ChildBoneID);
		DoTransform(B.m_NextBoneID);
	};

	Bone &B = m_BoneList[BoneID];
	TransformMatrixs[BoneID] *= Transform;
	DoTransform(B.m_ChildBoneID);
	return *this;
}

Mesh &Mesh::ApplyRotationTransformToBone(uint32_t BoneID, const LWSMatrix4f &Transform, LWSMatrix4f *TransformMatrixs) {
	LWSVector4f Pos = TransformMatrixs[BoneID][3];
	LWSMatrix4f Matrix = LWSMatrix4f::Translation(-Pos) * Transform;
	LWSVector4f R0 = Matrix[0];
	LWSVector4f R1 = Matrix[1];
	LWSVector4f R2 = Matrix[2];
	LWSVector4f R3 = Matrix[3];
	R3 += Pos.AAAB(LWSVector4f());
	return ApplyTransformToBone(BoneID, LWSMatrix4f(R0, R1, R2, R3), TransformMatrixs);
}

uint32_t Mesh::BuildBindTransforms(LWSMatrix4f *TransformMatrixs) {
	if (!m_BoneCount) return 0;
	std::function<void(const LWSMatrix4f &, uint32_t)> DoTransform = [this, &DoTransform, &TransformMatrixs](const LWSMatrix4f &PTransform, uint32_t i) {
		if (i == -1) return;
		Bone &B = m_BoneList[i];
		TransformMatrixs[i] = B.m_Transform*PTransform;
		DoTransform(TransformMatrixs[i], B.m_ChildBoneID);
		DoTransform(PTransform, B.m_NextBoneID);
		return;
	};
	DoTransform(LWSMatrix4f(), 0);
	return m_BoneCount;
}

uint32_t Mesh::BuildAnimationTransform(AnimationInstance &Instance, LWSMatrix4f *TransformMatrixs) {
	return BuildAnimationTransform(Instance.m_Animation ? Instance.m_TransformMatrixs : nullptr, TransformMatrixs);
}

uint32_t Mesh::BuildAnimationTransform(const LWSMatrix4f *AnimTransforms, LWSMatrix4f *TransformMatrixs) {
	if (!AnimTransforms) return BuildBindTransforms(TransformMatrixs);
	std::function<void(const LWSMatrix4f &, uint32_t)> DoTransform = [this, &DoTransform, &AnimTransforms, &TransformMatrixs](const LWSMatrix4f &PTransform, uint32_t i) {
		if (i == -1) return;
		Bone &B = m_BoneList[i];
		TransformMatrixs[i] = AnimTransforms[i] * PTransform;
		DoTransform(TransformMatrixs[i], B.m_ChildBoneID);
		DoTransform(PTransform, B.m_NextBoneID);
		return;
	};
	DoTransform(LWSMatrix4f(), 0);
	return m_BoneCount;
}

uint32_t Mesh::BuildRenderMatrixs(const LWSMatrix4f *TransformMatrixs, LWSMatrix4f *RenderMatrixs){
	for (uint32_t i = 0; i < m_BoneCount; i++) RenderMatrixs[i] = m_BoneList[i].m_InvBindMatrix * TransformMatrixs[i];
	return m_BoneCount;
}

LWSVector4f Mesh::GetMinBounds(void) const {
	return m_MinBounds;
}

LWSVector4f Mesh::GetMaxBounds(void) const {
	return m_MaxBounds;
}

Bone &Mesh::GetBone(uint32_t i) {
	return m_BoneList[i];
}

uint32_t Mesh::FindBone(uint32_t NameHash) const {
	for (uint32_t i = 0; i < m_BoneCount; i++) {
		if (m_BoneList[i].m_NameHash == NameHash) return i;
	}
	return -1;
}

uint32_t Mesh::FindBone(const LWUTF8Iterator &Name) const {
	return FindBone(Name.Hash());
}

uint32_t Mesh::GetBoneCount(void) const {
	return m_BoneCount;
}

MeshGeometry &Mesh::GetVertices(void) {
	return m_Vertices;
}

MeshGeometry &Mesh::GetIndices(void) {
	return m_Indices;
}

Primitive &Mesh::GetPrimitive(uint32_t i) {
	return m_PrimitiveList[i];
}

uint32_t Mesh::GetPrimitiveCount(void) const {
	return (uint32_t)m_PrimitiveList.size();
}

Mesh::Mesh(uint32_t PrimitiveCount, uint32_t BoneCount, const LWSVector4f &MinBounds, const LWSVector4f &MaxBounds) : m_BoneCount(BoneCount), m_MinBounds(MinBounds), m_MaxBounds(MaxBounds) {
	m_PrimitiveList.reserve(PrimitiveCount);
}