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
 *  Purpose: Class for managing the Patient Study Module
 *
 */


#include "dcmtk/config/osconfig.h" /* make sure OS specific configuration is included first */
#include "dcmtk/dcmiod/modpatientstudy.h"
#include "dcmtk/dcmiod/iodutil.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/dcmdata/dcvrlo.h"
#include "dcmtk/dcmdata/dcvras.h"
#include "dcmtk/dcmdata/dcvrds.h"

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

const OFString IODPatientStudyModule::m_ModuleName = "PatientStudyModule";

IODPatientStudyModule::IODPatientStudyModule(OFshared_ptr<DcmItem> item, OFshared_ptr<IODRules> rules)
    : IODModule(item, rules)
{
    resetRules();
}

OFString IODPatientStudyModule::getName() const
{
    return m_ModuleName;
}

IODPatientStudyModule::IODPatientStudyModule()
    : IODModule()
{
    resetRules();
}

void IODPatientStudyModule::resetRules()
{
    s_defaultRulesMutex.lock();
    if (!s_defaultRules)
    {
        s_defaultRules.reset(new IODRules());
        s_defaultRules->addRule(new IODRule(DCM_AdmittingDiagnosesDescription, "1-n", "3", getName(), DcmIODTypes::IE_STUDY), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_PatientAge, "1", "3", getName(), DcmIODTypes::IE_STUDY), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_PatientSize, "1", "3", getName(), DcmIODTypes::IE_STUDY), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_PatientWeight, "1", "3", getName(), DcmIODTypes::IE_STUDY), OFTrue);
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

IODPatientStudyModule::~IODPatientStudyModule()
{
}

// --- get attributes (C++ string) ---

OFCondition IODPatientStudyModule::getAdmittingDiagnosesDescription(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_AdmittingDiagnosesDescription, *m_Item, value, pos);
}

OFCondition IODPatientStudyModule::getPatientAge(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_PatientAge, *m_Item, value, pos);
}

OFCondition IODPatientStudyModule::getPatientWeight(Float64& value, const unsigned long pos) const
{
    return DcmIODUtil::getFloat64ValueFromItem(DCM_PatientWeight, *m_Item, value, pos);
}

OFCondition IODPatientStudyModule::getPatientSize(Float64& value, const unsigned long pos) const
{
    return DcmIODUtil::getFloat64ValueFromItem(DCM_PatientSize, *m_Item, value, pos);
}

// --- set attributes ---

OFCondition IODPatientStudyModule::setAdmittingDiagnosesDescription(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmLongString::checkStringValue(value, "1-n") : EC_Normal;
    if (result.good())
        result = m_Item->putAndInsertOFStringArray(DCM_AdmittingDiagnosesDescription, value);
    return result;
}

OFCondition IODPatientStudyModule::setPatientAge(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmAgeString::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
        result = m_Item->putAndInsertOFStringArray(DCM_PatientAge, value);
    return result;
}

OFCondition IODPatientStudyModule::setPatientSize(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmDecimalString::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
        result = m_Item->putAndInsertOFStringArray(DCM_PatientSize, value);
    return result;
}

OFCondition IODPatientStudyModule::setPatientWeight(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmDecimalString::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
        result = m_Item->putAndInsertOFStringArray(DCM_PatientWeight, value);
    return result;
}
