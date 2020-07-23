#include "Animation.h"
#include <algorithm>

//AnimTween
bool AnimTween::Deserialize(AnimTween &Tween, LWByteBuffer &Buffer) {
	LWETween<LWSVector4f>::Deserialize(Tween.m_Translation, Buffer);
	LWETween<LWSVector4f>::Deserialize(Tween.m_Scale, Buffer);
	LWETween<LWSQuaternionf>::Deserialize(Tween.m_Rotation, Buffer);
	return true;
}

LWSMatrix4f AnimTween::GetFrame(float Time, bool Loop) {
	if (Loop) {
		float Total = GetTotalTime();
		if (Total > 0.0f) Time = (float)fmodf(Time, Total);
	}
	LWSVector4f Trans = m_Translation.GetValue(Time, LWSVector4f(0.0f, 0.0f, 0.0f, 1.0f));
	LWSVector4f Scale = m_Scale.GetValue(Time, LWSVector4f(1.0f));
	LWSQuaternionf Rot = m_Rotation.GetValue(Time);
	return LWSMatrix4f(Scale, Rot, Trans).Transpose3x3();
}

uint32_t AnimTween::Serialize(LWByteBuffer &Buf) {
	uint32_t o = 0;
	o += m_Translation.Serialize(Buf);
	o += m_Scale.Serialize(Buf);
	o += m_Rotation.Serialize(Buf);
	return o;
}

float AnimTween::GetTotalTime(void) {
	return std::max<float>(std::max<float>(m_Translation.GetTotalTime(), m_Scale.GetTotalTime()), m_Rotation.GetTotalTime());
}

AnimTween::AnimTween(const LWEGLTFAnimTween &GFTLAnimTween) {
	const LWETween<LWVector3f> &T = GFTLAnimTween.m_Translation;
	const LWETween<LWVector3f> &S = GFTLAnimTween.m_Scale;
	const LWETween<LWQuaternionf> &R = GFTLAnimTween.m_Rotation;
	m_Translation = LWETween<LWSVector4f>(T.GetInterpolation(), T.GetFrameCount());
	m_Scale = LWETween<LWSVector4f>(T.GetInterpolation(), T.GetFrameCount());
	m_Rotation = LWETween<LWSQuaternionf>(T.GetInterpolation(), T.GetFrameCount());
	uint32_t TCnt = T.GetFrameCount();
	uint32_t SCnt = S.GetFrameCount();
	uint32_t RCnt = R.GetFrameCount();
	for (uint32_t i = 0; i < TCnt; i++) {
		const LWETweenFrame<LWVector3f> &F = T.GetFrame(i);
		if (T.GetInterpolation() == LWETween<LWVector3f>::CUBICSPLINE) {
			m_Translation.Push(LWSVector4f(F.m_Value[0], 1.0f), LWSVector4f(F.m_Value[1], 1.0f), LWSVector4f(F.m_Value[2], 1.0f), F.m_Time);
		} else m_Translation.Push(LWSVector4f(F.m_Value[0], 1.0f), F.m_Time);
	}
	for (uint32_t i = 0; i < SCnt; i++) {
		const LWETweenFrame<LWVector3f> &F = S.GetFrame(i);
		if (S.GetInterpolation() == LWETween<LWVector3f>::CUBICSPLINE) {
			m_Scale.Push(LWSVector4f(F.m_Value[0], 1.0f), LWSVector4f(F.m_Value[1], 1.0f), LWSVector4f(F.m_Value[2], 1.0f), F.m_Time);
		} else m_Scale.Push(LWSVector4f(F.m_Value[0], 1.0f), F.m_Time);
	}
	for (uint32_t i = 0; i < RCnt; i++) {
		const LWETweenFrame<LWQuaternionf> &F = R.GetFrame(i);
		if (R.GetInterpolation() == LWETween<LWQuaternionf>::CUBICSPLINE) {
			m_Rotation.Push(LWSQuaternionf(F.m_Value[0]), LWSQuaternionf(F.m_Value[1]), LWSQuaternionf(F.m_Value[2]), F.m_Time);
		} else m_Rotation.Push(LWSQuaternionf(F.m_Value[0]), F.m_Time);
	}
}

//AnimationInstance
LWSVector4f AnimationInstance::TransformPoint(const LWSVector4f &Pos, const LWSMatrix4f *Transforms, uint32_t Idx) {
	return Pos * Transforms[Idx];
}

LWSVector4f AnimationInstance::TransformPoint(const LWSVector4f &Pos, const LWSMatrix4f *Transforms, const LWVector4i &Indexs, const LWVector4f &Weights) {
	LWSMatrix4f Blended = Transforms[Indexs.x] * Weights.x + Transforms[Indexs.y] * Weights.y + Transforms[Indexs.z] * Weights.z + Transforms[Indexs.w] * Weights.w;
	return Pos * Blended;
}

AnimationInstance &AnimationInstance::Update(float dTime) {
	if (!isPlaying()) return *this;
	return SetTime(m_Time + dTime);
}

