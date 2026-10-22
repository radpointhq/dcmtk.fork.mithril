/*
 *
 *  Copyright (C) 2025-2026, OFFIS e.V.
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
 *  Module:  dcmfg
 *
 *  Author:  Michael Onken
 *
 *  Purpose: Tests for FGInterface class
 *
 */

#include "dcmtk/config/osconfig.h"

#include "dcmtk/dcmfg/fgfracon.h"
#include "dcmtk/dcmfg/fginterface.h"
#include "dcmtk/dcmfg/fgplanpo.h"
#include "dcmtk/dcmfg/fgpixmsr.h"
#include "dcmtk/ofstd/oftest.h"
#include "dcmtk/dcmdata/dcdict.h"
#include "dcmtk/dcmdata/dcdeftag.h"
#include "dcmtk/dcmdata/dcsequen.h"



OFTEST(dcmfg_fginterface_add_get)
{
    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFCHECK_FAIL("no data dictionary loaded, check environment variable: " DCM_DICT_ENVIRONMENT_VARIABLE);
        return;
    }

    FGInterface fgi;
    OFCHECK(fgi.getNumberOfFrames() == 0);

    // Add per-frame groups for 3 frames
    FGFrameContent fc0, fc1, fc2;
    OFCHECK(fc0.setStackID("S0").good());
    OFCHECK(fc1.setStackID("S1").good());
    OFCHECK(fc2.setStackID("S2").good());

    OFCHECK(fgi.addPerFrame(0, fc0).good());
    OFCHECK(fgi.addPerFrame(1, fc1).good());
    OFCHECK(fgi.addPerFrame(2, fc2).good());
    OFCHECK(fgi.getNumberOfFrames() == 3);

    // Retrieve per-frame and verify content
    OFBool isPerFrame = OFFalse;
    FGBase* fg = fgi.get(0, DcmFGTypes::EFG_FRAMECONTENT, isPerFrame);
    OFCHECK(fg != NULL);
    OFCHECK(isPerFrame == OFTrue);
    if (fg)
    {
        OFString val;
        OFstatic_cast(FGFrameContent*, fg)->getStackID(val);
        OFCHECK(val == "S0");
    }

    fg = fgi.get(2, DcmFGTypes::EFG_FRAMECONTENT, isPerFrame);
    OFCHECK(fg != NULL);
    if (fg)
    {
        OFString val;
        OFstatic_cast(FGFrameContent*, fg)->getStackID(val);
        OFCHECK(val == "S2");
    }

    // Out-of-range frame returns NULL
    fg = fgi.get(99, DcmFGTypes::EFG_FRAMECONTENT);
    OFCHECK(fg == NULL);

    // Non-existent type returns NULL
    fg = fgi.get(0, DcmFGTypes::EFG_PLANEPOSPATIENT);
    OFCHECK(fg == NULL);
}


OFTEST(dcmfg_fginterface_shared)
{
    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFCHECK_FAIL("no data dictionary loaded, check environment variable: " DCM_DICT_ENVIRONMENT_VARIABLE);
        return;
    }

    FGInterface fgi;

    // Add per-frame first so that we have frames
    FGFrameContent fc0, fc1;
    OFCHECK(fc0.setStackID("F0").good());
    OFCHECK(fc1.setStackID("F1").good());
    OFCHECK(fgi.addPerFrame(0, fc0).good());
    OFCHECK(fgi.addPerFrame(1, fc1).good());

    // Add a shared group
    FGPixelMeasures pm;
    OFCHECK(pm.setPixelSpacing("0.5\\0.5").good());
    OFCHECK(fgi.addShared(pm).good());

    // get() should return shared group for any frame
    OFBool isPerFrame = OFTrue;
    FGBase* fg = fgi.get(0, DcmFGTypes::EFG_PIXELMEASURES, isPerFrame);
    OFCHECK(fg != NULL);
    OFCHECK(isPerFrame == OFFalse);

    fg = fgi.get(1, DcmFGTypes::EFG_PIXELMEASURES, isPerFrame);
    OFCHECK(fg != NULL);
    OFCHECK(isPerFrame == OFFalse);

    // getShared() accessor
    const FunctionalGroups* shared = fgi.getShared();
    OFCHECK(shared != NULL);

    // Delete shared
    OFCHECK(fgi.deleteShared(DcmFGTypes::EFG_PIXELMEASURES) == OFTrue);
    fg = fgi.get(0, DcmFGTypes::EFG_PIXELMEASURES);
    OFCHECK(fg == NULL);

    // Delete non-existent shared returns OFFalse
    OFCHECK(fgi.deleteShared(DcmFGTypes::EFG_PIXELMEASURES) == OFFalse);
}


