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
 *  Purpose: Class for managing the SOP Common Module
 *
 */

#include "dcmtk/config/osconfig.h" /* make sure OS specific configuration is included first */
#include "dcmtk/dcmiod/modsopcommon.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/dcmdata/dcvrcs.h"
#include "dcmtk/dcmdata/dcvrda.h"
#include "dcmtk/dcmdata/dcvrsh.h"
#include "dcmtk/dcmdata/dcvrtm.h"
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

const OFString IODSOPCommonModule::m_ModuleName = "SOPCommonModule";

IODSOPCommonModule::IODSOPCommonModule(OFshared_ptr<DcmItem> item, OFshared_ptr<IODRules> rules)
    : IODModule(item, rules)
{
    // reset element rules
    resetRules();
}

IODSOPCommonModule::IODSOPCommonModule()
    : IODModule()
{
    resetRules();
}

void IODSOPCommonModule::resetRules()
{
    s_defaultRulesMutex.lock();
    if (!s_defaultRules)
    {
        s_defaultRules.reset(new IODRules());
        s_defaultRules->addRule(new IODRule(DCM_SOPClassUID, "1", "1", getName(), DcmIODTypes::IE_INSTANCE), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_SOPInstanceUID, "1", "1", getName(), DcmIODTypes::IE_INSTANCE), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_SpecificCharacterSet, "1-n", "1C", getName(), DcmIODTypes::IE_INSTANCE), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_InstanceCreationDate, "1", "3", getName(), DcmIODTypes::IE_INSTANCE), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_InstanceCreationTime, "1", "3", getName(), DcmIODTypes::IE_INSTANCE), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_InstanceCreatorUID, "1", "3", getName(), DcmIODTypes::IE_INSTANCE), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_TimezoneOffsetFromUTC, "1", "3", getName(), DcmIODTypes::IE_INSTANCE), OFTrue);
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

OFString IODSOPCommonModule::getName() const
{
    return m_ModuleName;
}

void IODSOPCommonModule::inventMissing()
{
    // SOP Instance UID
    ensureInstanceUID();

    // All other attributes
    IODComponent::inventMissing();
}

IODSOPCommonModule::~IODSOPCommonModule()
{
}

void IODSOPCommonModule::ensureInstanceUID(const OFBool correctInvalid)
{
    OFString uidstr;

    /* create new sop instance UID if required */
    if (getSOPInstanceUID(uidstr).bad() || uidstr.empty())
    {
        setSOPInstanceUID(DcmIODUtil::createUID(0 /* Instance Level */));
    }
    else if (!uidstr.empty() && correctInvalid)
    {
        if (DcmUniqueIdentifier::checkStringValue(uidstr, "1").bad())
        {
            setSOPInstanceUID(DcmIODUtil::createUID(0 /* Instance Level */));
        }
    }
}

OFCondition IODSOPCommonModule::getSpecificCharacterSet(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_SpecificCharacterSet, *m_Item, value, pos);
}

OFCondition IODSOPCommonModule::getSOPInstanceUID(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_SOPInstanceUID, *m_Item, value, pos);
}

OFCondition IODSOPCommonModule::getSOPClassUID(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_SOPClassUID, *m_Item, value, pos);
}

OFCondition IODSOPCommonModule::getInstanceCreationDate(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_InstanceCreationDate, *m_Item, value, pos);
}

OFCondition IODSOPCommonModule::getInstanceCreationTime(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_InstanceCreationTime, *m_Item, value, pos);
}

OFCondition IODSOPCommonModule::getInstanceCreatorUID(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_InstanceCreatorUID, *m_Item, value, pos);
}

OFCondition IODSOPCommonModule::getTimezoneOffsetFromUTC(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_TimezoneOffsetFromUTC, *m_Item, value, pos);
}

OFCondition IODSOPCommonModule::setSpecificCharacterSet(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmCodeString::checkStringValue(value, "1-n") : EC_Normal;
    if (result.good())
    {
        result = m_Item->putAndInsertOFStringArray(DCM_SpecificCharacterSet, value);
    }
    return result;
}

OFCondition IODSOPCommonModule::setSOPClassUID(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmUniqueIdentifier::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
    {
        result = m_Item->putAndInsertOFStringArray(DCM_SOPClassUID, value);
    }
    return result;
}

OFCondition IODSOPCommonModule::setSOPInstanceUID(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmUniqueIdentifier::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
    {
        result = m_Item->putAndInsertOFStringArray(DCM_SOPInstanceUID, value);
    }
    return result;
}

OFCondition IODSOPCommonModule::setInstanceCreationDate(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmDate::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
    {
        result = m_Item->putAndInsertOFStringArray(DCM_InstanceCreationDate, value);
    }
    return result;
}

OFCondition IODSOPCommonModule::setInstanceCreationTime(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmTime::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
    {
        result = m_Item->putAndInsertOFStringArray(DCM_InstanceCreationTime, value);
    }
    return result;
}

OFCondition IODSOPCommonModule::setInstanceCreatorUID(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmUniqueIdentifier::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
    {
        result = m_Item->putAndInsertOFStringArray(DCM_InstanceCreatorUID, value);
    }
    return result;
}

OFCondition IODSOPCommonModule::setTimeZoneOffsetFromUTC(const OFString& value, const OFBool checkValue) const
{
    OFCondition result;
    if (checkValue)
    {
        result = DcmShortString::checkStringValue(value, "1");
        if (result.good())
        {
            if (value.length() != 5)
                result = IOD_EC_InvalidElementValue;
        }
    }
    if (result.good())
    {
        result = m_Item->putAndInsertOFStringArray(DCM_TimezoneOffsetFromUTC, value);
    }
    return result;
}
