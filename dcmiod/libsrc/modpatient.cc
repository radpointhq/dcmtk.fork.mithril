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
 *  Purpose: Class for managing the Patient Module
 *
 */

#include "dcmtk/config/osconfig.h" /* make sure OS specific configuration is included first */
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/dcmiod/modpatient.h"
#include "dcmtk/dcmiod/iodutil.h"
#include "dcmtk/dcmdata/dcvrpn.h"
#include "dcmtk/dcmdata/dcvrlo.h"
#include "dcmtk/dcmdata/dcvrda.h"
#include "dcmtk/dcmdata/dcvrcs.h"

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

const OFString IODPatientModule::m_ModuleName = "PatientModule";

IODPatientModule::IODPatientModule(OFshared_ptr<DcmItem> item, OFshared_ptr<IODRules> rules)
    : IODModule(item, rules)
{
    // reset element rules
    resetRules();
}

OFString IODPatientModule::getName() const
{
    return m_ModuleName;
}

IODPatientModule::IODPatientModule()
    : IODModule()
{
    resetRules();
}

void IODPatientModule::resetRules()
{
    s_defaultRulesMutex.lock();
    if (!s_defaultRules)
    {
        s_defaultRules.reset(new IODRules());
        s_defaultRules->addRule(new IODRule(DCM_PatientName, "1", "2", getName(), DcmIODTypes::IE_PATIENT), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_PatientID, "1", "2", getName(), DcmIODTypes::IE_PATIENT), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_PatientBirthDate, "1", "2", getName(), DcmIODTypes::IE_PATIENT), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_PatientSex, "1", "2", getName(), DcmIODTypes::IE_PATIENT), OFTrue);
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

IODPatientModule::~IODPatientModule()
{
}

// --- get attributes (C++ string) ---

OFCondition IODPatientModule::getPatientName(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_PatientName, *m_Item, value, pos);
}

OFCondition IODPatientModule::getPatientBirthDate(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_PatientBirthDate, *m_Item, value, pos);
}

OFCondition IODPatientModule::getPatientSex(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_PatientSex, *m_Item, value, pos);
}

OFCondition IODPatientModule::getPatientID(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_PatientID, *m_Item, value, pos);
}

// --- set attributes ---

OFCondition IODPatientModule::setPatientName(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmPersonName::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
        result = m_Item->putAndInsertOFStringArray(DCM_PatientName, value);
    return result;
}

OFCondition IODPatientModule::setPatientID(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmLongString::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
        result = m_Item->putAndInsertOFStringArray(DCM_PatientID, value);
    return result;
}

OFCondition IODPatientModule::setPatientBirthDate(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmDate::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
        result = m_Item->putAndInsertOFStringArray(DCM_PatientBirthDate, value);
    return result;
}

OFCondition IODPatientModule::setPatientSex(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmCodeString::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
        result = m_Item->putAndInsertOFStringArray(DCM_PatientSex, value);
    return result;
}
