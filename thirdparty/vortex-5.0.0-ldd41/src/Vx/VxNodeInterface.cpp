// Vortex 5.0.0 build 800 "vortexldd41" - the default VxNodeInterface (restored).
// Mac TU 507 0x467482-0x467efd (static initialiser 0x467e98: <iostream> only), Win
// 0x752d00-0x7534e0 (+ slot 17 at 0x7ac7a0). The registered interface (vxGetNodeInterface) and
// its default instance are in VxSceneGraphInterface.cpp (V4.5). LDD registers no other one and
// binds no node: only VxNode::addRef / release could reach addRef / release, and they skip a null
// node.
#include "Vx/VxSceneGraphInterface.h"
#include "Vx/VxMessage.h"
#include <float.h>
#include <iostream>

namespace Vx {

// Mac 0x77cdfc, Win 0xaf2ff8: the null node handle
const VxNode VxNode::sNull;

namespace {

void warnDefault(const char* method)
{
    VxWarning(0, "VxNodeInterface: default implementation of %s() is used. You may want to create and register a concrete class with VX_GRAPHICS_REGISTER_NODE_INTERFACE for your own implentation.\n", method);
}

} // namespace

// Mac D1 0x46764a / D0 0x467620, Win 0x752d00
VxNodeInterface::~VxNodeInterface()
{
}

// Mac 0x4675d8, Win 0x752cf0 (ICF with slot 3)
int VxNodeInterface::addRef(const VxNode&)
{
    return 1;
}

// Mac 0x4675e2, Win 0x752cf0
int VxNodeInterface::release(const VxNode&)
{
    return 1;
}

// Mac 0x467894, Win 0x7533f0
std::string VxNodeInterface::getName(const VxNode&)
{
    warnDefault("getName");
    return std::string("NoAccessToName");
}

// Mac 0x467860, Win 0x752e60
void VxNodeInterface::setName(const VxNode&, const std::string&)
{
    warnDefault("setName");
}

// Mac 0x4677fc, Win 0x753440
std::string VxNodeInterface::getClassName(const VxNode&)
{
    warnDefault("getClassName");
    return std::string("NoAccessToName");
}

// Mac 0x467ab8, Win 0x752e80
void VxNodeInterface::addChild(const VxNode&, const VxNode&, int, int, int)
{
    warnDefault("addChild");
}

// Mac 0x467a84, Win 0x752ea0
void VxNodeInterface::removeChild(const VxNode&, const VxNode&)
{
    warnDefault("removeChild");
}

// Mac 0x467a4c, Win 0x752ec0
int VxNodeInterface::getNumChildren(const VxNode&)
{
    warnDefault("getNumChildren");
    return 0;
}

// Mac 0x467a00, Win 0x752ee0
VxNode VxNodeInterface::getChild(const VxNode&, int)
{
    warnDefault("getChild");
    return VxNode::sNull;
}

// Mac 0x4679b4, Win 0x752f10
VxNode VxNodeInterface::findChild(const VxNode&, const std::string&)
{
    warnDefault("findChild");
    return VxNode::sNull;
}

// Mac 0x46797c, Win 0x752f40
bool VxNodeInterface::hasChild(const VxNode&, const VxNode&)
{
    warnDefault("hasChild");
    return false;
}

// Mac 0x467930, Win 0x752f60
VxNode VxNodeInterface::getParent(const VxNode&, int)
{
    warnDefault("getParent");
    return VxNode::sNull;
}

// Mac 0x4678f8, Win 0x752f90
int VxNodeInterface::getNumParents(const VxNode&)
{
    warnDefault("getNumParents");
    return 0;
}

// Mac 0x4675ec, Win 0x752fb0
void VxNodeInterface::validateOrInsertTransform(const VxNode&)
{
    warnDefault("validateOrInsertTransform");
}

// Mac 0x467e4c, Win 0x752fd0
VxTransform VxNodeInterface::getTransform(const VxNode&)
{
    warnDefault("getTransform");
    return VxTransform();
}

// Mac 0x4674a4, Win 0x7ac7a0. NOTE: the original initialises only column 3 of the result before
// asking the scene-graph interface (the trivial one writes nothing: the rest stays
// indeterminate there; identity here), and reads the frame without a null test.
VxTransform VxNodeInterface::getNodeTransform(const VxNode& node)
{
    VxTransform tm;
    if (node.mHandle != 0) {
        VxVector3 scale(0, 0, 0);
        VxTransform parentWorld;
        VxNode n = node;
        vxSceneGraphInterface(VxFrame::currentInstance())->getNodeTransform(n, tm, scale, &parentWorld);
    }
    return tm;
}

// Mac 0x467e18, Win 0x753000
void VxNodeInterface::setTransform(const VxNode&, const VxTransform&)
{
    warnDefault("setTransform");
}

// Mac 0x467de4, Win 0x753020
void VxNodeInterface::enableCulling(const VxNode&, bool)
{
    warnDefault("enableCulling");
}

// Mac 0x467dac, Win 0x753040
bool VxNodeInterface::isCullingEnabled(const VxNode&)
{
    warnDefault("isCullingEnabled");
    return false;
}

// Mac 0x467d78, Win 0x753060
void VxNodeInterface::enableLighting(const VxNode&, bool)
{
    warnDefault("enableLighting");
}

// Mac 0x467d40, Win 0x753080
bool VxNodeInterface::isLightingEnabled(const VxNode&)
{
    warnDefault("isLightingEnabled");
    return false;
}

// Mac 0x467d0c, Win 0x7530a0
void VxNodeInterface::enableFog(const VxNode&, bool)
{
    warnDefault("enableFog");
}

// Mac 0x467cd4, Win 0x7530c0
bool VxNodeInterface::isFogEnabled(const VxNode&)
{
    warnDefault("isFogEnabled");
    return false;
}

// Mac 0x467ca0, Win 0x7530e0
void VxNodeInterface::hide(const VxNode&, bool, int, int)
{
    warnDefault("hide");
}

// Mac 0x467c68, Win 0x753100
bool VxNodeInterface::isHidden(const VxNode&)
{
    warnDefault("isHidden");
    return true;
}

// Mac 0x467c30, Win 0x753120
bool VxNodeInterface::isSwitchNodeON(const VxNode&)
{
    warnDefault("isSwitchNodeON");
    return true;
}

// Mac 0x467bba, Win 0x753140: an empty box
VxNodeBoundingBox VxNodeInterface::getBoundingBox(const VxNode&)
{
    warnDefault("getBoundingBox");
    VxNodeBoundingBox b;
    b.min = VxVector3(DBL_MAX, DBL_MAX, DBL_MAX);
    b.max = VxVector3(-DBL_MAX, -DBL_MAX, -DBL_MAX);
    return b;
}

// Mac 0x467b58, Win 0x753180: an empty sphere (radius -1)
VxNodeBoundingSphere VxNodeInterface::getBoundingSphere(const VxNode&)
{
    warnDefault("getBoundingSphere");
    VxNodeBoundingSphere s;
    s.center = VxVector3(0, 0, 0);
    s.radius = -1.0;
    return s;
}

// Mac 0x467b20, Win 0x7531c0
int VxNodeInterface::getNodeType(const VxNode&)
{
    warnDefault("getNodeType");
    return 0;
}

// Mac 0x467aec, Win 0x7531e0
void VxNodeInterface::setEffect(const VxNode&, int, int)
{
    warnDefault("setEffect");
}

// Mac 0x4677c8, Win 0x753200
void VxNodeInterface::setColor(const VxNode&, const void*)
{
    warnDefault("setColor");
}

// Mac 0x46777c, Win 0x753220
VxNode VxNodeInterface::clone(const VxNode&, int)
{
    warnDefault("clone");
    return VxNode::sNull;
}

// Mac 0x467730, Win 0x753250
VxNode VxNodeInterface::load(const char*, int)
{
    warnDefault("load");
    return VxNode::sNull;
}

// Mac 0x4676fc, Win 0x753280
void VxNodeInterface::save(const VxNode&, const char*, int)
{
    warnDefault("save");
}

// Mac 0x4676c8, Win 0x7532a0
void VxNodeInterface::updateNodeTreeFromParts(const VxNode&)
{
    warnDefault("updateNodeTreeFromParts");
}

// Mac 0x467664, Win 0x753490
std::string VxNodeInterface::getNodeTypeName(const VxNode&)
{
    warnDefault("getNodeTypeName");
    return std::string("");
}

} // namespace Vx
