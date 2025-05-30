/*
 *
 *  Copyright (C) 2007-2025, OFFIS e.V.
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
 *  Module:  dcmdata
 *
 *  Author:  Michael Onken
 *
 *  Purpose: Implements utility for converting standard image formats to DICOM
 *
 */


#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/cmdlnarg.h"
#include "dcmtk/ofstd/ofconapp.h"
#include "dcmtk/dcmdata/dcuid.h"
#include "dcmtk/dcmdata/dcfilefo.h"
#include "dcmtk/dcmdata/dcdict.h"
#include "dcmtk/dcmdata/libi2d/i2d.h"
#include "dcmtk/dcmdata/libi2d/i2djpgs.h"
#include "dcmtk/dcmdata/libi2d/i2dbmps.h"
#include "dcmtk/dcmdata/libi2d/i2dplsc.h"
#include "dcmtk/dcmdata/libi2d/i2dplvlp.h"
#include "dcmtk/dcmdata/libi2d/i2dplnsc.h"
#include "dcmtk/dcmdata/libi2d/i2dplop.h"
#include "dcmtk/dcmdata/dcmxml/xml2dcm.h"

#define OFFIS_CONSOLE_APPLICATION "img2dcm"
static char rcsid[] = "$dcmtk: " OFFIS_CONSOLE_APPLICATION " v" OFFIS_DCMTK_VERSION " " OFFIS_DCMTK_RELEASEDATE " $";

#define SHORTCOL 4
#define LONGCOL 21

enum InputFormat
{
  InputFormatJPEG,
  InputFormatBMP
};

static OFLogger img2dcmLogger = OFLog::getLogger("dcmtk.apps." OFFIS_CONSOLE_APPLICATION);

static OFCondition evaluateFromFileOptions(
#ifdef WITH_LIBXML
  OFConsoleApplication& app,
#else
  OFConsoleApplication& /* app */,
#endif
  OFCommandLine& cmd,
  Image2Dcm& converter)
{
  OFCondition cond;
#ifdef WITH_LIBXML
  OFBool dataset_from = OFFalse;
#endif

  // Parse command line options dealing with DICOM file import
  if ( cmd.findOption("--dataset-from") )
  {
#ifdef WITH_LIBXML
    dataset_from = OFTrue;
#endif
    OFString tempStr;
    OFCommandLine::E_ValueStatus valStatus;
    valStatus = cmd.getValue(tempStr);
    if (valStatus != OFCommandLine::VS_Normal)
      return makeOFCondition(OFM_dcmdata, 18, OF_error, "Unable to read value of --dataset-from option");
    converter.setTemplateFile(tempStr);
    converter.setTemplateFileIsXML(OFFalse);
  }

#ifdef WITH_LIBXML
  if ( cmd.findOption("--dataset-from-xml") )
  {
    app.checkConflict("--dataset-from-xml", "--dataset-from", dataset_from);
    OFString tempStr;
    OFCommandLine::E_ValueStatus valStatus;
    valStatus = cmd.getValue(tempStr);
    if (valStatus != OFCommandLine::VS_Normal)
      return makeOFCondition(OFM_dcmdata, 18, OF_error, "Unable to read value of --dataset-from option");
    converter.setTemplateFile(tempStr);
    converter.setTemplateFileIsXML(OFTrue);
  }
#endif

  cmd.beginOptionBlock();
  if (cmd.findOption("--study-from"))
  {
    OFString tempStr;
    OFCommandLine::E_ValueStatus valStatus;
    valStatus = cmd.getValue(tempStr);
    if (valStatus != OFCommandLine::VS_Normal)
      return makeOFCondition(OFM_dcmdata, 18, OF_error, "Unable to read value of --study-from option");
    converter.setStudyFrom(tempStr);
  }

  if (cmd.findOption("--series-from"))
  {
    OFString tempStr;
    OFCommandLine::E_ValueStatus valStatus;
    valStatus = cmd.getValue(tempStr);
    if (valStatus != OFCommandLine::VS_Normal)
      return makeOFCondition(OFM_dcmdata, 18, OF_error, "Unable to read value of --series-from option");
    converter.setSeriesFrom(tempStr);
  }
  cmd.endOptionBlock();

  if (cmd.findOption("--instance-inc"))
    converter.setIncrementInstanceNumber(OFTrue);

  // Return success
  return EC_Normal;
}


