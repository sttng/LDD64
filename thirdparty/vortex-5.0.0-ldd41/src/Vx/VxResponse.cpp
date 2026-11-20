// Vortex 5.0.0 build 800 "vortexldd41" - VxCollisionPairRequest and the collision response
// handlers (restored). Mac TU 514 0x476a32-0x477fa7 (file guess
// Vx/VxResponseBody_Body.cpp; its static initialiser 0x477f37 only stores
// MdtGetMaxConstraintSize() into a header static, Mac 0x7800bc), Win 0x722ea0-0x723070 (the
// request) and 0x74c2b0-0x74cf80 (the handlers; Win outlines the Body_Body helpers as
// 0x74c220 / 0x74c310 / 0x74c350 / 0x74c3a0 / 0x74c3f0 / 0x74c440 / 0x74c4c0 / 0x74ccc0 /
// 0x74cd80 / 0x74cec0, the Mac inlines them).
#include "Vx/VxResponse.h"
#include "Vx/VxBridge.h"
#include "Vx/VxCollisionGeometry.h"
#include "Vx/VxContactMaterial.h"
#include "Vx/VxMessage.h"
#include "Vinc/VincIntersect.h"
#include "Vinc/VincModel.h"
#include "Mdt/MdtBody.h"
#include "Mdt/MdtConstraint.h"
#include "Mdt/MdtWorld.h"

#include <math.h>
#include <new>
#include <string.h>
#include <limits>

