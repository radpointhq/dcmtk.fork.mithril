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
 *  Purpose: Class for managing the Segmentation Series Module
 *
 */

#include "dcmtk/config/osconfig.h" /* make sure OS specific configuration is included first */
#include "dcmtk/dcmiod/modsegmentationseries.h"
#include "dcmtk/dcmiod/iodutil.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/dcmdata/dcvrui.h"
#include "dcmtk/dcmdata/dcvris.h"

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

const OFString IODSegmentationSeriesModule::m_ModuleName = "SegmentationSeriesModule";

IODSegmentationSeriesModule::IODSegmentationSeriesModule(OFshared_ptr<DcmItem> item, OFshared_ptr<IODRules> rules)
    : IODModule(item, rules)
{
    // reset element rules
    resetRules();
}

IODSegmentationSeriesModule::IODSegmentationSeriesModule()
    : IODModule()
{
    resetRules();
}

IODSegmentationSeriesModule::~IODSegmentationSeriesModule()
{
}

OFString IODSegmentationSeriesModule::getName() const
{
    return m_ModuleName;
}

void IODSegmentationSeriesModule::resetRules()
{
    s_defaultRulesMutex.lock();
    if (!s_defaultRules)
    {
        s_defaultRules.reset(new IODRules());
        s_defaultRules->addRule(new IODRule(DCM_Modality, "1", "1", getName(), DcmIODTypes::IE_SERIES, "SEG"), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_SeriesNumber, "1", "1", getName(), DcmIODTypes::IE_SERIES, "1"), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_ReferencedSOPClassUID, "1", "1C", getName(), DcmIODTypes::IE_SERIES), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_ReferencedSOPInstanceUID, "1", "1C", getName(), DcmIODTypes::IE_SERIES), OFTrue);
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

OFCondition IODSegmentationSeriesModule::getModality(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_Modality, *m_Item, value, pos);
}

OFCondition IODSegmentationSeriesModule::getSeriesNumber(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_SeriesNumber, *m_Item, value, pos);
}

OFCondition IODSegmentationSeriesModule::getPPSSOPClassUID(OFString& value, const signed long pos) const
{
    DcmItem* localItem = NULL;
    OFCondition result = m_Item->findAndGetSequenceItem(DCM_ReferencedPerformedProcedureStepSequence, localItem, 0);
    if (result.good())
    {
        result = DcmIODUtil::getStringValueFromItem(DCM_ReferencedSOPClassUID, *localItem, value, pos);
    }
    return result;
}

OFCondition IODSegmentationSeriesModule::getPPSSOPInstanceUID(OFString& value, const signed long pos) const
{
    DcmItem* localItem = NULL;
    OFCondition result = m_Item->findAndGetSequenceItem(DCM_ReferencedPerformedProcedureStepSequence, localItem, 0);
    if (result.good())
    {
        result = DcmIODUtil::getStringValueFromItem(DCM_ReferencedSOPInstanceUID, *localItem, value, pos);
    }
    return result;
}

OFCondition IODSegmentationSeriesModule::setSeriesNumber(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmIntegerString::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
        result = m_Item->putAndInsertOFStringArray(DCM_SeriesNumber, value);
    return result;
}

OFCondition IODSegmentationSeriesModule::setPPSSOPClassUID(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmUniqueIdentifier::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
    {
        DcmItem* localItem = NULL;
        result = m_Item->findOrCreateSequenceItem(DCM_ReferencedPerformedProcedureStepSequence, localItem, 0);
        if (result.good())
        {
            result = m_Item->putAndInsertOFStringArray(DCM_ReferencedSOPClassUID, value);
        }
    }
    return result;
}

OFCondition IODSegmentationSeriesModule::setPPSSOPInstanceUID(const OFString& value, const OFBool checkValue)
{
    OFCondition result = (checkValue) ? DcmUniqueIdentifier::checkStringValue(value, "1") : EC_Normal;
    if (result.good())
    {
        DcmItem* localItem = NULL;
        result = m_Item->findOrCreateSequenceItem(DCM_ReferencedPerformedProcedureStepSequence, localItem, 0);
        if (result.good())
        {
            result = m_Item->putAndInsertOFStringArray(DCM_ReferencedSOPInstanceUID, value);
        }
    }
    return result;
}