static void addCmdLineOptions(OFCommandLine& cmd)
{
  cmd.addParam("imgfile-in",  "image input filename", OFCmdParam::PM_MultiMandatory);
  cmd.addParam("dcmfile-out", "DICOM output filename (\"-\" for stdout)");

  cmd.addGroup("general options:", LONGCOL, SHORTCOL + 2);
    cmd.addOption("--help",                  "-h",      "print this help text and exit", OFCommandLine::AF_Exclusive);
    cmd.addOption("--version",                          "print version information and exit", OFCommandLine::AF_Exclusive);
    OFLog::addOptions(cmd);

  cmd.addGroup("input options:", LONGCOL, SHORTCOL + 2);
    cmd.addSubGroup("general:");
      cmd.addOption("--input-format",        "-i",   1, "[i]nput file format: string",
                                                        "supported formats: JPEG (default), BMP");
      cmd.addOption("--dataset-from",        "-df",  1, "[f]ilename: string",
                                                        "use dataset from DICOM file f");
#ifdef WITH_LIBXML
      cmd.addOption("--dataset-from-xml",    "-dx",  1, "[f]ilename: string",
                                                        "use dataset from XML file f");
#endif
      cmd.addOption("--study-from",          "-stf", 1, "[f]ilename: string",
                                                        "read patient/study from DICOM file f");
      cmd.addOption("--series-from",         "-sef", 1, "[f]ilename: string",
                                                        "read patient/study/series from DICOM file f");
      cmd.addOption("--instance-inc",        "-ii",     "increase instance number read from DICOM file");
    cmd.addSubGroup("JPEG format:");
      cmd.addOption("--disable-progr",       "-dp",     "disable support for progressive JPEG");
      cmd.addOption("--disable-ext",         "-de",     "disable support for extended sequential JPEG");
      cmd.addOption("--insist-on-jfif",      "-jf",     "insist on JFIF header");
      cmd.addOption("--keep-appn",           "-ka",     "keep APPn sections (except JFIF)");
      cmd.addOption("--remove-com",          "-rc",     "remove COM segment");
#ifdef WITH_LIBXML
    cmd.addSubGroup("XML validation:");
      cmd.addOption("--validate-document",   "+Vd",     "validate XML document against DTD");
      cmd.addOption("--check-namespace",     "+Vn",     "check XML namespace in document root");
#endif

  cmd.addGroup("processing options:", LONGCOL, SHORTCOL + 2);
    cmd.addSubGroup("attribute checking:");
      cmd.addOption("--do-checks",                      "enable attribute validity checking (default)");
      cmd.addOption("--no-checks",                      "disable attribute validity checking");
      cmd.addOption("--insert-type2",        "+i2",     "insert missing type 2 attributes (default)\n(only with --do-checks)");
      cmd.addOption("--no-type2-insert",     "-i2",     "do not insert missing type 2 attributes \n(only with --do-checks)");
      cmd.addOption("--invent-type1",        "+i1",     "invent missing type 1 attributes (default)\n(only with --do-checks)");
      cmd.addOption("--no-type1-invent",     "-i1",     "do not invent missing type 1 attributes\n(only with --do-checks)");
    cmd.addSubGroup("character set conversion of study/series file:");
      cmd.addOption("--transliterate",       "-Ct",     "try to approximate characters that cannot be\nrepresented through similar looking characters");
      cmd.addOption("--discard-illegal",     "-Cd",     "discard characters that cannot be represented\nin destination character set");
    cmd.addSubGroup("other processing options:");
      cmd.addOption("--key",                 "-k",   1, "[k]ey: gggg,eeee=\"str\", path or dict name=\"str\"",
                                                        "add further attribute");

  cmd.addGroup("output options:");
    cmd.addSubGroup("target SOP class:");
      cmd.addOption("--sec-capture",         "-sc",     "write Secondary Capture SOP class (default)");
      cmd.addOption("--new-sc",              "-nsc",    "write new Secondary Capture SOP classes");
      cmd.addOption("--vl-photo",            "-vlp",    "write Visible Light Photographic SOP class");
      cmd.addOption("--oph-photo",           "-oph",    "write Ophthalmic Photography SOP classes");

    cmd.addSubGroup("output file format:");
      cmd.addOption("--write-file",          "+F",      "write file format (default)");
      cmd.addOption("--write-dataset",       "-F",      "write data set without file meta information");
    cmd.addSubGroup("group length encoding:");
      cmd.addOption("--group-length-recalc", "+g=",     "recalculate group lengths if present (default)");
      cmd.addOption("--group-length-create", "+g",      "always write with group length elements");
      cmd.addOption("--group-length-remove", "-g",      "always write without group length elements");
    cmd.addSubGroup("length encoding in sequences and items:");
      cmd.addOption("--length-explicit",     "+e",      "write with explicit lengths (default)");
      cmd.addOption("--length-undefined",    "-e",      "write with undefined lengths");
    cmd.addSubGroup("data set trailing padding (not with --write-dataset):");
      cmd.addOption("--padding-off",         "-p",      "no padding (implicit if --write-dataset)");
      cmd.addOption("--padding-create",      "+p",   2, "[f]ile-pad [i]tem-pad: integer",
                                                        "align file on multiple of f bytes\nand items on multiple of i bytes");
}


