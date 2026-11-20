// LEGO Digital Designer — restored source
// The LEGOPrimitive XML parser (Expat SAX handlers filling a LEGO::Primitive) and
// BrickDatabase::LoadXMLOperation, which runs it for one Primitives/<designID>.xml.
//
// Part of the original BrickDatabase.cpp (Mac TU 024): sub-TU 024e Mac 0x83b50-0x88540
// (AABB 0x83b50, vector 0x83cf0, 3x3 matrix 0x83d70, start handler 0x83eb0 (14 KB), the
// transformation attributes 0x87880, BasicArray push_back instances 0x879e0 / 0x87b10 /
// 0x87c80, character data 0x87c50, end handler 0x87ef0), LoadXMLOperation from 024b (ctor
// 0x7f810) and 024f (Execute 0x88610, destructor 0x886a0). Win 0x4bbe20-0x4bf100 (+ ctor
// 0x4befb0, Execute 0x4bf120, destructor 0x4b2bb0; the transformation helpers 0x4a2cb0 /
// 0x4b3150 / 0x4a2ce0 are shared with ModelIO). File name inferred (plan item 1.9).
//
// Grammar (one state machine, PrimitiveParser::m_state; any element in the wrong state, any
// malformed value -> m_ok = false and XML_StopParser):
//   <LEGOPrimitive versionMajor="1" versionMinor="0">           (other versions are rejected)
//     <Annotations><Annotation key="value" …/></Annotations>    -> m_annotations
//     <Bounding><AABB minX … maxZ/></Bounding>                  -> bone 0 (+ m_bounding if zero)
//     <GeometryBounding><AABB …/></GeometryBounding>            -> m_bounding
//     <Collision><Box|Sphere|Capsule …/></Collision>            -> bone 0 collision volumes
//     <Connectivity><Custom2DField|Simple2DField|Hinge|Axel|Slider|Ball|Fixed|Rail …/>
//                                                                -> bone 0 connection fields
//     <PhysicsAttributes inertiaTensor centerOfMass mass frictionType/>   -> bone 0
//     <Decoration faces subMaterialRedirectLookupTable/>
//     <Paths><Path type points>x,y,z:…</Path></Paths>
//     <DefaultOrientation angle ax ay az tx ty tz/>   <DefaultCamera lat lon/>
//     <Flex><Bone boneId flexCheckConnection angle … tz>
//             <Bounding/> <Collision/> <Connectivity/> <PhysicsAttributes/>  -> that bone
//           </Bone></Flex>
//   </LEGOPrimitive>
// Unknown elements are ignored (e.g. <Tube> collision volumes and <Gear> fields of the shipped
// files). Transformations: "angle" in degrees about (ax, ay, az), then (tx, ty, tz).
#include "BrickDatabase.h"
#include "ConnectionFieldPlanar.h"
#include "ConnectionFieldAxle.h"
#include "ConnectionFieldBall.h"
#include "ConnectionFieldFixed.h"
#include "ConnectionFieldHinge.h"
#include "ConnectionFieldRail.h"
#include "ConnectionFieldSlider.h"
#include "LEGO/StdLib.h"
#include "StringConversion.h"

#ifndef XML_STATIC   // (the projects define it as well)
#define XML_STATIC   // Expat is linked statically into LDD.exe
#endif
#include "expat.h"

#include <iostream>

namespace LEGO {

namespace {

typedef BrickDatabase::PrimitiveParser Parser;

#if !defined(_WIN32)
const Real32 kDegreesToRadians = 0.017453292f;     // Mac 0x3c8efa35 (Win: see ParseAttribute)
#endif

// <… angle ax ay az tx ty tz>: rotation about an axis through `m_center`, then translation.
// (struct and member names inferred; Win 0x4a2cb0 initialises it, 0x4a2ce0 builds the matrix,
// 0x4b3150 parses one attribute; Mac inlines the first two.)
struct TransformAttributes {
    Vector3 m_axis;          // +0x00 (0, 1, 0)
    Real32  m_angle;         // +0x0c radians
    Vector3 m_translation;   // +0x10
    Vector3 m_center;        // +0x1c (never set from XML)

    TransformAttributes()                                                // Win 0x4a2cb0
        : m_axis(0.0f, 1.0f, 0.0f), m_angle(0.0f), m_translation(0.0f, 0.0f, 0.0f),
          m_center(0.0f, 0.0f, 0.0f)
    {
    }

    // Mac 0x87880, Win 0x4b3150
    // NOTE: after a failed "angle" compare the t* names are compared even when an a* name
    // matched (both builds) — harmless.
    void ParseAttribute(const char* name, const char* value)
    {
        if (StrCmp(name, "angle") == 0) {
#if defined(_WIN32)
            // Win 0x4b3150: pi (float) / 180.0 (double) * value, rounded once (x87, 53-bit).
            m_angle = (Real32)((Real64)3.1415927f / 180.0 * StrToReal32(value, 0));
#else
            m_angle = StrToReal32(value, 0) * kDegreesToRadians;
#endif
            return;
        }
        if (StrCmp(name, "ax") == 0)
            m_axis.x = StrToReal32(value, 0);
        else if (StrCmp(name, "ay") == 0)
            m_axis.y = StrToReal32(value, 0);
        else if (StrCmp(name, "az") == 0)
            m_axis.z = StrToReal32(value, 0);
        if (StrCmp(name, "tx") == 0)
            m_translation.x = StrToReal32(value, 0);
        else if (StrCmp(name, "ty") == 0)
            m_translation.y = StrToReal32(value, 0);
        else if (StrCmp(name, "tz") == 0)
            m_translation.z = StrToReal32(value, 0);
    }

