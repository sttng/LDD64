// Vortex 5.0.0 build 800 "vortexldd41" - VxUniqueNameList (restored).
// Mac TU 522 0x47e3aa-0x47f528 (file name unknown). All names inferred.
#include "Vx/VxUniqueNameList.h"
#include <ctype.h>
#include <sstream>

namespace Vx {

// Mac 0x47e3aa (isdigit through the C locale's rune table)
static int isDigitChar(int c)
{
    return (unsigned)c < 0x100 && isdigit(c) ? 1 : 0;
}

// Mac 0x47e3da (C2), 0x47e3f6 (C1)
VxUniqueNameList::VxUniqueNameList()
{
}

// Mac 0x47e440 (D2), 0x47e47a (D1)
VxUniqueNameList::~VxUniqueNameList()
{
}

// Mac 0x47e40a (std::find, 0x47f1ba)
bool VxUniqueNameList::contains(const std::string& name) const
{
    for (const std::string* i = mNames.begin(); i != mNames.end(); ++i)
        if (*i == name)
            return true;
    return false;
}

// Mac 0x47e48e (static, string in EAX): for "base(123)" the index of '(', else -1. The name
// must be longer than 3 characters and end with ')'; only digits may stand between.
int VxUniqueNameList::findNumberSuffix(const std::string& name)
{
    if (name.size() > 3 && name[name.size() - 1] == ')') {
        std::string s(name.c_str());
        std::string::size_type open = s.find_last_of("(");
        if (open != std::string::npos) {
            std::string digits(s, open + 1, std::string::npos);
            digits.erase(digits.size() - 1);
            int n = 0;
            for (std::string::size_type i = 0; i < digits.size(); ++i)
                n += isDigitChar((unsigned char)digits[i]) != 0;
            if ((int)digits.size() == n)
                return (int)open;
        }
    }
    return -1;
}

// Mac 0x47e758
void VxUniqueNameList::removeNumberSuffix(std::string& name)
{
    int i = findNumberSuffix(name);
    if (i != -1)
        name.erase((std::string::size_type)i);
}

// Mac 0x47e7b4
bool VxUniqueNameList::hasNumberSuffix(const std::string& name)
{
    return findNumberSuffix(name) != -1;
}

// Mac 0x47e7ce: base = name without a "(n)" suffix; base, then base(1), base(2), ... until
// the list does not contain it
std::string VxUniqueNameList::makeUniqueName(const std::string& name) const
{
    std::string base(name);
    if (hasNumberSuffix(base))
        removeNumberSuffix(base);
    std::string result(base);
    unsigned long i = 1;
    while (contains(result)) {
        std::ostringstream os;
        os << base.c_str() << '(' << i << ')';
        result = os.str();
        ++i;
    }
    return result;
}

// Mac 0x47ef56
const std::string& VxUniqueNameList::addUnique(const std::string& name)
{
    std::string s(name);
    if (contains(s))
        s = makeUniqueName(s);
    mNames.push_back(s);
    return mNames.end()[-1];
}

// Mac 0x47f0e8
void VxUniqueNameList::add(const std::string& name)
{
    if (!contains(name))
        mNames.push_back(name);
    else
        addUnique(name);
}

} // namespace Vx