static I2DImgSource *createInputPlugin(InputFormat ifrm)
{
  switch (ifrm)
  {
    case InputFormatBMP:
      return new I2DBmpSource();
    case InputFormatJPEG:
    default:
      return new I2DJpegSource();
  }
}


static OFCondition startConversion(
  OFConsoleApplication& app,
  OFCommandLine& cmd)
{

  /* print resource identifier */
  OFLOG_DEBUG(img2dcmLogger, rcsid << OFendl);

  // Main class for controlling conversion
  Image2Dcm i2d;
  // Output plugin to use (i.e. SOP class to write)
  I2DOutputPlug *outPlug = NULL;
  // Input plugin to use (i.e. file format to read)
  I2DImgSource *inputPlug = NULL;
  // Group length encoding mode for output DICOM file
  E_GrpLenEncoding grpLengthEnc = EGL_recalcGL;
  // Item and Sequence encoding mode for output DICOM file
  E_EncodingType lengthEnc = EET_ExplicitLength;
  // Padding mode for output DICOM file
  E_PaddingEncoding padEnc = EPD_noChange;
  // File pad length for output DICOM file
  OFCmdUnsignedInt filepad = 0;
  // Item pad length for output DICOM file
  OFCmdUnsignedInt itempad = 0;
  // Write file format (with meta header)
  E_FileWriteMode writeMode = EWM_fileformat;
  // Override keys are applied at the very end of the conversion "pipeline"
  OFList<OFString> overrideKeys;
  // The transfer syntax proposed to be written by output plugin
  E_TransferSyntax writeXfer;
  // the input file format
  InputFormat inForm = InputFormatJPEG;

  // Parse rest of command line options
  OFLog::configureFromCommandLine(cmd, app);

  // create list of input files
  OFString paramValue;
  OFString outputFile;
  OFList<OFString> inputFiles;
  const int paramCount = cmd.getParamCount();
  for (int i = 1; i < paramCount; i++)
  {
    cmd.getParam(i, paramValue);
    inputFiles.push_back(paramValue);
  }

  // get output filename
  cmd.getParam(paramCount, outputFile);

  OFString tempStr;
  if (cmd.findOption("--input-format"))
  {
    app.checkValue(cmd.getValue(tempStr));
    if (tempStr == "JPEG")
    {
      inForm = InputFormatJPEG;
    }
    else if (tempStr == "BMP")
    {
      inForm = InputFormatBMP;
    }
    else
    {
      return makeOFCondition(OFM_dcmdata, 18, OF_error, "No plugin for selected input format available");
    }
  }

  inputPlug = createInputPlugin(inForm);
  OFLOG_INFO(img2dcmLogger, OFFIS_CONSOLE_APPLICATION ": Instantiated input plugin: " << inputPlug->inputFormat());

 // Find out which output plugin to use
  cmd.beginOptionBlock();
  if (cmd.findOption("--sec-capture"))
  {
    outPlug = new I2DOutputPlugSC();
  }
  if (cmd.findOption("--new-sc"))
  {
    outPlug = new I2DOutputPlugNewSC();
  }
  if (cmd.findOption("--vl-photo"))
  {
    outPlug = new I2DOutputPlugVLP();
  }
  if (cmd.findOption("--oph-photo"))
  {
    outPlug = new I2DOutputPlugOphthalmicPhotography();
  }

  cmd.endOptionBlock();
  if (!outPlug) // default is the old Secondary Capture object
    outPlug = new I2DOutputPlugSC();
  if (outPlug == NULL) return EC_MemoryExhausted;
  OFLOG_INFO(img2dcmLogger, OFFIS_CONSOLE_APPLICATION ": Instantiated output plugin: " << outPlug->ident());

  if (inputFiles.size() > 1)
  {
    // check if the output format supports multiframe
    if (! outPlug->supportsMultiframe())
    {
      OFLOG_ERROR(img2dcmLogger, outPlug->ident() << " does not support multiframe images");
      delete outPlug;
      return EC_SOPClassMismatch;
    }
  }

  cmd.beginOptionBlock();
  if (cmd.findOption("--write-file"))    writeMode = EWM_fileformat;
  if (cmd.findOption("--write-dataset")) writeMode = EWM_dataset;
  cmd.endOptionBlock();

  cmd.beginOptionBlock();
  if (cmd.findOption("--group-length-recalc")) grpLengthEnc = EGL_recalcGL;
  if (cmd.findOption("--group-length-create")) grpLengthEnc = EGL_withGL;
  if (cmd.findOption("--group-length-remove")) grpLengthEnc = EGL_withoutGL;
  cmd.endOptionBlock();

  cmd.beginOptionBlock();
  if (cmd.findOption("--length-explicit"))  lengthEnc = EET_ExplicitLength;
  if (cmd.findOption("--length-undefined")) lengthEnc = EET_UndefinedLength;
  cmd.endOptionBlock();

  cmd.beginOptionBlock();
  if (cmd.findOption("--padding-off"))
  {
    filepad = 0;
    itempad = 0;
  }
  else if (cmd.findOption("--padding-create"))
  {
    OFCmdUnsignedInt opt_filepad; OFCmdUnsignedInt opt_itempad;
    app.checkValue(cmd.getValueAndCheckMin(opt_filepad, 0));
    app.checkValue(cmd.getValueAndCheckMin(opt_itempad, 0));
    itempad = opt_itempad;
    filepad = opt_filepad;
  }
  cmd.endOptionBlock();

  // create override attribute dataset (copied from findscu code)
  if (cmd.findOption("--key", 0, OFCommandLine::FOM_FirstFromLeft))
  {
    const char *ovKey = NULL;
    do {
      app.checkValue(cmd.getValue(ovKey));
      overrideKeys.push_back(ovKey);
    } while (cmd.findOption("--key", 0, OFCommandLine::FOM_NextFromLeft));
  }
  i2d.setOverrideKeys(overrideKeys);

  size_t conversionFlags = 0;
  if (cmd.findOption("--transliterate"))
    conversionFlags |= DCMTypes::CF_transliterate;
  if (cmd.findOption("--discard-illegal"))
    conversionFlags |= DCMTypes::CF_discardIllegal;
  i2d.setConversionFlags(conversionFlags);

  // evaluate validity checking options
  OFBool insertType2 = OFTrue;
  OFBool inventType1 = OFTrue;
  OFBool doChecks = OFTrue;
  cmd.beginOptionBlock();
  if (cmd.findOption("--no-checks"))
    doChecks = OFFalse;
  if (cmd.findOption("--do-checks"))
    doChecks = OFTrue;
  cmd.endOptionBlock();

  cmd.beginOptionBlock();
  if (cmd.findOption("--insert-type2"))
    insertType2 = OFTrue;
  if (cmd.findOption("--no-type2-insert"))
    insertType2 = OFFalse;
  cmd.endOptionBlock();

  cmd.beginOptionBlock();
  if (cmd.findOption("--invent-type1"))
    inventType1 = OFTrue;
  if (cmd.findOption("--no-type1-invent"))
    inventType1 = OFFalse;
  cmd.endOptionBlock();
  i2d.setValidityChecking(doChecks, insertType2, inventType1);
  outPlug->setValidityChecking(doChecks, insertType2, inventType1);

#ifdef WITH_LIBXML
  // evaluate XML parsing options
  if (cmd.findOption("--validate-document"))
  {
    app.checkDependence("--validate-document", "--dataset-from-xml", cmd.findOption("--dataset-from-xml"));
    i2d.setXMLvalidation(OFTrue);
  } else i2d.setXMLvalidation(OFFalse);

  if (cmd.findOption("--check-namespace"))
  {
    app.checkDependence("--check-namespace", "--dataset-from-xml", cmd.findOption("--dataset-from-xml"));
    i2d.setXMLnamespaceCheck(OFTrue);
  }
  else i2d.setXMLnamespaceCheck(OFFalse);
#endif

  // evaluate --xxx-from options and transfer syntax options
  OFCondition cond;
  cond = evaluateFromFileOptions(app, cmd, i2d);
  if (cond.bad())
  {
    delete outPlug; outPlug = NULL;
    delete inputPlug; inputPlug = NULL;
    return cond;
  }

  if (inputPlug->inputFormat() == "JPEG")
  {
    I2DJpegSource *jpgSource = OFstatic_cast(I2DJpegSource*, inputPlug);
    if (!jpgSource)
    {
       delete outPlug; outPlug = NULL;
       delete inputPlug; inputPlug = NULL;
       return EC_MemoryExhausted;
    }
    if ( cmd.findOption("--disable-progr") )
      jpgSource->setProgrSupport(OFFalse);
    if ( cmd.findOption("--disable-ext") )
      jpgSource->setExtSeqSupport(OFFalse);
    if ( cmd.findOption("--insist-on-jfif") )
      jpgSource->setInsistOnJFIF(OFTrue);
    if ( cmd.findOption("--keep-appn") )
      jpgSource->setKeepAPPn(OFTrue);
    if ( cmd.findOption("--remove-com") )
      jpgSource->setKeepCOM(OFFalse);
  }

  /* make sure data dictionary is loaded */
  if (!dcmDataDict.isDictionaryLoaded())
  {
    OFLOG_WARN(img2dcmLogger, "no data dictionary loaded, check environment variable: "
      << DCM_DICT_ENVIRONMENT_VARIABLE);
  }

  DcmDataset *resultObject = NULL;
  OFLOG_INFO(img2dcmLogger, OFFIS_CONSOLE_APPLICATION ": Starting image conversion");

  OFListIterator(OFString) if_iter = inputFiles.begin();
  OFListIterator(OFString) if_last = inputFiles.end();

  inputPlug->setImageFile(*if_iter++); // we are guaranteed to have at least one input file
  cond = i2d.convertFirstFrame(inputPlug, outPlug, inputFiles.size(), resultObject, writeXfer);
  size_t frameNum = 1;

  // iterate over all extra input filenames
  while (cond.good() && (if_iter != if_last))
  {
    // create a new input format plugin for each file to be processed
    delete inputPlug;
    inputPlug = createInputPlugin(inForm);
    inputPlug->setImageFile(*if_iter++);
    cond = i2d.convertNextFrame(inputPlug, ++frameNum);
  }

  // adjust byte order of pixel data to local OW byte order
  if (cond.good()) cond = i2d.adjustByteOrder(inputFiles.size());

  // update offset table if image type is encapsulated
  if (cond.good()) cond = i2d.updateOffsetTable();

  // update attributes related to lossy compression
  if (cond.good()) cond = i2d.updateLossyCompressionInfo(inputPlug, inputFiles.size(), resultObject);

  // Save
  if (cond.good())
  {
    OFLOG_INFO(img2dcmLogger, OFFIS_CONSOLE_APPLICATION ": Saving output DICOM to file " << outputFile);
    DcmFileFormat dcmff(resultObject);
    cond = dcmff.saveFile(outputFile, writeXfer, lengthEnc,  grpLengthEnc, padEnc, OFstatic_cast(Uint32, filepad), OFstatic_cast(Uint32, itempad), writeMode);
  }

  // Cleanup and return
  delete outPlug; outPlug = NULL;
  delete inputPlug; inputPlug = NULL;
  delete resultObject; resultObject = NULL;

  return cond;
}


