/*
 *
 *  Copyright (C) 2015-2026, Open Connections GmbH
 *  All rights reserved.  See COPYRIGHT file for details.
 *
 *  This software and supporting documentation are maintained by
 *
 *    OFFIS e.V.
 *    R&D Division Health
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *
 *  Module: dcmiod
 *
 *  Author: Michael Onken
 *
 *  Purpose: Class for managing the Frame of Reference Module
 *
 */

#include "dcmtk/config/osconfig.h" /* make sure OS specific configuration is included first */
#include "dcmtk/dcmiod/modfor.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/dcmdata/dcvrlo.h"
#include "dcmtk/dcmdata/dcvrui.h"
#include "dcmtk/dcmiod/iodutil.h"

// Per-class default IODRules shared across instances (copy-on-write; see
// IODComponent). Must live at namespace scope, not inside a function: in
// C++98 the initialization of function-local statics is not thread-safe,
// and that applies to the OFMutex itself -- a mutex cannot guard its own
// construction. Namespace-scope statics are initialized before main() on
// the single startup thread, so both objects are guaranteed ready when
// user threads first try to lock.
namespace
{
OFshared_ptr<IODRules> s_defaultRules;
OFMutex s_defaultRulesMutex;
}

const OFString IODFoRModule::m_ModuleName = "FrameOfReferenceModule";

IODFoRModule::IODFoRModule(OFshared_ptr<DcmItem> item, OFshared_ptr<IODRules> rules)
    : IODModule(item, rules)
{
    // reset element rules
    resetRules();
}

IODFoRModule::IODFoRModule()
    : IODModule()
{
    resetRules();
}

void IODFoRModule::resetRules()
{
    s_defaultRulesMutex.lock();
    if (!s_defaultRules)
    {
        s_defaultRules.reset(new IODRules());
        s_defaultRules->addRule(new IODRule(DCM_FrameOfReferenceUID, "1", "1", getName(), DcmIODTypes::IE_FOR), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_PositionReferenceIndicator, "1", "2", getName(), DcmIODTypes::IE_FOR), OFTrue);
    }
    s_defaultRulesMutex.unlock();
    if (!m_ExternalRules)
    {
        m_Rules       = s_defaultRules;
        m_HasOwnRules = OFFalse;
    }
    else
    {
        IODRules::iterator it = s_defaultRules->begin();
        while (it != s_defaultRules->end())
        {
            m_Rules->addRule(it->second->clone(), OFTrue);
            ++it;
        }
    }
}

OFString IODFoRModule::getName() const
{
    return m_ModuleName;
}

IODFoRModule::~IODFoRModule()
{
    // Nothing to do
}

OFCondition IODFoRModule::getFrameOfReferenceUID(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_FrameOfReferenceUID, *m_Item, value, pos);
}

OFCondition IODFoRModule::getPositionReferenceIndicator(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_PositionReferenceIndicator, *m_Item, value, pos);
}

OFCondition IODFoRModule::setFrameOfReferenceUID(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmUniqueIdentifier::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
        result = m_Item->putAndInsertOFStringArray(DCM_FrameOfReferenceUID, value);
    return result;
}

OFCondition IODFoRModule::setPositionReferenceIndicator(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmLongString::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
        result = m_Item->putAndInsertOFStringArray(DCM_PositionReferenceIndicator, value);
    return result;
}

void IODFoRModule::ensureFrameOfReferenceUID(const OFBool correctInvalid)
{
    OFString uidstr;

    // Create new Frame of Reference instance UID if required
    if (getFrameOfReferenceUID(uidstr).bad() || uidstr.empty())
    {
        setFrameOfReferenceUID(DcmIODUtil::createUID(1 /* Series Level */));
    }
    else if (!uidstr.empty() && correctInvalid)
    {
        if (DcmUniqueIdentifier::checkStringValue(uidstr, "1").bad())
        {
            setFrameOfReferenceUID(DcmIODUtil::createUID(1 /* Series Level */));
        }
    }
}
