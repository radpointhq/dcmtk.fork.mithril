/*
 *
 *  Copyright (C) 2024-2026, Open Connections GmbH
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
 *  Purpose: Class for managing the ICC Profile Module
 *
 */

#include "dcmtk/config/osconfig.h" /* make sure OS specific configuration is included first */

#include "dcmtk/ofstd/ofcast.h"
#include "dcmtk/ofstd/ofcond.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/dcmdata/dcerror.h"
#include "dcmtk/dcmdata/dcvrobow.h"
#include "dcmtk/dcmdata/dcvrcs.h"
#include "dcmtk/dcmiod/modiccprofile.h"
#include "dcmtk/dcmiod/iodutil.h"
#include "dcmtk/dcmiod/iodrules.h"
#include "dcmtk/dcmiod/iccexample.h"

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

const OFString IODICCProfileModule::m_ModuleName = "ICCProfileModule";

IODICCProfileModule::IODICCProfileModule(OFshared_ptr<DcmItem> item, OFshared_ptr<IODRules> rules)
    : IODModule(item, rules)
{
    // reset element rules
    resetRules();
}

IODICCProfileModule::IODICCProfileModule()
    : IODModule()
{
    resetRules();
}


IODICCProfileModule::~IODICCProfileModule()
{
    // nothing to do
}

void IODICCProfileModule::resetRules()
{
    s_defaultRulesMutex.lock();
    if (!s_defaultRules)
    {
        s_defaultRules.reset(new IODRules());
        s_defaultRules->addRule(new IODRule(DCM_ICCProfile, "1", "1", getName(), DcmIODTypes::IE_IMAGE), OFTrue);
        s_defaultRules->addRule(new IODRule(DCM_ColorSpace, "1", "3", getName(), DcmIODTypes::IE_IMAGE), OFTrue);
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

void IODICCProfileModule::clearData()
{
    IODModule::clearData();
}

OFString IODICCProfileModule::getName() const
{
    return m_ModuleName;
}

OFCondition IODICCProfileModule::read(DcmItem& source, const OFBool clearOldData)
{
    if (clearOldData)
        clearData();

    IODComponent::read(source, OFFalse /* data already cleared */);
    return EC_Normal;
}

OFCondition IODICCProfileModule::write(DcmItem& destination)
{
    return IODComponent::write(destination);
}

// --- get() functionality ---

OFCondition IODICCProfileModule::getICCProfile(const Uint8*& value, Uint32& numBytes)
{
    DcmElement* elem = NULL;
    OFCondition result = DcmIODUtil::getAndCheckElementFromDataset(*m_Item, elem, getRules()->getByTag(DCM_ICCProfile));
    if (result.good())
    {
        if (elem->ident() == EVR_OB)

        {
            Uint8* val = NULL;
            result = OFstatic_cast(DcmOtherByteOtherWord*, elem)->getUint8Array(val);
            if (result.good())
            {
                // detach value pointer and set numBytes
                value = val;
                numBytes = elem->getLength();
                elem->detachValueField();
            }
        }
        else
        {
            result = EC_InvalidVR;
        }
    }
    delete elem; // clean up, we copied the value already
    return result;
}

OFCondition IODICCProfileModule::getColorSpace(OFString& value, const signed long pos) const
{
    return DcmIODUtil::getStringValueFromItem(DCM_ColorSpace, *m_Item, value, pos);
}

// --- set() functionality ---

OFCondition IODICCProfileModule::setICCProfile(const Uint8* value, const unsigned long numBytes, const OFBool /* not used */)
{
    return m_Item->putAndInsertUint8Array(DCM_ICCProfile, value, numBytes);
}

OFCondition IODICCProfileModule::setDefaultProfile(const OFBool& setColorSpaceDescription)
{
    OFCondition result = setICCProfile(DCMTK_SRGB_ICC_SAMPLE, DCMTK_SRGB_ICC_SAMPLE_LEN, OFFalse /* do not check, we expect this is valid */);
    if (result.good() && setColorSpaceDescription)
    {
        result = setColorSpace("SRGB", OFFalse /* do not check, we expect this is valid */);
    }
    return result;
}

OFCondition IODICCProfileModule::setColorSpace(const OFString& value, const OFBool checkValue)
{
    OFCondition result;
    IODRule* rule = getRules()->getByTag(DCM_ColorSpace);
    if (rule)
    {
        result = (checkValue) ? DcmCodeString::checkStringValue(value, rule->getVM()) : EC_Normal;
        if (result.good()) result = m_Item->putAndInsertOFStringArray(DCM_ColorSpace, value);
    }
    else
    {
        result = IOD_EC_NoSuchRule;
    }
    return result;
}
