////////////////////////////////////////////////////////////////////
// Copyright (C) 2011, SafeNet, Inc. All rights reserved.
//
// HASP(R) is a registered trademark of SafeNet, Inc. 
//
//
// 
////////////////////////////////////////////////////////////////////
#include "hasp_api_cpp_.h"
#include <string.h>

////////////////////////////////////////////////////////////////////
// Construction/Destruction
////////////////////////////////////////////////////////////////////

ChaspHandle::ChaspHandle() {
    clear();
}

ChaspHandle::~ChaspHandle() {
    clear();
}

////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//! Equality operator
////////////////////////////////////////////////////////////////////
bool ChaspHandle::operator==(const ChaspHandle& other) const
{
    return (0 == memcmp(this, &other, sizeof(*this)));
}

////////////////////////////////////////////////////////////////////
//! Inequality operator
////////////////////////////////////////////////////////////////////
bool ChaspHandle::operator!=(const ChaspHandle& other) const
{
    return !this->operator==(other);
}

////////////////////////////////////////////////////////////////////
//! Clears the handle
////////////////////////////////////////////////////////////////////
void ChaspHandle::clear() {
    memset(this, 0x00, sizeof(*this));
}

////////////////////////////////////////////////////////////////////
//! Determines if the handle is valid
////////////////////////////////////////////////////////////////////
bool ChaspHandle::isNull() const
{
    return (0 == m_ulIndex);
}
