/*
 *
 *  Copyright (C) 2003-2026, OFFIS e.V.
 *  All rights reserved.  See COPYRIGHT file for details.
 *
 *  This software and supporting documentation were developed by
 *
 *    OFFIS e.V.
 *    R&D Division Health
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *
 *  Module: dcmsr
 *
 *  Author: Joerg Riesmeier
 *
 *  Purpose:
 *    classes: DSRKeyObjectSelectionDocumentConstraintChecker
 *
 */


#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#include "dcmtk/dcmsr/dsrkeycc.h"
#include "dcmtk/dcmsr/codes/dcm.h"


DSRKeyObjectSelectionDocumentConstraintChecker::DSRKeyObjectSelectionDocumentConstraintChecker()
  : DSRIODConstraintChecker()
{
}


DSRKeyObjectSelectionDocumentConstraintChecker::~DSRKeyObjectSelectionDocumentConstraintChecker()
{
}


OFBool DSRKeyObjectSelectionDocumentConstraintChecker::isByReferenceAllowed() const
{
    return OFFalse;
}


OFBool DSRKeyObjectSelectionDocumentConstraintChecker::isTemplateSupportRequired() const
{
    return OFTrue;
}


OFCondition DSRKeyObjectSelectionDocumentConstraintChecker::getRootTemplateIdentification(OFString &templateIdentifier,
                                                                                          OFString &mappingResource) const
{
    templateIdentifier = "2010";
    mappingResource = "DCMR";
    return EC_Normal;
}


DSRTypes::E_DocumentType DSRKeyObjectSelectionDocumentConstraintChecker::getDocumentType() const
{
    return DT_KeyObjectSelectionDocument;
}


OFBool DSRKeyObjectSelectionDocumentConstraintChecker::checkContentRelationship(const E_ValueType sourceValueType,
                                                                                const E_RelationshipType relationshipType,
                                                                                const E_ValueType targetValueType,
                                                                                const OFBool byReference) const
{
    /* the following code implements the constraints of table A.35.4-2 in DICOM PS3.3 */
    OFBool result = OFFalse;
    /* by-reference relationships not allowed at all */
    if (!byReference)
    {
        OFBool printWarning = OFFalse;
        /* row 1 of the table */
        if ((relationshipType == RT_contains) && (sourceValueType == VT_Container))
        {
            result = (targetValueType == VT_Text)      || (targetValueType == VT_Image) || (targetValueType == VT_Waveform) ||
                     (targetValueType == VT_Composite) || (targetValueType == VT_Container);
            /* report a warning on CP-2595 */
            printWarning = (targetValueType == VT_Container);
        }
        /* row 2 of the table */
        else if ((relationshipType == RT_hasObsContext) && (sourceValueType == VT_Container))
        {
            result = (targetValueType == VT_Text)  || (targetValueType == VT_Code) || (targetValueType == VT_UIDRef) ||
                     (targetValueType == VT_PName) || (targetValueType == VT_Container);
        }
        /* row 3 of the table */
        else if ((relationshipType == RT_hasConceptMod) && (sourceValueType == VT_Container))
        {
            result = (targetValueType == VT_Code);
        }
        /* row 4 of the table */
        else if ((relationshipType == RT_hasAcqContext) && (sourceValueType == VT_Container))
        {
            result = (targetValueType == VT_Code)     || (targetValueType == VT_Date)   || (targetValueType == VT_Time) ||
                     (targetValueType == VT_DateTime) || (targetValueType == VT_UIDRef) || (targetValueType == VT_Num)  ||
                     (targetValueType == VT_Text);
            printWarning = result;
        }
        /* row 5 of the table */
        else if ((relationshipType == RT_hasAcqContext) && (sourceValueType == VT_Image))
        {
            result = (targetValueType == VT_Code)     || (targetValueType == VT_Date)   || (targetValueType == VT_Time) ||
                     (targetValueType == VT_DateTime) || (targetValueType == VT_UIDRef) || (targetValueType == VT_Num)  ||
                     (targetValueType == VT_Text);
            /* report a warning on CP-2595 */
            printWarning = result;
        }
        /* row 6 of the table */
        else if ((relationshipType == RT_hasAcqContext) && (sourceValueType == VT_Composite))
        {
            result = (targetValueType == VT_Code)     || (targetValueType == VT_Date)   || (targetValueType == VT_Time) ||
                     (targetValueType == VT_DateTime) || (targetValueType == VT_UIDRef) || (targetValueType == VT_Num)  ||
                     (targetValueType == VT_Text);
            /* report a warning on CP-2595 */
            printWarning = result;
        }
        /* row 7 of the table */
        else if ((relationshipType == RT_hasAcqContext) && (sourceValueType == VT_Waveform))
        {
            result = (targetValueType == VT_Code)     || (targetValueType == VT_Date)   || (targetValueType == VT_Time) ||
                     (targetValueType == VT_DateTime) || (targetValueType == VT_UIDRef) || (targetValueType == VT_Num)  ||
                     (targetValueType == VT_Text);
            /* report a warning on CP-2595 */
            printWarning = result;
        }
        /* print a warning on breaking changes from CP-2595 */
        if (printWarning)
        {
            DCMSR_WARN("Content relationship breaks backward compatibility with implementations prior to CP-2595");
            /* print more details to the debug logger */
            DCMSR_DEBUG("  Adding \"" << relationshipTypeToReadableName(relationshipType)
                << " " << valueTypeToDefinedTerm(targetValueType) << "\" to " << valueTypeToDefinedTerm(sourceValueType)
                << " in " << documentTypeToReadableName(getDocumentType()) << " IOD");
        }
    }
    return result;
}


OFCondition DSRKeyObjectSelectionDocumentConstraintChecker::checkRootConceptName(const DSRCodedEntryValue &conceptName) const
{
    /* TODO: need to check for codes from CID 7010 "Key Object Selection Document Title" */
    return conceptName.isEqual(CODE_DCM_ManifestWithDescription, "01" /*defaultCodingSchemeVersion*/) ?
        SR_EC_UnsupportedRootConceptName : DSRIODConstraintChecker::checkRootConceptName(conceptName);
}