namespace Vx {

// --------------------------------------------------------------------------------------
// VxCollisionPairRequest

// C2 Mac 0x476c0a (C1 0x476c2c), Win 0x722ea0
VxCollisionPairRequest::VxCollisionPairRequest()
{
    mRecord = (VincRequest*)::operator new(sizeof(VincRequest));
    VincRequestInit(mRecord);
}

// Win 0x722f00
VxCollisionPairRequest::VxCollisionPairRequest(int contactMaxCount, bool faceNormalsFirst)
{
    mRecord = (VincRequest*)::operator new(sizeof(VincRequest));
    memset(mRecord, 0, sizeof(VincRequest));
    VincRequestInit(mRecord);
    mRecord->contactMaxCount = contactMaxCount;
    mRecord->faceNormalsFirst = faceNormalsFirst != 0;
}

// C2 Mac 0x476bb0 (C1 0x476bf0), Win 0x722ec0
VxCollisionPairRequest::VxCollisionPairRequest(const VxCollisionPairRequest& o)
{
    mRecord = (VincRequest*)::operator new(sizeof(VincRequest));
    memset(mRecord, 0, sizeof(VincRequest));
    VincRequestCopy(o.mRecord, mRecord);
}

// D2 Mac 0x476b5c (D1 0x476b72), Win 0x722f70
VxCollisionPairRequest::~VxCollisionPairRequest()
{
    ::operator delete(mRecord);
}

// Mac 0x476b86, Win 0x722f50
VxCollisionPairRequest& VxCollisionPairRequest::operator=(const VxCollisionPairRequest& o)
{
    if (this != &o)
        VincRequestCopy(o.mRecord, mRecord);
    return *this;
}

// Mac 0x476aa2, Win 0x722f80
int VxCollisionPairRequest::getContactMaxCount() const
{
    return mRecord->contactMaxCount;
}

// Mac 0x476b14, Win 0x722f90
void VxCollisionPairRequest::setContactMaxCount(int n)
{
    if (n > 0x400) {
        VxWarning(0, "VxCollisionPairRequest::setContactMaxCount: @a maxCount exeeds max count (%d)\n", 0x400);
        n = 0x400;
    }
    mRecord->contactMaxCount = n;
}

// Mac 0x476ac0, Win 0x723060
int VxCollisionPairRequest::getGroupMaxCount() const
{
    return mRecord->contactGroupMaxCount;
}

// Mac 0x476ace, Win 0x723070
void VxCollisionPairRequest::setGroupMaxCount(int n)
{
    if (n > 6) {
        VxWarning(0, "VxCollisionPairRequest::setGroupMaxCount: @a maxCount exeeds max count (%d)\n", 6);
        n = 6;
    }
    mRecord->contactGroupMaxCount = n;
}

// Mac 0x476aae, Win 0x722fe0
void VxCollisionPairRequest::setFaceNormalsFirst(bool b)
{
    mRecord->faceNormalsFirst = b;
}

// Win 0x723010
void VxCollisionPairRequest::setM08(VxReal x)
{
    mRecord->m08 = x;
}

// Win 0x723030
void VxCollisionPairRequest::setM10(VxReal x)
{
    if (0.0 < x && x < 1.0)
        mRecord->m10 = x;
}

// --------------------------------------------------------------------------------------
// the hellos

// Mac 0x476d4c, Win 0x74c2b0: the geometry types' request; no contacts yet
void VxResponseStartBody_Body::respond(VxBridge* bridge, McdModelPair* pair, CxSpace*, MdtWorldID, VxResponseEntry*)
{
    VxCollisionPairRequest& r = bridge->getPairRequest((int)(size_t)pair->model1->m7c, (int)(size_t)pair->model2->m7c);
    VincHelloWithRequest(pair, r.mRecord);
    pair->m24 = 0;
}

// Mac 0x476cc8, Win 0x74c610
void VxResponseStart_Sensor_Body::respond(VxBridge*, McdModelPair* pair, CxSpace*, MdtWorldID, VxResponseEntry* entry)
{
    VincHelloWithRequest(pair, entry->mRequest.mRecord);
    pair->m24 = 0;
}

// Mac 0x476da0, Win 0x74c650
void VxResponseStart_Sensor_Sensor::respond(VxBridge*, McdModelPair* pair, CxSpace*, MdtWorldID, VxResponseEntry* entry)
{
    VincHelloWithRequest(pair, entry->mRequest.mRecord);
    pair->m24 = 0;
}

// Mac 0x476dc8, Win 0x74c630
void VxResponseStart_Sensor_PS::respond(VxBridge*, McdModelPair* pair, CxSpace*, MdtWorldID, VxResponseEntry* entry)
{
    VincHelloWithRequest(pair, entry->mRequest.mRecord);
    pair->m24 = 0;
}

// --------------------------------------------------------------------------------------
// the goodbye

// Mac 0x476cf0, Win 0x74c5c0
void VxResponseEndBody_Body::respond(VxBridge*, McdModelPair* pair, CxSpace*, MdtWorldID, VxResponseEntry*)
{
    MdtContact* c = (MdtContact*)pair->m24;
    while (c != 0) {
        MdtContact* next = (MdtContact*)MdtContactGetNext(&c->head);
        MdtConstraintWakeBodies(&c->head);
        MdtConstraintDisable(&c->head);
        MdtContactDestroy(&c->head);
        c = next;
    }
    pair->m24 = 0;
}

// --------------------------------------------------------------------------------------
// the body-body active response

bool gVxResponseKeepAllContacts = true; // Win 0xabc5d8

namespace {

// the state of an old contact kept across a step (Mac 0x84 bytes, Win 0x88; 100 of them on
// the stack, 99 used) (names inferred)
struct SavedContact {
    MeReal cf0;             // +0x00 contact m_cf0
    MeReal cf8;             // +0x08 contact m_cf8
    int rowStatus[6];       // +0x10 Kea row status
    MeReal lambda0;         // +0x28 Kea normal force
    MeReal b00[3];          // +0x30 contact point in body 1's frame
    MeReal b18[3];          // +0x48 contact point in body 2's frame
    MeReal direction[3];    // +0x60 friction direction
    MdtContact* contact;    // +0x78
    unsigned int flags;     // +0x7c contact m_d00
    bool used;              // +0x80 matched by a new contact
};

enum { kSavedContactMax = 100 };

// Mac 0x476c40 (out of line), Win 0x74c350 (both inlined into the match search Win 0x74cec0)
// (the VxVector3 tolerance compare; name inferred)
inline bool isEqual(const MeReal* a, const MeReal* b, MeReal tol)
{
#if defined(_WIN32)
    return fabs(a[0] - b[0]) < tol && fabs(a[1] - b[1]) < tol && fabs(a[2] - b[2]) < tol;
#else
    // NOTE: differs from the Win form only for NaN in x or y
    if (tol <= fabs(a[0] - b[0]) || tol <= fabs(a[1] - b[1]))
        return false;
    return fabs(a[2] - b[2]) < tol;
#endif
}

// Win 0x74c310: the old contacts are not all resting (a contact without flag bit 0, or with
// row 1 or 2 at a bound); true without contacts (name inferred)
bool contactsActive(const MdtContact* c)
{
    if (c == 0)
        return true;
    for (; c != 0; c = c->next) {
        if ((c->m_d00 & 1) == 0)
            return true;
        if ((c->head.bcl.rowStatus[1] & 1) != 0)
            return true;
        if ((c->head.bcl.rowStatus[2] & 1) != 0)
            return true;
    }
    return false;
}

// Win 0x74ccc0
void save(SavedContact& s, MdtContact* c)
{
    s.contact = c;
    s.cf0 = c->m_cf0;
    s.cf8 = c->m_cf8;
    for (int k = 0; k < 3; ++k) {
        s.b00[k] = c->m_b00[k];
        s.b18[k] = c->m_b18[k];
        s.direction[k] = c->direction[k];
    }
    for (int k = 0; k < 6; ++k)
        s.rowStatus[k] = c->head.bcl.rowStatus[k];
    s.lambda0 = c->head.bcl.lambda[0];
    s.flags = (unsigned int)c->m_d00;
    s.used = false;
}

// Win 0x74c3f0: every contact (up to max - 1)
int saveAll(MdtContact* c, SavedContact* buf, int max)
{
    int n = 0;
    for (; c != 0; c = c->next)
        if (n < max - 1)
            save(buf[n++], c);
    return n;
}

// Win 0x74c3a0: the contacts without flag bit 1 (up to max - 1)
int saveActive(MdtContact* c, SavedContact* buf, int max)
{
    int n = 0;
    for (; c != 0; c = c->next)
        if ((c->m_d00 & 2) == 0 && n < max - 1)
            save(buf[n++], c);
    return n;
}

// Win 0x74cec0: the first unused old contact at the same point in either body's frame
int findSaved(SavedContact* buf, int n, const MeReal* p1, const MeReal* p2, MeReal tol)
{
    for (int i = 0; i < n; ++i) {
        SavedContact& s = buf[i];
        if (!s.used) {
            bool same = isEqual(s.b00, p1, tol) || isEqual(s.b18, p2, tol);
            s.used = same;
            if (same)
                return i;
        }
    }
    return -1;
}

// Win 0x74cd80: the new contact continues the old one
void restore(MdtContact* c, const SavedContact& s)
{
    if (s.contact == c)
        return;
    for (int k = 0; k < 3; ++k) {
        c->m_b00[k] = s.b00[k];
        c->m_b18[k] = s.b18[k];
    }
    for (int k = 0; k < 6; ++k)
        c->head.bcl.rowStatus[k] = s.rowStatus[k];
    c->head.bcl.lambda[0] = s.lambda0;
    if ((s.flags & 4) == 0) {
        c->m_cf0 = 0;
        c->m_cf8 = 0;
        return;
    }
    c->m_cf0 = s.cf0;
    c->m_cf8 = s.cf8;
    // the old friction direction projected onto the new contact plane: n x (d x n)
    const MeReal* n = c->normal;
    const MeReal* d = s.direction;
    MeReal cx = d[1] * n[2] - d[2] * n[1];
    MeReal cy = d[2] * n[0] - n[2] * d[0];
    MeReal cz = d[0] * n[1] - d[1] * n[0];
    c->direction[0] = n[1] * cz - n[2] * cy;
    c->direction[1] = cx * n[2] - cz * n[0];
    c->direction[2] = cy * n[0] - cx * n[1];
    c->material.mOptions |= 1;
}

// Win 0x74c440: the smaller / larger mass of the two bodies that are not frozen, computed once
// (while *maxMass == 0)
MeReal bodyMass(const MdtBody* b1, const MdtBody* b2, MeReal* minMass, MeReal* maxMass, int model)
{
    if (*maxMass == 0.0) {
        if (b1->frozen == 0) {
            *minMass = b1->mass;
            *maxMass = b1->mass;
        }
        if (b2->frozen == 0) {
            MeReal m = b2->mass;
            if (!(*minMass <= m))
                *minMass = m;
            if (!(m <= *maxMass))
                *maxMass = m;
        }
    }
    return model == VxMaterialBase::kFrictionModelBoxProportionalLow ? *minMass : *maxMass;
}

// Win 0x74c4c0: the friction models the solver does not know become box friction
void setupFriction(MdtContact* c, const MdtBody* b1, const MdtBody* b2, MeReal* minMass, MeReal* maxMass, int saved)
{
    VxContactMaterial& m = c->material;
    for (int axis = 0; axis < VxMaterialBase::kFrictionAxisNumber; ++axis) {
        int model = m.getFrictionModel(axis);
        if (model == VxMaterialBase::kFrictionModelScaledBoxFast) {
            if (saved < 0) {
                m.setFrictionModel(axis, VxMaterialBase::kFrictionModelScaledBox);
            } else {
                m.setBoxFrictionForce(axis, fabs(c->head.bcl.lambda[0]) * m.getFrictionCoefficient(axis));
                m.setFrictionModel(axis, VxMaterialBase::kFrictionModelBox);
            }
        } else if (model < VxMaterialBase::kFrictionModelScaledBoxFast) {
            if (model > VxMaterialBase::kFrictionModelScaledBox) {
                MeReal force = m.getBoxFrictionForce(axis);
                int mode = m.getFrictionModel(axis);
                MeReal mass = bodyMass(b1, b2, minMass, maxMass, mode);
                m.setBoxFrictionForce(axis, mass * force);
                m.setFrictionModel(axis, VxMaterialBase::kFrictionModelBox);
            }
        } else if (model == VxMaterialBase::kFrictionModelNeutral) {
            m.setFrictionModel(axis, VxMaterialBase::kFrictionModelNone);
        }
    }
}

// a point in a body's frame (transform rows = axes, row 3 = position); the world point
// without a body
inline void toBodyFrame(const MdtBody* b, const MeReal* p, MeReal* out)
{
    if (b == 0) {
        out[0] = p[0];
        out[1] = p[1];
        out[2] = p[2];
        return;
    }
    const MeReal* t = &b->transform[0][0];
    MeReal dx = p[0] - t[12], dy = p[1] - t[13], dz = p[2] - t[14];
    out[0] = dx * t[0] + dy * t[1] + dz * t[2];
    out[1] = dx * t[4] + dy * t[5] + dz * t[6];
    out[2] = dx * t[8] + dy * t[9] + dz * t[10];
}

} // namespace

// Mac 0x476df0, Win 0x74c6e0
void VxResponseBody_Body::respond(VxBridge* bridge, McdModelPair* pair, McdIntersectResult* r, CxSpace*, MdtWorldID,
                                  VxResponseEntry*)
{
    if (r->contactCount == 0) {
        // the pair separated: its contacts go (the bodies are not woken)
        MdtContact* c = (MdtContact*)pair->m24;
        while (c != 0) {
            MdtContact* next = (MdtContact*)MdtContactGetNext(&c->head);
            MdtConstraintDisable(&c->head);
            MdtContactDestroy(&c->head);
            c = next;
        }
        pair->m24 = 0;
        return;
    }
    MdtWorldID world = bridge->mWorld;
    VincModelID m1, m2;
    VincModelPairGetModels(pair, &m1, &m2);
    MdtBody* raw1 = (MdtBody*)m1->m6c;
    MdtBody* b1 = raw1 != 0 ? raw1->self : 0;
    MdtBody* raw2 = (MdtBody*)m2->m6c;
    MdtBody* b2 = raw2 != 0 ? raw2->self : 0;
    if ((b1 == 0 || b1->frozen != 0) && (b2 == 0 || b2->frozen != 0))
        return;

    MdtContact** link = (MdtContact**)&pair->m24;
    MdtContact* cur = *link;
    MeReal maxMass = 0.0;
    MeReal minMass = std::numeric_limits<MeReal>::infinity();   // the original 1.79769313486232e+308 (> DBL_MAX)
    MeReal scale = 1.0;
    int group = -1;
    unsigned int inGroup = 0;
    unsigned int groupSize = 0;
    bool grouped = r->contactGroupCount > 0;
    bool active = contactsActive(cur);
    SavedContact saved[kSavedContactMax];
    int savedCount = 0;
    if (gVxResponseKeepAllContacts && r->contactCount > 0)
        savedCount = saveAll(cur, saved, kSavedContactMax);
    else if (!active && r->contactCount > 0)
        savedCount = saveActive(cur, saved, kSavedContactMax);

    for (int i = 0; i < r->contactCount; ++i) {
        if (grouped && inGroup == groupSize) {
            ++group;
            groupSize = (unsigned int)r->groups[group].count;
            if (groupSize < 2)
                scale = r->groups[group].m04;
            else
                scale = r->groups[group].m04 / (MeReal)(int)groupSize;
            inGroup = 0;
        }
        VincContact* vc = &r->contacts[i];
        VxCollisionGeometry* cg1 = (VxCollisionGeometry*)vc->model1->userData;
        VxMaterial* mat1 = cg1->getMaterial(vc->element1);
        VxCollisionGeometry* cg2 = (VxCollisionGeometry*)vc->model2->userData;
        VxMaterial* mat2 = cg2->getMaterial(vc->element2);
        VxContactMaterial* cm = bridge->getContactMaterial(mat1, mat2);
        bool sticky = cm->mIntegratedSlipDisplacement;
        if (!sticky) {
            for (int axis = 0; axis != VxMaterialBase::kFrictionAxisNumber; ++axis)
                if (cm->getFrictionModel(axis) == VxMaterialBase::kFrictionModelScaledBoxFast) {
                    sticky = true;
                    break;
                }
        }
        int match = -1;
        MeReal p1[3] = {0, 0, 0};
        MeReal p2[3] = {0, 0, 0};
        if (savedCount > 0 && sticky) {
            toBodyFrame(b1, vc->position, p1);
            toBodyFrame(b2, vc->position, p2);
            match = findSaved(saved, savedCount, p1, p2, pair->request->m2c);
        }

        if (cur == 0) {
            cur = (MdtContact*)MdtContactCreate(world);
            MdtContactGetContactMaterial(&cur->head);
            *link = cur;
            if (cur == 0)
                break;
            MdtConstraintSetBodies(&cur->head, b1, b2);
            MdtConstraintWakeBodies(&cur->head);
        } else if (cur->head.body[0] != b1 || cur->head.body[1] != b2) {
            MdtConstraintDisable(&cur->head);
            MdtConstraintSetBodies(&cur->head, b1, b2);
            MdtConstraintWakeBodies(&cur->head);
        }
        MdtConstraintEnable(&cur->head);

        // Win 0x74c220
        MdtContactSetNormal(&cur->head, vc->normal[0], vc->normal[1], vc->normal[2]);
        MdtContactSetPosition(&cur->head, vc->position[0], vc->position[1], vc->position[2]);
        MdtContactSetPenetration(&cur->head, -vc->separation);
        MdtContactSetM_b58(&cur->head, scale);
        MdtContactSetWakeBodies(&cur->head, 0);
        MdtContactSetM_d04(&cur->head, group);
        MdtContactSetContactMaterial(&cur->head, cm);
        cur->m_b48 = b1 != 0 ? &b1->transform[0][0] : 0;
        cur->m_b4c = b2 != 0 ? &b2->transform[0][0] : 0;

        if (match < 0) {
            cur->m_cf0 = 0;
            cur->m_cf8 = 0;
            cur->m_d00 &= ~4;
        } else {
            restore(cur, saved[match]);
            cur->m_d00 |= 4;
        }
        setupFriction(cur, b1, b2, &minMass, &maxMass, match);
        if (active) {
            cur->m_d00 |= 2;
            cur->m_cf0 = 0;
            cur->m_cf8 = 0;
        } else {
            cur->m_d00 &= ~2;
        }
        cur->m_d08 = raw1;
        cur->m_d0c = raw2;
        link = &cur->next;
        cur = *link;
        ++inGroup;
    }
    // the contacts not reused go
    *link = 0;
    while (cur != 0) {
        MdtContact* next = (MdtContact*)MdtContactGetNext(&cur->head);
        MdtConstraintWakeBodies(&cur->head);
        MdtConstraintDisable(&cur->head);
        MdtContactDestroy(&cur->head);
        cur = next;
    }
    // NOTE: dead code in both builds (contactCount is not 0 here): the bodies' largest time
    // step from the pair's safe time
    if (world != 0 && world->useBodyMaxTimeStep != 0 && r->contactCount == 0) {
        VincSafeTimeResult st;
        VincSafeTime(pair, MdtWorldGetStepSize(world), &st);
        if (b1 != 0)
            MdtBodySetMaxTimeStep(b1, b1->maxTimeStep <= st.time ? b1->maxTimeStep : st.time);
        if (b2 != 0)
            MdtBodySetMaxTimeStep(b2, b2->maxTimeStep <= st.time ? b2->maxTimeStep : st.time);
    }
    // the bodies' contact callback (VxPart's ContactHandler) with the contact list
    MdtConstraint* head = (MdtConstraint*)pair->m24;
    if (head != 0) {
        if (raw1 != 0 && raw1->m548 != 0)
            raw1->m548(raw1, head);
        else if (raw2 != 0 && raw2->m548 != 0)
            raw2->m548(raw2, head);
    }
}

} // namespace Vx