OFTEST(dcmfg_fginterface_delete_perframe)
{
    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFCHECK_FAIL("no data dictionary loaded, check environment variable: " DCM_DICT_ENVIRONMENT_VARIABLE);
        return;
    }

    FGInterface fgi;
    FGFrameContent fc0, fc1, fc2;
    fc0.setStackID("A");
    fc1.setStackID("B");
    fc2.setStackID("C");
    OFCHECK(fgi.addPerFrame(0, fc0).good());
    OFCHECK(fgi.addPerFrame(1, fc1).good());
    OFCHECK(fgi.addPerFrame(2, fc2).good());

    // Delete single per-frame group
    OFCHECK(fgi.deletePerFrame(1, DcmFGTypes::EFG_FRAMECONTENT) == OFTrue);
    OFCHECK(fgi.get(1, DcmFGTypes::EFG_FRAMECONTENT) == NULL);

    // Other frames unaffected
    OFCHECK(fgi.get(0, DcmFGTypes::EFG_FRAMECONTENT) != NULL);
    OFCHECK(fgi.get(2, DcmFGTypes::EFG_FRAMECONTENT) != NULL);

    // Delete from out-of-range frame
    OFCHECK(fgi.deletePerFrame(99, DcmFGTypes::EFG_FRAMECONTENT) == OFFalse);
}


OFTEST(dcmfg_fginterface_delete_frame)
{
    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFCHECK_FAIL("no data dictionary loaded, check environment variable: " DCM_DICT_ENVIRONMENT_VARIABLE);
        return;
    }

    FGInterface fgi;
    FGFrameContent fc0, fc1;
    fc0.setStackID("X");
    fc1.setStackID("Y");
    OFCHECK(fgi.addPerFrame(0, fc0).good());
    OFCHECK(fgi.addPerFrame(1, fc1).good());

    // Delete all groups for frame 0
    size_t deleted = fgi.deleteFrame(0);
    OFCHECK(deleted > 0);
    OFCHECK(fgi.get(0, DcmFGTypes::EFG_FRAMECONTENT) == NULL);

    // Frame 1 unaffected
    OFCHECK(fgi.get(1, DcmFGTypes::EFG_FRAMECONTENT) != NULL);

    // getNumberOfFrames still returns 2 (vector size unchanged, slot is NULL)
    OFCHECK(fgi.getNumberOfFrames() == 2);

    // Delete again returns 0
    OFCHECK(fgi.deleteFrame(0) == 0);
}


OFTEST(dcmfg_fginterface_out_of_order_insert)
{
    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFCHECK_FAIL("no data dictionary loaded, check environment variable: " DCM_DICT_ENVIRONMENT_VARIABLE);
        return;
    }

    FGInterface fgi;
    FGFrameContent fc5, fc2, fc0;
    fc5.setStackID("5");
    fc2.setStackID("2");
    fc0.setStackID("0");

    // Insert out of order: 5, 2, 0
    OFCHECK(fgi.addPerFrame(5, fc5).good());
    OFCHECK(fgi.getNumberOfFrames() == 6); // vector resized to 6

    OFCHECK(fgi.addPerFrame(2, fc2).good());
    OFCHECK(fgi.addPerFrame(0, fc0).good());

    // Verify all three are retrievable
    FGBase* fg = fgi.get(5, DcmFGTypes::EFG_FRAMECONTENT);
    OFCHECK(fg != NULL);
    fg = fgi.get(2, DcmFGTypes::EFG_FRAMECONTENT);
    OFCHECK(fg != NULL);
    fg = fgi.get(0, DcmFGTypes::EFG_FRAMECONTENT);
    OFCHECK(fg != NULL);

    // Gaps (frames 1, 3, 4) return NULL
    OFCHECK(fgi.get(1, DcmFGTypes::EFG_FRAMECONTENT) == NULL);
    OFCHECK(fgi.get(3, DcmFGTypes::EFG_FRAMECONTENT) == NULL);
    OFCHECK(fgi.get(4, DcmFGTypes::EFG_FRAMECONTENT) == NULL);
}