AnimationInstance &AnimationInstance::SetTime(float Time) {
	bool isLoop = isLooping();
	m_Time = Time;
	if (!m_Animation) return *this;
	float Total = m_Animation->GetTotalTime();
	if (m_Time >= Total) m_Flag |= Finished;
	if (isLoop) {
		if (Total > 0.0f) m_Time = fmodf(m_Time, Total);
	} else m_Time = std::min<float>(m_Time, Total);
	m_Animation->MakeAnimationTransform(m_Time, isLooping(), m_TransformMatrixs, Mesh::MaxBones);
	return *this;
}

AnimationInstance &AnimationInstance::Play(void) {
	m_Flag |= Playing;
	return *this;
}

AnimationInstance &AnimationInstance::Pause(void) {
	m_Flag &= ~Playing;
	return *this;
}

AnimationInstance &AnimationInstance::SetLooping(bool isLooping) {
	m_Flag = (m_Flag&~Loop) | (isLooping ? Loop : 0);
	return *this;
}

bool AnimationInstance::isLooping(void) const {
	return (m_Flag&Loop) != 0;
}

bool AnimationInstance::isPlaying(void) const {
	return (m_Flag&Playing) != 0;
}

bool AnimationInstance::isFinished(void) const {
	return (m_Flag&Finished) != 0;
}

AnimationInstance::AnimationInstance(Animation *Anim, uint32_t Flags) : m_Animation(Anim), m_Flag(Flags) {
}

//Animation
bool Animation::Deserialize(Animation &Anim, LWByteBuffer &Buf) {
	uint32_t Count = Buf.Read<uint32_t>();
	Anim = Animation(Count);
	Anim.SetNameHash(Buf.Read<uint32_t>());
	for (uint32_t i = 0; i < Count; i++) AnimTween::Deserialize(Anim.GetAnimation(i), Buf);
	Anim.Finished();
	return true;
}

void Animation::MakeGLTFSkin(LWEGLTFParser &P, LWEGLTFSkin *Skin) {
	m_Count = std::min<uint32_t>((uint32_t)Skin->m_JointList.size(), Mesh::MaxBones);
	for(uint32_t i=0;i<m_Count;i++){
		LWEGLTFAnimTween GLTFAnim;
		P.BuildNodeAnimation(GLTFAnim, Skin->m_JointList[i]);
		m_AnimationList[i] = AnimTween(GLTFAnim);
	}
	Finished();
	return;
}

uint32_t Animation::Serialize(LWByteBuffer &Buf) {
	uint32_t o = 0;
	o += Buf.Write<uint32_t>(m_Count);
	o += Buf.Write<uint32_t>(m_NameHash);
	for (uint32_t i = 0; i < m_Count; i++) o += m_AnimationList[i].Serialize(Buf);
	return o;
}

Animation &Animation::Finished(void) {
	float Time = 0;
	for (uint32_t i = 0; i < m_Count; i++) Time = std::max<float>(Time, m_AnimationList[i].GetTotalTime());
	m_TotalTime = Time;
	return *this;
}

Animation &Animation::SetNameHash(uint32_t NameHash) {
	m_NameHash = NameHash;
	return *this;
}

AnimTween &Animation::GetAnimation(uint32_t ID) {
	return m_AnimationList[ID];
}

uint32_t Animation::MakeAnimationTransform(float Time, bool Loop, LWSMatrix4f *TransformMatrixs, uint32_t TransformMatrixCount) {
	if (Loop) {
		if (m_TotalTime>0.0f) Time = fmodf(Time, m_TotalTime);
	}
	uint32_t Cnt = std::min<uint32_t>(m_Count, TransformMatrixCount);
	for (uint32_t i = 0; i < Cnt; i++) TransformMatrixs[i] = m_AnimationList[i].GetFrame(Time);
	return Cnt;
}

uint32_t Animation::MakeBoneTransforms(float Time, bool Loop, Mesh *Msh, LWSMatrix4f *TransformMatrixs) {
	uint32_t BoneCnt = Msh->GetBoneCount();
	if (!BoneCnt) return 0;
	if (Loop) {
		if (m_TotalTime > 0.0f) Time = fmodf(Time, m_TotalTime);
	}
	std::function<void(const LWSMatrix4f &, uint32_t)> DoTransform = [this, &DoTransform, &Msh, &Time, &TransformMatrixs](const LWSMatrix4f &PTransform, uint32_t i) {
		if (i == -1) return;
		Bone &B = Msh->GetBone(i);
		TransformMatrixs[i] = m_AnimationList[i].GetFrame(Time) * PTransform;
		DoTransform(TransformMatrixs[i], B.m_ChildBoneID);
		DoTransform(PTransform, B.m_NextBoneID);
		return;
	};
	DoTransform(LWSMatrix4f(), 0);
	return BoneCnt;
}

uint32_t Animation::GetCount(void) const {
	return m_Count;
}

float Animation::GetTotalTime(void) const {
	return m_TotalTime;
}

uint32_t Animation::GetNameHash(void) const {
	return m_NameHash;
}

Animation::Animation(uint32_t Count) : m_Count(Count) {}