    // Win 0x4a2ce0 (EAX = out), Mac inlined: a zero axis or angle gives a pure translation.
    void GetMatrix(Matrix3D& out) const
    {
        if (!m_axis.IsZero() && m_angle != 0.0f) {
            out.SetRotation(m_axis, m_angle, m_translation, m_center);
        } else {
            out.SetIdentity();
            out.SetTranslation(m_translation);
        }
    }
};

// Expat stops after the current handler; the state machine is dead from here on.
void Fail(Parser& parser)
{
    parser.m_ok = false;
    XML_StopParser(*parser.m_parser, XML_FALSE);
}

// The bone the elements of the current block belong to: the <Flex><Bone> being read, else 0.
Primitive::Bone& CurrentBone(Parser& parser)
{
    Int32 bone = parser.m_bone < 0 ? 0 : parser.m_bone;
    return parser.m_primitive->m_bones[bone];
}

bool AttributeIsTrue(const char* value)
{
    return StrToUInt32(value, 0, 10) != 0;
}

// <AABB minX minY minZ maxX maxY maxZ/>. Mac 0x83b50, Win 0x4bbe20 (EAX = attributes)
void ParseAABB(BoundingBox& box, const char** attributes)
{
    for (; attributes[0]; attributes += 2) {
        const char* name = attributes[0];
        const char* value = attributes[1];
        if (StrCmp(name, "minX") == 0)
            box.m_min.x = StrToReal32(value, 0);
        else if (StrCmp(name, "minY") == 0)
            box.m_min.y = StrToReal32(value, 0);
        else if (StrCmp(name, "minZ") == 0)
            box.m_min.z = StrToReal32(value, 0);
        else if (StrCmp(name, "maxX") == 0)
            box.m_max.x = StrToReal32(value, 0);
        else if (StrCmp(name, "maxY") == 0)
            box.m_max.y = StrToReal32(value, 0);
        else if (StrCmp(name, "maxZ") == 0)
            box.m_max.z = StrToReal32(value, 0);
    }
}

// "x,y,z". Mac 0x83cf0, Win 0x4bc060 (ESI = parser, EDI = out)
void ParseVector3(Parser& parser, Vector3& out, const char* text)
{
    char* p;
    out.x = StrToReal32(text, &p);
    if (*p == ',') {
        ++p;
        out.y = StrToReal32(p, &p);
        if (*p == ',') {
            ++p;
            out.z = StrToReal32(p, 0);
            return;
        }
    }
    Fail(parser);
}

// "m00,m01,m02,m10,…,m22" into the upper left 3x3 of the column-major matrix (elements 0-2,
// 4-6, 8-10). Mac 0x83d70, Win 0x4bc110 (ESI = parser, EDI = out)
void ParseMatrix3x3(Parser& parser, Matrix3D& out, const char* text)
{
    static const UInt32 kElements[9] = { 0, 1, 2, 4, 5, 6, 8, 9, 10 };
    char* p = const_cast<char*>(text);
    for (UInt32 i = 0; i < 9; ++i) {
        if (i != 0) {
            if (*p != ',') {
                Fail(parser);
                return;
            }
            ++p;
        }
        out.m[kElements[i]] = StrToReal32(p, i == 8 ? 0 : &p);
    }
}

void XMLCALL CharacterData(void* userData, const XML_Char* text, int length);

void AddField(Parser& parser, ConnectionField* field)
{
    CurrentBone(parser).m_connectionFields.PushBack(RefPointer<ConnectionField>(field));
}

// <Decoration faces=… subMaterialRedirectLookupTable="a,b,…"/>: the table must not be longer
// than the face count and is padded with the identity. (inlined in the start handler)
void ParseDecoration(Parser& parser, const char** attributes)
{
    Primitive& primitive = *parser.m_primitive;
    AString table;
    for (; attributes[0]; attributes += 2) {
        if (StrCmp(attributes[0], "faces") == 0)
            primitive.m_decorationFaceCount = StrToUInt32(attributes[1], 0, 10);
        else if (StrCmp(attributes[0], "subMaterialRedirectLookupTable") == 0)
            table = attributes[1];
    }
    const char* p = table.IsEmpty() ? "" : table.GetBuffer();
    const char* end = p + table.GetLength();
    for (; p < end; ++p) {
        char* next;
        primitive.m_subMaterialRedirect.PushBack(StrToUInt32(p, &next, 10));
        p = next;
    }
    if (primitive.m_decorationFaceCount < primitive.m_subMaterialRedirect.GetSize()) {
        Fail(parser);
        return;
    }
    while (primitive.m_subMaterialRedirect.GetSize() < primitive.m_decorationFaceCount)
        primitive.m_subMaterialRedirect.PushBack(primitive.m_subMaterialRedirect.GetSize());
}

// <Bone boneId flexCheckConnection="a,b,c" angle … tz>: grows the bone and inverse-matrix
// arrays to boneId + 1. (inlined in the start handler)
void ParseBone(Parser& parser, const char** attributes)
{
    TransformAttributes transform;
    UInt32 boneId = 0;
    UInt32 check[3] = { 0, 0, 0 };
    for (; attributes[0]; attributes += 2) {
        if (StrCmp(attributes[0], "boneId") == 0) {
            boneId = StrToUInt32(attributes[1], 0, 10);
        } else if (StrCmp(attributes[0], "flexCheckConnection") == 0) {
            char* p = const_cast<char*>(attributes[1]);
            check[0] = StrToUInt32(p, &p, 10);
            ++p;
            check[1] = StrToUInt32(p, &p, 10);
            ++p;
            check[2] = StrToUInt32(p, &p, 10);
        } else {
            transform.ParseAttribute(attributes[0], attributes[1]);
        }
    }
    Matrix3D matrix;
    Matrix3D inverse;
    transform.GetMatrix(matrix);
    inverse.SetInverse(matrix);       // stays identity for a singular matrix

    Primitive& primitive = *parser.m_primitive;
    // NOTE: boneId grows the arrays to boneId + 1 unchecked (both builds; 0xFFFFFFFF wraps the
    // size to 0 and the bone is written past it); fixed: a boneId from kMaxBones on fails the
    // primitive.
    const UInt32 kMaxBones = 1024;
    if (boneId >= kMaxBones) {
        Fail(parser);
        return;
    }
    if (primitive.m_bones.GetSize() <= boneId)
        primitive.m_bones.Resize(boneId + 1, Primitive::Bone());
    Primitive::Bone& bone = primitive.m_bones[boneId];
    bone.m_transform = matrix;
    bone.m_flexCheckConnection[0] = check[0];
    bone.m_flexCheckConnection[1] = check[1];
    bone.m_flexCheckConnection[2] = check[2];
    if (primitive.m_inverseBoneTransforms.GetSize() <= boneId)
        primitive.m_inverseBoneTransforms.Resize(boneId + 1, Matrix3D());
    primitive.m_inverseBoneTransforms[boneId] = inverse;
    parser.m_bone = (Int32)boneId;
}

// <Ball type flexAttributes="lower,upper,twist,flexX,flexY"> (angles in degrees).
// NOTE: the defaults (-pi, pi, pi, 0, 0) are never used: the limits are only set for
// kind >= 999000 with the attribute present (both builds).
void ParseBall(Parser& parser, const char** attributes)
{
    TransformAttributes transform;
    UInt32 kind = 0;
    bool hasFlex = false;
    Real32 lower = -3.1415927f;
    Real32 upper = 3.1415927f;
    Real32 twist = 3.1415927f;
    Real32 flexX = 0.0f;
    Real32 flexY = 0.0f;
    for (; attributes[0]; attributes += 2) {
        if (StrCmp(attributes[0], "type") == 0) {
            kind = StrToUInt32(attributes[1], 0, 10);
        } else if (StrCmp(attributes[0], "flexAttributes") == 0) {
            char* p = const_cast<char*>(attributes[1]);
            hasFlex = true;
#if defined(_WIN32)
            // Win: pi (float) / 180.0 (double) * value, rounded once (x87, 53-bit).
            Real64 toRadians = (Real64)3.1415927f / 180.0;
            lower = (Real32)(toRadians * StrToReal32(p, &p));
            ++p;
            upper = (Real32)(toRadians * StrToReal32(p, &p));
            ++p;
            twist = (Real32)(toRadians * StrToReal32(p, &p));
#else
            // Mac: value * 0.017453292f.
            lower = StrToReal32(p, &p) * kDegreesToRadians;
            ++p;
            upper = StrToReal32(p, &p) * kDegreesToRadians;
            ++p;
            twist = StrToReal32(p, &p) * kDegreesToRadians;
#endif
            ++p;
            flexX = StrToReal32(p, &p);
            ++p;
            flexY = StrToReal32(p, &p);
        } else {
            transform.ParseAttribute(attributes[0], attributes[1]);
        }
    }
    Matrix3D matrix;
    transform.GetMatrix(matrix);
    ConnectionFieldBall* ball = ConnectionFieldBall::Create(matrix, kind);
    if (!ball) {
        Fail(parser);
        return;
    }
    RefPointer<ConnectionField> field(ball);
    AddField(parser, ball);
    if (kind >= 999000 && hasFlex)
        ball->SetLimits(lower, upper, twist, flexX, flexY);
}

// <Path type="1|2" points="n">: a new path of n zero points, filled from the text by the end
// handler. (inlined in the start handler)
void StartPath(Parser& parser, const char** attributes)
{
    UInt32 type = 0xffffffff;
    UInt32 points = 0;
    for (; attributes[0]; attributes += 2) {
        if (StrCmp(attributes[0], "type") == 0)
            type = StrToUInt32(attributes[1], 0, 10);
        else if (StrCmp(attributes[0], "points") == 0)
            points = StrToUInt32(attributes[1], 0, 10);
    }
    if (type == 0xffffffff || points < 2 || (type != Primitive::Path::TypeOpen &&
                                             type != Primitive::Path::TypeClosed)) {
        Fail(parser);
        return;
    }
    Primitive::Path path;
    path.m_type = type;
    parser.m_primitive->m_paths.PushBack(path);
    parser.m_primitive->m_paths.Back().m_points.Resize(points, Vector3(0.0f, 0.0f, 0.0f));
    parser.m_text.Clear();
    XML_SetCharacterDataHandler(*parser.m_parser, CharacterData);
}

// Start handler. Mac 0x83eb0, Win 0x4bc280
void XMLCALL StartElement(void* userData, const XML_Char* name, const XML_Char** attributes)
{
    Parser& parser = *static_cast<Parser*>(userData);
    Primitive& primitive = *parser.m_primitive;
    const bool ok = parser.m_ok;
    const UInt32 state = parser.m_state;
    const bool inBone = parser.m_bone >= 0;
    // state of a block that may also appear inside <Flex><Bone>
    const UInt32 blockParent = inBone ? Parser::StateBone : Parser::StatePrimitive;

    if (StrCmp(name, "LEGOPrimitive") == 0) {
        if (ok && state == Parser::StateDocument) {
            parser.m_state = Parser::StatePrimitive;
            for (const char** a = attributes; a[0]; a += 2) {
                if (StrCmp(a[0], "versionMajor") == 0)
                    parser.m_versionMajor = StrToUInt32(a[1], 0, 10);
                else if (StrCmp(a[0], "versionMinor") == 0)
                    parser.m_versionMinor = StrToUInt32(a[1], 0, 10);
            }
            if (parser.m_versionMajor == 1 && parser.m_versionMinor == 0) {
                primitive.m_bones.Resize(1, Primitive::Bone());
                return;
            }
        }
        Fail(parser);
        return;
    }
    if (StrCmp(name, "Bounding") == 0) {
        if (ok && state == blockParent) {
            parser.m_state = Parser::StateBounding;
            return;
        }
        Fail(parser);
        return;
    }
    if (StrCmp(name, "GeometryBounding") == 0) {
        if (ok && state == Parser::StatePrimitive) {
            parser.m_state = Parser::StateGeometryBounding;
            return;
        }
        Fail(parser);
        return;
    }
    if (StrCmp(name, "AABB") == 0) {
        if (state == Parser::StateGeometryBounding) {
            if (ok) {
                ParseAABB(primitive.m_bounding, attributes);
                return;
            }
        } else if (state == Parser::StateBounding && ok) {
            if (inBone) {
                ParseAABB(primitive.m_bones[parser.m_bone].m_bounding, attributes);
                return;
            }
            BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            ParseAABB(box, attributes);
            if (primitive.m_bones.GetSize() == 1)
                primitive.m_bones[0].m_bounding = box;
            if (primitive.m_bounding.IsZero())
                primitive.m_bounding = box;
            return;
        }
        Fail(parser);
        return;
    }
    if (StrCmp(name, "Collision") == 0) {
        if (ok && state == blockParent) {
            parser.m_state = Parser::StateCollision;
            return;
        }
        Fail(parser);
        return;
    }
    if (StrCmp(name, "Box") == 0 || StrCmp(name, "Sphere") == 0 || StrCmp(name, "Capsule") == 0) {
        if (!(ok && state == Parser::StateCollision)) {
            Fail(parser);
            return;
        }
        Primitive::CollisionVolume volume;
        TransformAttributes transform;
        if (StrCmp(name, "Box") == 0) {
            volume.m_type = Primitive::CollisionVolume::Box;
            for (const char** a = attributes; a[0]; a += 2) {
                if (StrCmp(a[0], "sX") == 0)
                    volume.m_size.x = StrToReal32(a[1], 0);
                else if (StrCmp(a[0], "sY") == 0)
                    volume.m_size.y = StrToReal32(a[1], 0);
                else if (StrCmp(a[0], "sZ") == 0)
                    volume.m_size.z = StrToReal32(a[1], 0);
                else
                    transform.ParseAttribute(a[0], a[1]);
            }
        } else if (StrCmp(name, "Sphere") == 0) {
            volume.m_type = Primitive::CollisionVolume::Sphere;
            for (const char** a = attributes; a[0]; a += 2) {
                if (StrCmp(a[0], "radius") == 0)
                    volume.m_size.x = StrToReal32(a[1], 0);
                else
                    transform.ParseAttribute(a[0], a[1]);
            }
        } else {
            volume.m_type = Primitive::CollisionVolume::Capsule;
            for (const char** a = attributes; a[0]; a += 2) {
                if (StrCmp(a[0], "radius") == 0)
                    volume.m_size.y = StrToReal32(a[1], 0);
                else if (StrCmp(a[0], "length") == 0)
                    volume.m_size.x = StrToReal32(a[1], 0);
                else
                    transform.ParseAttribute(a[0], a[1]);
            }
        }
        transform.GetMatrix(volume.m_transform);
        CurrentBone(parser).m_collisionVolumes.PushBack(volume);   // Mac 0x879e0
        return;
    }
    if (StrCmp(name, "Connectivity") == 0) {
        if (ok && state == blockParent) {
            parser.m_state = Parser::StateConnectivity;
            return;
        }
        Fail(parser);
        return;
    }
    if (StrCmp(name, "Simple2DField") == 0 || StrCmp(name, "Custom2DField") == 0) {
        if (!(ok && state == Parser::StateConnectivity)) {
            Fail(parser);
            return;
        }
        TransformAttributes transform;
        UInt32 kind = 0;
        UInt32 width = 0;
        UInt32 height = 0;
        for (const char** a = attributes; a[0]; a += 2) {
            if (StrCmp(a[0], "type") == 0)
                kind = StrToUInt32(a[1], 0, 10);
            else if (StrCmp(a[0], "width") == 0)
                width = StrToUInt32(a[1], 0, 10);
            else if (StrCmp(a[0], "height") == 0)
                height = StrToUInt32(a[1], 0, 10);
            else
                transform.ParseAttribute(a[0], a[1]);
        }
        Matrix3D matrix;
        transform.GetMatrix(matrix);
        RefPointer<ConnectionField> field(ConnectionFieldPlanar::Create(matrix, kind, width, height));
        if (field.IsNull()) {
            Fail(parser);
            return;
        }
        AddField(parser, field);                                     // Mac 0x87b10
        if (StrCmp(name, "Custom2DField") == 0) {
            // the grid follows as element text ("type:capacity[:flags[:group]]," per point)
            parser.m_text.Clear();
            XML_SetCharacterDataHandler(*parser.m_parser, CharacterData);
        }
        return;
    }
    if (StrCmp(name, "Hinge") == 0) {
        if (!(ok && state == Parser::StateConnectivity)) {
            Fail(parser);
            return;
        }
        TransformAttributes transform;
        UInt32 kind = 0;
        bool oriented = false;
        bool hasLimits = false;
        Real32 minAngle = 0.0f;
        Real32 maxAngle = 0.0f;
        Real32 flipMinAngle = 0.0f;
        Real32 flipMaxAngle = 0.0f;
        for (const char** a = attributes; a[0]; a += 2) {
            if (StrCmp(a[0], "type") == 0) {
                kind = StrToUInt32(a[1], 0, 10);
            } else if (StrCmp(a[0], "oriented") == 0) {
                oriented = AttributeIsTrue(a[1]);
            } else if (StrCmp(a[0], "LimMin") == 0) {
                hasLimits = true;
                minAngle = StrToReal32(a[1], 0);
            } else if (StrCmp(a[0], "LimMax") == 0) {
                maxAngle = StrToReal32(a[1], 0);
            } else if (StrCmp(a[0], "FlipLimMin") == 0) {
                flipMinAngle = StrToReal32(a[1], 0);
            } else if (StrCmp(a[0], "FlipLimMax") == 0) {
                flipMaxAngle = StrToReal32(a[1], 0);
            } else {
                transform.ParseAttribute(a[0], a[1]);
            }
        }
        Matrix3D matrix;
        transform.GetMatrix(matrix);
        RefPointer<ConnectionField> field(ConnectionFieldHinge::Create(matrix, kind, oriented, hasLimits,
                                                                       minAngle, maxAngle, flipMinAngle,
                                                                       flipMaxAngle));
        if (field.IsNull()) {
            Fail(parser);
            return;
        }
        AddField(parser, field);
        return;
    }
    if (StrCmp(name, "Axel") == 0) {
        if (!(ok && state == Parser::StateConnectivity)) {
            Fail(parser);
            return;
        }
        TransformAttributes transform;
        UInt32 kind = 0;
        Real32 length = 0.0f;
        bool grabbing = false;
        bool requireGrabbing = false;
        bool startCapped = false;
        bool endCapped = false;
        for (const char** a = attributes; a[0]; a += 2) {
            if (StrCmp(a[0], "type") == 0)
                kind = StrToUInt32(a[1], 0, 10);
            else if (StrCmp(a[0], "length") == 0)
                length = StrToReal32(a[1], 0);
            else if (StrCmp(a[0], "grabbing") == 0)
                grabbing = AttributeIsTrue(a[1]);
            else if (StrCmp(a[0], "requireGrabbing") == 0)
                requireGrabbing = AttributeIsTrue(a[1]);
            else if (StrCmp(a[0], "startCapped") == 0)
                startCapped = AttributeIsTrue(a[1]);
            else if (StrCmp(a[0], "endCapped") == 0)
                endCapped = AttributeIsTrue(a[1]);
            else
                transform.ParseAttribute(a[0], a[1]);
        }
        Matrix3D matrix;
        transform.GetMatrix(matrix);
        RefPointer<ConnectionField> field(ConnectionFieldAxle::Create(matrix, kind, length, startCapped,
                                                                      endCapped, grabbing, requireGrabbing));
        if (field.IsNull()) {
            Fail(parser);
            return;
        }
        AddField(parser, field);
        return;
    }
    if (StrCmp(name, "Slider") == 0) {
        if (!(ok && state == Parser::StateConnectivity)) {
            Fail(parser);
            return;
        }
        TransformAttributes transform;
        UInt32 kind = 0;
        Real32 length = 0.0f;
        bool startCapped = false;
        bool endCapped = false;
        for (const char** a = attributes; a[0]; a += 2) {
            if (StrCmp(a[0], "type") == 0)
                kind = StrToUInt32(a[1], 0, 10);
            else if (StrCmp(a[0], "length") == 0)
                length = StrToReal32(a[1], 0);
            else if (StrCmp(a[0], "startCapped") == 0)
                startCapped = AttributeIsTrue(a[1]);
            else if (StrCmp(a[0], "endCapped") == 0)
                endCapped = AttributeIsTrue(a[1]);
            else
                transform.ParseAttribute(a[0], a[1]);
        }
        Matrix3D matrix;
        transform.GetMatrix(matrix);
        RefPointer<ConnectionField> field(ConnectionFieldSlider::Create(matrix, kind, length, startCapped,
                                                                        endCapped));
        if (field.IsNull()) {
            Fail(parser);
            return;
        }
        AddField(parser, field);
        return;
    }
    if (StrCmp(name, "Ball") == 0) {
        if (!(ok && state == Parser::StateConnectivity)) {
            Fail(parser);
            return;
        }
        ParseBall(parser, attributes);
        return;
    }
    if (StrCmp(name, "Fixed") == 0) {
        if (!(ok && state == Parser::StateConnectivity)) {
            Fail(parser);
            return;
        }
        TransformAttributes transform;
        UInt32 kind = 0;
        UInt32 axes = 0;
        for (const char** a = attributes; a[0]; a += 2) {
            if (StrCmp(a[0], "type") == 0)
                kind = StrToUInt32(a[1], 0, 10);
            else if (StrCmp(a[0], "axes") == 0)
                axes = StrToUInt32(a[1], 0, 10);
            else
                transform.ParseAttribute(a[0], a[1]);
        }
        Matrix3D matrix;
        transform.GetMatrix(matrix);
        RefPointer<ConnectionField> field(ConnectionFieldFixed::Create(matrix, kind, axes));
        if (field.IsNull()) {
            Fail(parser);
            return;
        }
        AddField(parser, field);
        return;
    }
    if (StrCmp(name, "Rail") == 0) {
        if (!(ok && state == Parser::StateConnectivity)) {
            Fail(parser);
            return;
        }
        TransformAttributes transform;
        UInt32 kind = 0;
        Real32 length = 0.0f;
        for (const char** a = attributes; a[0]; a += 2) {
            if (StrCmp(a[0], "type") == 0)
                kind = StrToUInt32(a[1], 0, 10);
            else if (StrCmp(a[0], "length") == 0)
                length = StrToReal32(a[1], 0);
            else
                transform.ParseAttribute(a[0], a[1]);
        }
        Matrix3D matrix;
        transform.GetMatrix(matrix);
        RefPointer<ConnectionField> field(ConnectionFieldRail::Create(matrix, kind, length));
        if (field.IsNull()) {
            Fail(parser);
            return;
        }
        AddField(parser, field);
        return;
    }
    if (StrCmp(name, "Decoration") == 0) {
        if (ok && state == Parser::StatePrimitive) {
            ParseDecoration(parser, attributes);
            return;
        }
        Fail(parser);
        return;
    }
    if (StrCmp(name, "Paths") == 0) {
        if (ok && state == Parser::StatePrimitive) {
            parser.m_state = Parser::StatePaths;
            return;
        }
        Fail(parser);
        return;
    }
    if (StrCmp(name, "Path") == 0) {
        if (ok && state == Parser::StatePaths && attributes[0]) {
            StartPath(parser, attributes);
            return;
        }
        Fail(parser);
        return;
    }
    if (StrCmp(name, "DefaultOrientation") == 0) {
        if (ok && state == Parser::StatePrimitive) {
            TransformAttributes transform;
            for (const char** a = attributes; a[0]; a += 2)
                transform.ParseAttribute(a[0], a[1]);
            transform.GetMatrix(parser.m_defaultOrientation);
            return;
        }
        Fail(parser);
        return;
    }
    if (StrCmp(name, "DefaultCamera") == 0) {
        if (ok && state == Parser::StatePrimitive) {
            for (const char** a = attributes; a[0]; a += 2) {
                if (StrCmp(a[0], "lat") == 0)
                    parser.m_defaultCamera.x = StrToReal32(a[1], 0);
                else if (StrCmp(a[0], "lon") == 0)
                    parser.m_defaultCamera.y = StrToReal32(a[1], 0);
            }
            return;
        }
        Fail(parser);
        return;
    }
    if (StrCmp(name, "Flex") == 0) {
        if (ok && state == Parser::StatePrimitive) {
            parser.m_state = Parser::StateFlex;
            return;
        }
        Fail(parser);
        return;
    }
    if (StrCmp(name, "Bone") == 0) {
        if (ok && state == Parser::StateFlex) {
            parser.m_state = Parser::StateBone;
            ParseBone(parser, attributes);
            return;
        }
        Fail(parser);
        return;
    }
    if (StrCmp(name, "Annotations") == 0) {
        if (ok && state == Parser::StatePrimitive) {
            parser.m_state = Parser::StateAnnotations;
            return;
        }
        Fail(parser);
        return;
    }
    if (StrCmp(name, "Annotation") == 0) {
        if (ok && state == Parser::StateAnnotations) {
            // every attribute is one annotation: the name as ASCII, the value UTF-8 decoded
            for (const char** a = attributes; a[0]; a += 2)
                primitive.m_annotations[String(a[0])] = DecodeString(kTextEncodingUTF8, a[1], StrLen(a[1]));
            return;
        }
        Fail(parser);
        return;
    }
    if (StrCmp(name, "PhysicsAttributes") != 0)
        return;                                             // unknown elements are ignored
    if (!(ok && state == blockParent)) {
        Fail(parser);
        return;
    }
    Primitive::Bone& bone = CurrentBone(parser);
    for (const char** a = attributes; a[0]; a += 2) {
        if (StrCmp(a[0], "inertiaTensor") == 0)
            ParseMatrix3x3(parser, bone.m_inertiaTensor, a[1]);
        else if (StrCmp(a[0], "centerOfMass") == 0)
            ParseVector3(parser, bone.m_centerOfMass, a[1]);
        else if (StrCmp(a[0], "mass") == 0)
            bone.m_mass = StrToReal32(a[1], 0);
        else if (StrCmp(a[0], "frictionType") == 0)
            bone.m_frictionType = StrToUInt32(a[1], 0, 10);
    }
}

// Character data of <Custom2DField> / <Path> (the handler is only installed there): every
// chunk except a lone line feed is appended. Mac 0x87c50, Win 0x4bef80
void XMLCALL CharacterData(void* userData, const XML_Char* text, int length)
{
    Parser& parser = *static_cast<Parser*>(userData);
    if (length != 1 || *text != '\n')
        parser.m_text.Append(text, (UInt32)length);
}

// </Custom2DField>: (height + 1) rows of (width + 1) "type:capacity[:flags[:group]]" entries,
// ',' separated; only the very last entry may be followed by white space instead of ','.
// (inlined in the end handler; the point setter is Win 0x4b2fe0, inlined on Mac)
// NOTE: the Win setter marks the field as using extended types for types 30-37 only; the
// restored ConnectionFieldPlanar::SetPoint (Mac shape) does so for every type > 29 (no shipped
// primitive uses a type above 29).
bool ParseCustomGrid(Parser& parser)
{
    ConnectionFieldPlanar& field =
        static_cast<ConnectionFieldPlanar&>(*CurrentBone(parser).m_connectionFields.Back());
    const char* p = parser.m_text.IsEmpty() ? "" : parser.m_text.GetBuffer();
    const char* end = p + parser.m_text.GetLength();
    const UInt32 width = field.GetWidth();
    const UInt32 height = field.GetHeight();
    for (UInt32 row = 0; row <= height; ++row) {
        for (UInt32 column = 0; column <= width; ++column) {
            const bool last = row == height && column == width;
            char* next;
            UInt8 type = StrToUInt8(p, &next, 10);
            p = next;
            if (*p != ':')
                return false;
            if (++p >= end)
                return false;
            UInt8 capacity = StrToUInt8(p, &next, 10);
            p = next;
            UInt8 flags = 0;
            UInt8 group = 0;
            if (*p == ',') {
                ++p;
            } else if (*p == ':') {
                ++p;
                flags = StrToUInt8(p, &next, 10);
                p = next;
                if (*p == ',') {
                    ++p;
                } else if (*p == ':') {
                    ++p;
                    group = StrToUInt8(p, &next, 10);
                    p = next;
                    if (*p == ',') {
                        ++p;
                    } else if (*p != '\0') {
                        if (!last)
                            return false;
                        SkipSpaces(&p);
                        if (p != end)
                            return false;
                    }
                } else if (*p != '\0') {
                    if (!last)
                        return false;
                    SkipSpaces(&p);
                    if (p != end)
                        return false;
                }
            } else if (*p != '\0') {
                if (!last)
                    return false;
                SkipSpaces(&p);
                if (p != end)
                    return false;
            }
            if (type >= ConnectionFieldPlanar::TYPE_COUNT)
                return false;
            field.SetPoint(row * (width + 1) + column, type, capacity, flags, group);
        }
    }
    return true;
}

// </Path>: "x,y,z" per point, any one-character separators.
bool ParsePathPoints(Parser& parser)
{
    Primitive::Path& path = parser.m_primitive->m_paths.Back();
    const char* p = parser.m_text.IsEmpty() ? "" : parser.m_text.GetBuffer();
    const char* end = p + parser.m_text.GetLength();
    for (UInt32 i = 0; i < path.m_points.GetSize(); ++i) {
        Vector3& point = path.m_points[i];
        char* next;
        point.x = StrToReal32(p, &next);
        p = next + 1;
        if (p >= end)
            return false;
        point.y = StrToReal32(p, &next);
        p = next + 1;
        if (p >= end)
            return false;
        point.z = StrToReal32(p, &next);
        p = next + 1;
        if (p > end)
            return false;
    }
    return true;
}

// End handler. Mac 0x87ef0, Win 0x4be880
void XMLCALL EndElement(void* userData, const XML_Char* name)
{
    Parser& parser = *static_cast<Parser*>(userData);
    const bool ok = parser.m_ok;
    const UInt32 state = parser.m_state;
    const UInt32 blockParent = parser.m_bone >= 0 ? Parser::StateBone : Parser::StatePrimitive;
    UInt32 expected;
    UInt32 next;

    if (StrCmp(name, "LEGOPrimitive") == 0) {
        expected = Parser::StatePrimitive;
        next = Parser::StateDocument;
    } else if (StrCmp(name, "Bounding") == 0) {
        expected = Parser::StateBounding;
        next = blockParent;
    } else if (StrCmp(name, "GeometryBounding") == 0) {
        expected = Parser::StateGeometryBounding;
        next = Parser::StatePrimitive;
    } else if (StrCmp(name, "Collision") == 0) {
        expected = Parser::StateCollision;
        next = blockParent;
    } else if (StrCmp(name, "Connectivity") == 0) {
        expected = Parser::StateConnectivity;
        next = blockParent;
    } else if (StrCmp(name, "Custom2DField") == 0) {
        if (ok && state == Parser::StateConnectivity) {
            if (!ParseCustomGrid(parser)) {
                Fail(parser);
                return;
            }
            // NOTE: m_text is left as it is (cleared when the next text block starts)
            XML_SetCharacterDataHandler(*parser.m_parser, 0);
            return;
        }
        Fail(parser);
        return;
    } else if (StrCmp(name, "Paths") == 0) {
        expected = Parser::StatePaths;
        next = Parser::StatePrimitive;
    } else if (StrCmp(name, "Path") == 0) {
        if (ok && state == Parser::StatePaths) {
            if (!ParsePathPoints(parser)) {
                Fail(parser);
                return;
            }
            XML_SetCharacterDataHandler(*parser.m_parser, 0);
            return;
        }
        Fail(parser);
        return;
    } else if (StrCmp(name, "Flex") == 0) {
        expected = Parser::StateFlex;
        next = Parser::StatePrimitive;
    } else if (StrCmp(name, "Bone") == 0) {
        if (ok && state == Parser::StateBone) {
            parser.m_state = Parser::StateFlex;
            parser.m_bone = -1;
            return;
        }
        Fail(parser);
        return;
    } else if (StrCmp(name, "Annotations") == 0) {
        expected = Parser::StateAnnotations;
        next = Parser::StatePrimitive;
    } else {
        return;                                             // leaf elements: nothing to do
    }
    if (ok && state == expected) {
        parser.m_state = next;
        return;
    }
    Fail(parser);
}

} // namespace

// ---------------------------------------------------------------------------------------------
// BrickDatabase::LoadXMLOperation
// ---------------------------------------------------------------------------------------------

// Mac 0x7f810, Win 0x4befb0 (context set-up Win 0x4b2b10)
BrickDatabase::LoadXMLOperation::LoadXMLOperation(BrickDatabase* database, UInt32 designID)
    : m_primitive(new Primitive(database, designID)), m_parser(XML_ParserCreate(0)),
      m_succeeded(false)
{
    m_context.m_primitive = m_primitive;
    m_context.m_parser = &m_parser;
    m_context.m_defaultCamera = Vector2(-0.800467014f, 0.76566267f);   // Mac 0xbf4ceb68, 0x3f440278
    m_context.m_ok = true;
    m_context.m_versionMajor = 0;
    m_context.m_versionMinor = 0;
    m_context.m_bone = -1;
    m_context.m_state = PrimitiveParser::StateDocument;
    XML_SetUserData(m_parser, &m_context);
    XML_SetElementHandler(m_parser, StartElement, EndElement);
}

// Mac 0x886a0 (D0 0x88700, thunks 0x88770 / 0x887e0; members 0x8c290), Win 0x4b2bb0
BrickDatabase::LoadXMLOperation::~LoadXMLOperation()
{
    if (m_parser)
        XML_ParserFree(m_parser);   // an operation that never ran (BrickDatabase.h; low)
}

// Mac 0x88610, Win 0x4bf120
void BrickDatabase::LoadXMLOperation::Execute()
{
    const char* text = m_text.IsEmpty() ? "" : m_text.GetBuffer();
    m_succeeded = XML_Parse(m_parser, text, (int)m_text.GetLength(), 1) != 0;
    XML_ParserFree(m_parser);
    m_parser = 0;
    m_text.Clear();
    m_succeeded = m_succeeded && m_context.m_ok;
}

} // namespace LEGO