OFTEST(dcmfg_fginterface_replace_perframe)
{
    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFCHECK_FAIL("no data dictionary loaded, check environment variable: " DCM_DICT_ENVIRONMENT_VARIABLE);
        return;
    }

    FGInterface fgi;
    FGFrameContent fc_old, fc_new;
    fc_old.setStackID("OLD");
    fc_new.setStackID("NEW");

    OFCHECK(fgi.addPerFrame(0, fc_old).good());

    // Verify old value
    OFString val;
    FGFrameContent* fg = OFstatic_cast(FGFrameContent*, fgi.get(0, DcmFGTypes::EFG_FRAMECONTENT));
    OFCHECK(fg != NULL);
    if (fg)
    {
        fg->getStackID(val);
        OFCHECK(val == "OLD");
    }

    // Replace with new value
    OFCHECK(fgi.addPerFrame(0, fc_new).good());
    fg = OFstatic_cast(FGFrameContent*, fgi.get(0, DcmFGTypes::EFG_FRAMECONTENT));
    OFCHECK(fg != NULL);
    if (fg)
    {
        fg->getStackID(val);
        OFCHECK(val == "NEW");
    }
}


OFTEST(dcmfg_fginterface_write_read_roundtrip)
{
    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFCHECK_FAIL("no data dictionary loaded, check environment variable: " DCM_DICT_ENVIRONMENT_VARIABLE);
        return;
    }

    FGInterface fgi;
    fgi.setCheckOnWrite(OFFalse); // skip consistency check for this unit test

    // Add per-frame FrameContent for 3 frames
    for (Uint32 i = 0; i < 3; i++)
    {
        FGFrameContent fc;
        char buf[16];
        OFStandard::snprintf(buf, sizeof(buf), "F%u", i);
        fc.setStackID(buf);
        fc.setTemporalPositionIndex(i + 1);
        OFCHECK(fgi.addPerFrame(i, fc).good());
    }

    // Add shared PixelMeasures
    FGPixelMeasures pm;
    pm.setPixelSpacing("1.0\\1.0");
    OFCHECK(fgi.addShared(pm).good());

    // Write to dataset
    DcmItem dataset;
    OFCHECK(fgi.write(dataset).good());

    // Read into a fresh FGInterface
    FGInterface fgi2;
    OFCHECK(fgi2.read(dataset).good());
    OFCHECK(fgi2.getNumberOfFrames() == 3);

    // Verify per-frame content survived roundtrip
    for (Uint32 i = 0; i < 3; i++)
    {
        OFBool isPerFrame = OFFalse;
        FGFrameContent* fc = OFstatic_cast(FGFrameContent*,
            fgi2.get(i, DcmFGTypes::EFG_FRAMECONTENT, isPerFrame));
        OFCHECK(fc != NULL);
        OFCHECK(isPerFrame == OFTrue);
        if (fc)
        {
            OFString val;
            fc->getStackID(val);
            char buf[16];
            OFStandard::snprintf(buf, sizeof(buf), "F%u", i);
            OFCHECK(val == buf);
            Uint32 tpi = 0;
            fc->getTemporalPositionIndex(tpi);
            OFCHECK(tpi == i + 1);
        }
    }

    // Verify shared group survived roundtrip
    OFBool isPerFrame = OFTrue;
    FGPixelMeasures* pm2 = OFstatic_cast(FGPixelMeasures*,
        fgi2.get(0, DcmFGTypes::EFG_PIXELMEASURES, isPerFrame));
    OFCHECK(pm2 != NULL);
    OFCHECK(isPerFrame == OFFalse);
}


OFTEST(dcmfg_fginterface_clear)
{
    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFCHECK_FAIL("no data dictionary loaded, check environment variable: " DCM_DICT_ENVIRONMENT_VARIABLE);
        return;
    }

    FGInterface fgi;
    FGFrameContent fc;
    fc.setStackID("T");
    OFCHECK(fgi.addPerFrame(0, fc).good());
    OFCHECK(fgi.addPerFrame(1, fc).good());

    FGPixelMeasures pm;
    pm.setPixelSpacing("1\\1");
    OFCHECK(fgi.addShared(pm).good());

    OFCHECK(fgi.getNumberOfFrames() == 2);

    fgi.clear();
    OFCHECK(fgi.getNumberOfFrames() == 0);
    OFCHECK(fgi.get(0, DcmFGTypes::EFG_FRAMECONTENT) == NULL);
    OFCHECK(fgi.get(0, DcmFGTypes::EFG_PIXELMEASURES) == NULL);
}