int main(int argc, char *argv[])
{
  // Parse command line and exclusive options
  OFConsoleApplication app(OFFIS_CONSOLE_APPLICATION, "Convert standard image formats into DICOM format", rcsid);
  OFCommandLine cmd;

  cmd.setOptionColumns(LONGCOL, SHORTCOL);
  cmd.setParamColumn(LONGCOL + SHORTCOL + 4);
  addCmdLineOptions(cmd);

  prepareCmdLineArgs(argc, argv, OFFIS_CONSOLE_APPLICATION);
  if (app.parseCommandLine(cmd, argc, argv))
  {
    /* check exclusive options first */
    if (cmd.hasExclusiveOption())
    {
      if (cmd.findOption("--version"))
      {
        app.printHeader(OFTrue /*print host identifier*/);

#ifdef WITH_LIBXML
        COUT << OFendl << "External libraries used:" << OFendl;
        COUT << "- LIBXML, Version " << LIBXML_DOTTED_VERSION << OFendl;
#if defined(LIBXML_ICONV_ENABLED) && defined(LIBXML_ZLIB_ENABLED)
       COUT << "  with built-in LIBICONV and ZLIB support" << OFendl;
#elif defined(LIBXML_ICONV_ENABLED)
        COUT << "  with built-in LIBICONV support" << OFendl;
#elif defined(LIBXML_ZLIB_ENABLED)
       COUT << "  with built-in ZLIB support" << OFendl;
#endif
#endif
        exit(0);
      }
    }
  }

#ifdef WITH_LIBXML
  DcmXMLParseHelper::initLibrary(); // initialize XML parser
#endif

  int result = 0;
  OFCondition cond = startConversion(app, cmd);
  if (cond.bad())
  {
    OFLOG_FATAL(img2dcmLogger, "Error converting file: " << cond.text());
    result = 1;
  }

#ifdef WITH_LIBXML
    DcmXMLParseHelper::cleanupLibrary(); // clean up XML library before quitting
#endif

  return result;
}