OFTEST(dcmfg_fginterface_convert_shared_to_perframe)
{
    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFCHECK_FAIL("no data dictionary loaded, check environment variable: " DCM_DICT_ENVIRONMENT_VARIABLE);
        return;
    }

    FGInterface fgi;

    // Create 3 frames with FrameContent
    for (Uint32 i = 0; i < 3; i++)
    {
        FGFrameContent fc;
        char buf[16];
        OFStandard::snprintf(buf, sizeof(buf), "%u", i);
        fc.setStackID(buf);
        OFCHECK(fgi.addPerFrame(i, fc).good());
    }

    // Add shared PixelMeasures
    FGPixelMeasures pm;
    pm.setPixelSpacing("0.5\\0.5");
    OFCHECK(fgi.addShared(pm).good());

    // Verify it's shared
    OFBool isPerFrame = OFTrue;
    OFCHECK(fgi.get(0, DcmFGTypes::EFG_PIXELMEASURES, isPerFrame) != NULL);
    OFCHECK(isPerFrame == OFFalse);

    // Now add a per-frame PixelMeasures with different content for frame 1;
    // this should trigger conversion from shared to per-frame for all frames
    FGPixelMeasures pm_different;
    pm_different.setPixelSpacing("1.0\\1.0");
    OFCHECK(fgi.addPerFrame(1, pm_different).good());

    // All frames should now have per-frame PixelMeasures
    for (Uint32 i = 0; i < 3; i++)
    {
        isPerFrame = OFFalse;
        FGBase* fg = fgi.get(i, DcmFGTypes::EFG_PIXELMEASURES, isPerFrame);
        OFCHECK(fg != NULL);
        OFCHECK(isPerFrame == OFTrue);
    }

    // Frame 1 should have the different value
    FGPixelMeasures* pm1 = OFstatic_cast(FGPixelMeasures*,
        fgi.get(1, DcmFGTypes::EFG_PIXELMEASURES));
    OFCHECK(pm1 != NULL);
    if (pm1)
    {
        Float64 val = 0.0;
        pm1->getPixelSpacing(val, 0);
        OFCHECK(val == 1.0);
    }
}


OFTEST(dcmfg_fginterface_write_1based_frames)
{
    // Regression test: when per-frame groups are added with 1-based frame
    // numbers (as dcmect's addFrame does), writing must produce exactly N
    // items in PerFrameFunctionalGroupsSequence, not N+1.
    // The bug was that writePerFrameFGSequential() used the vector index
    // (which includes a NULL slot at index 0) as the sequence item position,
    // causing findOrCreateSequenceItem() to create an extra empty item.

    if (!dcmDataDict.isDictionaryLoaded())
    {
        OFCHECK_FAIL("no data dictionary loaded, check environment variable: " DCM_DICT_ENVIRONMENT_VARIABLE);
        return;
    }

    FGInterface fgi;
    fgi.setCheckOnWrite(OFFalse);

    // Add per-frame groups using 1-based frame numbers (mimics dcmect behavior)
    const Uint32 numFrames = 3;
    for (Uint32 f = 1; f <= numFrames; f++)
    {
        FGFrameContent fc;
        char buf[16];
        OFStandard::snprintf(buf, sizeof(buf), "F%u", f);
        fc.setStackID(buf);
        OFCHECK(fgi.addPerFrame(f, fc).good());
    }

    // Write to dataset
    DcmItem dataset;
    OFCHECK(fgi.write(dataset).good());

    // Verify PerFrameFunctionalGroupsSequence has exactly numFrames items
    DcmSequenceOfItems* perFrameSeq = NULL;
    OFCHECK(dataset.findAndGetSequence(DCM_PerFrameFunctionalGroupsSequence, perFrameSeq).good());
    OFCHECK(perFrameSeq != NULL);
    if (perFrameSeq)
    {
        OFCHECK(perFrameSeq->card() == numFrames);
    }

    // Read back and verify content survived the roundtrip
    FGInterface fgi2;
    OFCHECK(fgi2.read(dataset).good());
    OFCHECK(fgi2.getNumberOfFrames() == numFrames);

    for (Uint32 f = 0; f < numFrames; f++)
    {
        FGFrameContent* fc = OFstatic_cast(FGFrameContent*,
            fgi2.get(f, DcmFGTypes::EFG_FRAMECONTENT));
        OFCHECK(fc != NULL);
        if (fc)
        {
            OFString val;
            fc->getStackID(val);
            char buf[16];
            OFStandard::snprintf(buf, sizeof(buf), "F%u", f + 1);
            OFCHECK(val == buf);
        }
    }
}
