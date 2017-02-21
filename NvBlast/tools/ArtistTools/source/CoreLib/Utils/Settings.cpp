/*******************************************************************************
* NVIDIA Corporation 
* Software License Agreement - OpenAutomate SDK 
* 
* IMPORTANT - READ BEFORE COPYING, INSTALLING OR USING
* Do not use or load the OpenAutomate SDK and any associated materials
* provided by NVIDIA on NVIDIA’s website (collectively, the "Software")
* until You have carefully read the following terms and conditions. By
* loading or using the Software, You agree to fully comply with the terms
* and conditions of this Software License Agreement ("Agreement") by and
* between NVIDIA Corporation, a Delaware corporation with its principal
* place of business at 2701 San Tomas Expressway, Santa Clara, California
* 95050 U.S.A. ("NVIDIA"), and You. If You do not wish to so agree, do not
* install or use the Software. 
* 
* For the purposes of this Agreement: 
* 
* "Licensee," "You" and/or "Your" shall mean, collectively and
* individually, Original Equipment Manufacturers, Independent Hardware
* Vendors, Independent Software Vendors, and End-Users of the Software
* pursuant to the terms and conditions of this Agreement.   
* 
* "Derivative Works" shall mean derivatives of the Software created by You
* or a third party on Your behalf, which term shall include:  (a) for
* copyrightable or copyrighted material, any translation, abridgement,
* revision or other form in which an existing work may be recast,
* transformed or adapted; (b) for work protected by topography or mask
* right, any translation, abridgement, revision or other form in which an
* existing work may be recast, transformed or adapted; (c) for patentable
* or patented material, any Improvement; and (d) for material protected by
* trade secret, any new material derived from or employing such existing
* trade secret.
* 
* "Excluded License" is any license that requires as a condition of use,
* modification and/or distribution of software subject to the Excluded
* License, that such software or other software distributed and/or
* combined with such software be (i) disclosed or distributed in source
* code form, (ii) licensed for the purpose of making derivative works, or
* (iii) redistributable at no charge.
* 
* SECTION 1 - GRANT OF LICENSE.
* NVIDIA agrees to provide the Software and any associated materials
* pursuant to the terms and conditions of this Agreement.  Subject to the
* terms of this Agreement, NVIDIA grants to You a nonexclusive,
* transferable, worldwide, revocable, limited, royalty-free, fully paid-up
* license under NVIDIA’s copyrights to 
* 
* (a) install, deploy, use, have used execute, reproduce, display,
* perform, run, modify the source code of the Software, or to prepare and
* have prepared Derivative Works thereof the Software for Your own
* internal development, testing and maintenance purposes to incorporate
* the Software or Derivative Works thereof, in part or whole, into Your
* software applications; 
* 
* (b)	 to transfer, distribute and sublicense the Software (in its
* unmodified form as delivered to You by NVIDIA pursuant to this
* Agreement) in any medium or technology for Your sublicensees to
* incorporate the Software or Derivative Works thereof, in part or whole,
* into their respective software applications; and
* 
* (c) to transfer, distribute and sublicense Derivative Works (in object
* code only) of the Software (i)_as incorporated in Your application
* software in any medium or technology; and (ii) certified as OpenAutomate
* Compatible Software.
* 
* You may exercise your license rights pursuant to Subsection 1(b) and (c)
* above pursuant to the terms and conditions of any form of end-user
* software license agreement of Your choice, including but not limited to
* an Excluded License.
* 
* In the event NVIDIA certifies Your application software, incorporating
* the Derivative Works (in object code only) of the Software, as
* OpenAutomate compatible ("OpenAutomate Compatible Software"), NVIDIA
* grants You a nonexclusive, worldwide, revocable, paid-up license to use
* the name and trademark to "OpenAutomate Compatible" solely for the
* purposes of identifying and/or marketing Your application software as
* OpenAutomate Compatible Software; provided that Licensee fully complies
* with the following:	
* 
* (x) Licensee agrees that it is strictly prohibited from using the name
* and trademark of "OpenAutomate Compatible" if Your application software
* is not OpenAutomate Compatible Software;
* 
* (y) if NVIDIA objects to Your improper use of the "OpenAutomate
* Compatible" name and trademark, You will take all reasonable steps
* necessary to resolve NVIDIA’s objections. NVIDIA may reasonably monitor
* the quality of Your application software bearing the "OpenAutomate
* Compatible" name or trademark pursuant to this Agreement; and
* 
* (z) any goodwill attached to NVIDIA’s trademarks, service marks, or
* trade names belong to NVIDIA and this Agreement does not grant You any
* right to use them.
* 
* If You are not the final manufacturer or vendor of a computer system or
* software program incorporating the Software, or if Your Contractors (as
* defined below), affiliates or subsidiaries need to exercise any, some or
* all of the license grant described above herein to the Software on Your
* behalf, then You may transfer a copy of the Software, (and related
* end-user documentation) to such recipient for use in accordance with the
* terms of this Agreement, provided such recipient agrees to be fully
* bound by the terms hereof. Except as expressly permitted in this
* Agreement, Unless otherwise authorized in the Agreement, You shall not
* otherwise assign, sublicense, lease, or in any other way transfer or
* disclose Software to any third party. Unless otherwise authorized in the
* Agreement, You shall not reverse- compile, disassemble,
* reverse-engineer, or in any manner attempt to derive the source code of
* the Software from the object code portions of the Software. 
* 
* Except as expressly stated in this Agreement, no license or right is
* granted to You directly or by implication, inducement, estoppel or
* otherwise. NVIDIA shall have the right to inspect or have an independent
* auditor inspect Your relevant records to verify Your compliance with the
* terms and conditions of this Agreement. 
* 
* SECTION 2 - CONFIDENTIALITY.
* If applicable, any exchange of Confidential Information (as defined in
* the NDA) shall be made pursuant to the terms and conditions of a
* separately signed Non-Disclosure Agreement ("NDA") by and between NVIDIA
* and You. For the sake of clarity, You agree that the Software is
* Confidential Information of NVIDIA.
* 
* If You wish to have a third party consultant or subcontractor
* ("Contractor") perform work on Your behalf which involves access to or
* use of Software, You shall obtain a written confidentiality agreement
* from the Contractor which contains terms and obligations with respect to
* access to or use of Software no less restrictive than those set forth in
* this Agreement and excluding any distribution or sublicense rights, and
* use for any other purpose than permitted in this Agreement. Otherwise,
* You shall not disclose the terms or existence of this Agreement or use
* NVIDIA's name in any publications, advertisements, or other
* announcements without NVIDIA's prior written consent.  Unless otherwise
* provided in this Agreement, You do not have any rights to use any NVIDIA
* trademarks or logos.
* 
* SECTION 3 - OWNERSHIP OF SOFTWARE AND INTELLECTUAL PROPERTY RIGHTS.
* All rights, title and interest to all copies of the Software remain with
* NVIDIA, subsidiaries, licensors, or its suppliers. The Software is
* copyrighted and protected by the laws of the United States and other
* countries, and international treaty provisions. You may not remove any
* copyright notices from the Software. NVIDIA may make changes to the
* Software, or to items referenced therein, at any time and without
* notice, but is not obligated to support or update the Software. Except
* as otherwise expressly provided, NVIDIA grants no express or implied
* right under any NVIDIA patents, copyrights, trademarks, or other
* intellectual property rights. 
* 
* All rights, title and interest in the Derivative Works of the Software
* remain with You subject to the underlying license from NVIDIA to the
* Software.  In Your sole discretion, You may grant NVIDIA, upon NVIDIA’s
* request for such a license described herein, an irrevocable, perpetual,
* nonexclusive, worldwide, royalty-free paid-up license to make, have
* made, use, have used, sell, license, distribute, sublicense or otherwise
* transfer Derivative Works created by You that add functionality or
* improvement to the Software.  
* 
* You has no obligation to give NVIDIA any suggestions, comments or other
* feedback ("Feedback") relating to the Software.  However, NVIDIA may use
* and include any Feedback that You voluntarily provide to improve the
* Software or other related NVIDIA technologies.  Accordingly, if You
* provide Feedback, You agree NVIDIA and its licensees may freely use,
* reproduce, license, distribute, and otherwise commercialize the Feedback
* in the Software or other related technologies without the payment of any
* royalties or fees.  
* 
* You may transfer the Software only if the recipient agrees to be fully
* bound by these terms and conditions of this Agreement. 
* 
* SECTION 4 - NO WARRANTIES. 
* THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY EXPRESS OR IMPLIED WARRANTY
* OF ANY KIND, INCLUDING WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT,
* OR FITNESS FOR A PARTICULAR PURPOSE. NVIDIA does not warrant or assume
* responsibility for the accuracy or completeness of any information,
* text, graphics, links or other items contained within the Software.
* NVIDIA does not represent that errors or other defects will be
* identified or corrected.
* 
* SECTION 5 - LIMITATION OF LIABILITY.
* EXCEPT WITH RESPECT TO THE MISUSE OF THE OTHER PARTY’S INTELLECTUAL
* PROPERTY OR DISCLOSURE OF THE OTHER PARTY’S CONFIDENTIAL INFORMATION IN
* BREACH OF THIS AGREEMENT, IN NO EVENT SHALL NVIDIA, SUBSIDIARIES,
* LICENSORS, OR ITS SUPPLIERS BE LIABLE FOR ANY DAMAGES WHATSOEVER
* (INCLUDING, WITHOUT LIMITATION,  INDIRECT, LOST PROFITS, CONSEQUENTIAL,
* BUSINESS INTERRUPTION OR LOST INFORMATION) ARISING OUT OF THE USE OF OR
* INABILITY TO USE THE SOFTWARE, EVEN IF NVIDIA HAS BEEN ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGES. SOME JURISDICTIONS PROHIBIT EXCLUSION OR
* LIMITATION OF LIABILITY FOR IMPLIED WARRANTIES OR CONSEQUENTIAL OR
* INCIDENTAL DAMAGES, SO THE ABOVE LIMITATION MAY NOT APPLY TO YOU. YOU
* MAY ALSO HAVE OTHER LEGAL RIGHTS THAT VARY FROM JURISDICTION TO
* JURISDICTION.  NOTWITHSTANDING THE FOREGOING, NVIDIA’S AGGREGATE
* LIABILITY ARISING OUT OF THIS AGREEMENT SHALL NOT EXCEED ONE HUNDRED
* UNITED STATES DOLLARS (USD$100).
* 
* SECTION 6 - TERM.
* This Agreement and the licenses granted hereunder shall be effective as
* of the date You download the applicable Software ("Effective Date") and
* continue for a period of one (1) year ("Initial Term") respectively,
* unless terminated earlier in accordance with the "Termination" provision
* of this Agreement.  Unless either party notifies the other party of its
* intent to terminate this Agreement at least three (3) months prior to
* the end of the Initial Term or the applicable renewal period, this
* Agreement will be automatically renewed for one (1) year renewal periods
* thereafter, unless terminated in accordance with the "Termination"
* provision of this Agreement.  
* 
* SECTION 7 - TERMINATION.
* NVIDIA may terminate this Agreement at any time if You violate its
* terms. Upon termination, You will immediately destroy the Software or
* return all copies of the Software to NVIDIA, and certify to NVIDIA in
* writing that such actions have been completed.  Upon termination or
* expiration of this Agreement the license grants to Licensee shall
* terminate, except that sublicenses rightfully granted by Licensee under
* this Agreement in connection with Section 1(b) and (c) of this Agreement
* provided by Licensee prior to the termination or expiration of this
* Agreement shall survive in accordance with their respective form of
* license terms and conditions.
* 
* SECTION 8 - MISCELLANEOUS.
* 
* SECTION 8.1 - SURVIVAL.
* Those provisions in this Agreement, which by their nature need to
* survive the termination or expiration of this Agreement, shall survive
* termination or expiration of the Agreement, including but not limited to
* Sections 2, 3, 4, 5, 7, and 8.
* 
* SECTION 8.2 - APPLICABLE LAWS.
* Claims arising under this Agreement shall be governed by the laws of
* Delaware, excluding its principles of conflict of laws and the United
* Nations Convention on Contracts for the Sale of Goods. The state and/or
* federal courts residing in Santa Clara County, California shall have
* exclusive jurisdiction over any dispute or claim arising out of this
* Agreement. You may not export the Software in violation of applicable
* export laws and regulations. 
* 
* SECTION 8.3 - AMENDMENT.
* The Agreement shall not be modified except by a written agreement that
* names this Agreement and any provision to be modified, is dated
* subsequent to the Effective Date, and is signed by duly authorized
* representatives of both parties.
* 
* SECTION 8.4 - NO WAIVER.
* No failure or delay on the part of either party in the exercise of any
* right, power or remedy under this Agreement or under law, or to insist
* upon or enforce performance by the other party of any of the provisions
* of this Agreement or under law, shall operate as a waiver thereof, nor
* shall any single or partial exercise of any right, power or remedy
* preclude other or further exercise thereof, or the exercise of any other
* right, power or remedy; rather the provision, right, or remedy shall be
* and remain in full force and effect.
* 
* SECTION 8.5 - NO ASSIGNMENT. 
* This Agreement and Licensee’s rights and obligations herein, may not be
* assigned, subcontracted, delegated, or otherwise transferred by Licensee
* without NVIDIA’s prior written consent, and any attempted assignment,
* subcontract, delegation, or transfer in violation of the foregoing will
* be null and void.  The terms of this Agreement shall be binding upon
* Licensee’s assignees.
* 
* SECTION 8.6 - GOVERNMENT RESTRICTED RIGHTS. 
* The parties acknowledge that the Software is subject to U.S. export
* control laws and regulations.   The parties agree to comply with all
* applicable international and national laws that apply to the Software,
* including the U.S. Export Administration Regulations, as well as
* end-user, end-use and destination restrictions issued by U.S. and other
* governments.
* 
* The Software has been developed entirely at private expense and is
* commercial computer software provided with RESTRICTED RIGHTS. Use,
* duplication or disclosure of the Software by the U.S. Government or a
* U.S. Government subcontractor is subject to the restrictions set forth
* in the Agreement under which the Software was obtained pursuant to DFARS
* 227.7202-3(a) or as set forth in subparagraphs (c)(1) and (2) of the
* Commercial Computer Software - Restricted Rights clause at FAR
* 52.227-19, as applicable. Contractor/manufacturer is NVIDIA, 2701 San
* Tomas Expressway, Santa Clara, CA 95050. Use of the Software by the
* Government constitutes acknowledgment of NVIDIA's proprietary rights
* therein. 
* 
* SECTION 8.7 - INDEPENDENT CONTRACTORS.
* Licensee’s relationship to NVIDIA is that of an independent contractor,
* and neither party is an agent or partner of the other.  Licensee will
* not have, and will not represent to any third party that it has, any
* authority to act on behalf of NVIDIA.
* 
* SECTION 8.8 - SEVERABILITY.
* If for any reason a court of competent jurisdiction finds any provision
* of this Agreement, or portion thereof, to be unenforceable, that
* provision of the Agreement will be enforced to the maximum extent
* permissible so as to affect the intent of the parties, and the remainder
* of this Agreement will continue in full force and effect. This Agreement
* has been negotiated by the parties and their respective counsel and will
* be interpreted fairly in accordance with its terms and without any
* strict construction in favor of or against either party.
* 
* SECTION 8.9 - ENTIRE AGREEMENT.  
* This Agreement and NDA constitute the entire agreement between the
* parties with respect to the subject matter contemplated herein, and
* merges all prior and contemporaneous communications.
* 
******************************************************************************/


#include "settings.h"
#include "Nv.h"

char OptionsFileName[] = "FurViewerOptions.txt";
//std::vector<void *> AllocBlocks;

using namespace std;

void Error(const char *fmt, ...);
static int HexCharToInt(unsigned char c);
bool GetExePath(char* exe_path);
static int Hex2BytesToInt(const unsigned char *buf);
static oaFloat StrToFloat(const char *buf);
static void WriteFloat(FILE *fp, oaFloat val);
static void StripNewLine(char *str);
void *Alloc(size_t n);
char *StrDup(const char *str);

bool GetExePath(char* exe_path)
{
#if WIN32
	char delim = '\\';
#else
	char delim = '/';
#endif

	DWORD dwRet;
	dwRet = GetModuleFileNameA(NV_NULL, exe_path, MAX_PATH);
	if ( dwRet == 0 )
	{
		exe_path[0] = '\0';
		return false;
	}

	int i;
	for ( i = dwRet; i > 0; i-- )
	{
		if ( exe_path[i-1] == delim )
			break;
	}
	exe_path[i] = '\0';

	return true;
}

void Error(const char *fmt, ...)
{
	va_list AP;
	va_start(AP, fmt);

	char msg[1024];
	//fprintf(msg, "ERROR: ");
	//vfprintf(msg, fmt, AP);
	//fprintf(msg, "\n");
	sprintf(msg, fmt, AP);
	OutputDebugStringA(msg);
	assert(0);
//	exit(-1);
}

static int HexCharToInt(unsigned char c)
{
	if(c >= '0' && c <= '9')
		return((int)(c - '0'));

	c = tolower(c);
	if(c >= 'a' && c <= 'f')
		return((int)(c - 'a' + 10));

	return(0);
}

static int Hex2BytesToInt(const unsigned char *buf)
{
	return(HexCharToInt(buf[0]) << 4 | HexCharToInt(buf[1]));
}

static oaFloat StrToFloat(const char *buf)
{
	size_t Len = strlen(buf);
	if(Len != sizeof(oaFloat) * 2)
		return(0.0);

	oaFloat Ret;
	unsigned char *Buf = (unsigned char *)&Ret;
	for(int i=0; i < sizeof(oaFloat); ++i)
	{
		Buf[i] = Hex2BytesToInt((const unsigned char *)buf);
		buf += 2;
	}

	return(Ret);
}

static void WriteFloat(FILE *fp, oaFloat val)
{
	unsigned char *Buf = (unsigned char *)&val;
	for(int i=0;  i < sizeof(oaFloat); ++i)
		fprintf(fp, "%02x", Buf[i]);
}

static void StripNewLine(char *str)
{
	size_t StrLen = strlen(str);
	if(StrLen == 0)
		return;

	for(size_t i=StrLen-1; i >= 0; --i)
		if(str[i] == '\n')
		{
			str[i] = 0;
			break;
		}
}

void *Alloc(size_t n)
{
	void *Ret = malloc(n);
	assert(n != 0);

	//AllocBlocks.push_back(Ret);
	return(Ret);
}

char *StrDup(const char *str)
{
	assert(str != NV_NULL);
	char *Ret = strdup(str);

	//AllocBlocks.push_back(Ret);
	return(Ret);
}

AppSettings::AppSettings()
{
	int NumOptions = 0;
}

AppSettings::~AppSettings()
{
	Cleanup();
}

void AppSettings::InitOptions(void)
{
	{
		oaNamedOption *Option;

		// Resolution (enum)
		Option = &Options[NumOptions++];
		oaInitOption(Option);
		Option->Name = "User/Resolution";
		Option->DataType = OA_TYPE_ENUM;
		Option->Value.Enum = "640x480";

		Options[NumOptions] = *Option;
		Option = &Options[NumOptions++];
		Option->Value.Enum = "1024x768";

		Options[NumOptions] = *Option;
		Option = &Options[NumOptions++];
		Option->Value.Enum = "1200x768";

		Options[NumOptions] = *Option;
		Option = &Options[NumOptions++];
		Option->Value.Enum = "1600x1200";

		// AA (enum)
		Option = &Options[NumOptions++];
		oaInitOption(Option);

		Option->Name = "User/AA";
		Option->DataType = OA_TYPE_ENUM;
		Option->Value.Enum = "Off";

		Options[NumOptions] = *Option;
		Option = &Options[NumOptions++];
		Option->Value.Enum = "2X";

		Options[NumOptions] = *Option;
		Option = &Options[NumOptions++];
		Option->Value.Enum = "4X";

		Options[NumOptions] = *Option;
		Option = &Options[NumOptions++];
		Option->Value.Enum = "8X";

		// device id (int)
		Option = &Options[NumOptions++];
		oaInitOption(Option);
		Option->Name = "User/Device";
		Option->DataType = OA_TYPE_INT;

		Option->Value.Int = -1;  // when -1 to choose a good GPU

		//Backdoor (enum)
		Option = &Options[NumOptions++];
		oaInitOption(Option);
		Option->Name = "User/Backdoor";
		Option->DataType = OA_TYPE_STRING;
		Option->Value.String = "";

		// HideUI (bool)
		Option = &Options[NumOptions++];
		oaInitOption(Option);
		Option->Name = "User/HideUI";
		Option->DataType = OA_TYPE_BOOL;

		Option->Value.Bool = OA_FALSE;

		// Perf mode (bool)
		Option = &Options[NumOptions++];
		oaInitOption(Option);
		Option->Name = "User/PerfMode";
		Option->DataType = OA_TYPE_BOOL;

		Option->Value.Bool = OA_FALSE;

		// Play (bool)
		Option = &Options[NumOptions++];
		oaInitOption(Option);
		Option->Name = "User/Play";
		Option->DataType = OA_TYPE_BOOL;

		Option->Value.Bool = OA_FALSE;

		// ProjectPath (string)
		Option = &Options[NumOptions++];
		oaInitOption(Option);
		Option->Name = "User/ProjectPath";
		Option->DataType = OA_TYPE_STRING;
		Option->Value.String = "";

		// FurAssetPath (string)
		Option = &Options[NumOptions++];
		oaInitOption(Option);
		Option->Name = "User/FurAssetPath";
		Option->DataType = OA_TYPE_STRING;
		Option->Value.String = "";

		// FurDemoPlaylist (string)
		Option = &Options[NumOptions++];
		oaInitOption(Option);
		Option->Name = "User/FurDemoPlaylist";
		Option->DataType = OA_TYPE_STRING;
		Option->Value.String = "";

		//Option = &Options[NumOptions++];
		//oaInitOption(Option);
		//Option->Name = "r2_slight_fade";
		//Option->DataType = OA_TYPE_FLOAT;
		//Option->MinValue.Float = 0.0;
		//Option->MaxValue.Float = 100.0;
		//Option->NumSteps = 200;
	}

	//****************************************************************************
	//*** Init OptionValues
	//****************************************************************************

	// Initialize default options
	for(int i=0; i < NumOptions; ++i)
	{
		string Name(Options[i].Name);
		OptionValueMap[Name].Name = Options[i].Name;
		OptionValueMap[Name].Type = Options[i].DataType;
		OptionValueMap[Name].Value = Options[i].Value;
	}

	InitDefaultOptions();
	///////////////////////////////////////////////////////
	// Load any persistent options if they've been previously set
	ReadOptionsFile();
}

void AppSettings::InitDefaultOptions(void)
{
	oaValue Value;

	Value.Enum = "1200x768";
	SetOptionValue("User/Resolution", OA_TYPE_ENUM, &Value);

	Value.Enum = "8X";
	SetOptionValue("User/AA", OA_TYPE_ENUM, &Value);
}

void AppSettings::SetOptionValue(const char *name, 
	oaOptionDataType type,
	const oaValue *value)
{
	string Name(name);
	assert(OptionValueMap.find(Name) != OptionValueMap.end());
	assert(OptionValueMap[Name].Type == type);

	switch(type)
	{
	case OA_TYPE_STRING:
		OptionValueMap[Name].Value.String = StrDup(value->String);
		break;

	case OA_TYPE_ENUM:
		OptionValueMap[Name].Value.Enum = StrDup(value->Enum);
		break;

	default:
		OptionValueMap[Name].Value = *value;
	}
}

void AppSettings::ReadOptionsFile(void)
{ 
	char optionFile[MAX_PATH] = "";
	if(!GetExePath(optionFile))
	{
		fprintf(stderr, "Cannot get exe path\r\n");
	}
	strcat(optionFile, OptionsFileName);


	FILE *FP = fopen(optionFile, "rb");
	if(!FP)
		return;

	fprintf(stderr, "FurViewer: Reading options file \"%s\".\n", 
		optionFile);
	fflush(stderr);

	char Line[1024] = "";
	int LineNum = 1;
	while (fgets(Line, sizeof(Line), FP) != NV_NULL)
	{
		StripNewLine(Line);
		if(Line[0] == 0)
			continue;

		char *Name = strtok(Line, "\t");
		char *Value = strtok(NV_NULL, "");

		if(!Name || !Value)
		{
			//Error("Invalid format in options file \"%s\" on line %d\n", 
			//OptionsFileName, 
			//LineNum);
			continue;
		}

		map<string, OptionValue>::const_iterator OptVal = 
			OptionValueMap.find(string(Name));

		if(OptVal == OptionValueMap.end())
			Error("Unknown option \"%s\" defined in options file \"%s\" on line %d.",
			Name, 
			OptionsFileName, 
			LineNum);

		SetOptionValue(Name, OptVal->second.Type, Value);

		LineNum++;
	}

	fclose(FP);
} 

void AppSettings::Cleanup(void)
{
	//vector<void *>::iterator Iter = AllocBlocks.begin();
	//for(; Iter != AllocBlocks.end(); Iter++)
	//	free(*Iter);

	//AllocBlocks.clear();
}


void AppSettings::WriteOptionsFile(FILE *fp)
{
	map<string, OptionValue>::const_iterator Iter = OptionValueMap.begin();
	for(; Iter != OptionValueMap.end(); ++Iter)
	{
		fprintf(fp, "%s\t", Iter->second.Name);
		switch(Iter->second.Type)
		{
		case OA_TYPE_INT:
			fprintf(fp, "%d", Iter->second.Value.Int);
			break;

		case OA_TYPE_BOOL:
			fprintf(fp, "%d", (int)Iter->second.Value.Bool);
			break;

		case OA_TYPE_FLOAT:
			WriteFloat(fp, Iter->second.Value.Float);
			break;

		case OA_TYPE_STRING:
			fprintf(fp, "%s", Iter->second.Value.String);
			break;

		case OA_TYPE_ENUM:
			fprintf(fp, "%s", Iter->second.Value.Enum);
			break;

		}

		fprintf(fp, "\n");
	}
}

void AppSettings::WriteOptionsFile(void)
{
	char optionFile[MAX_PATH] = "";
	if(!GetExePath(optionFile))
	{
		fprintf(stderr, "Cannot get exe path\r\n");
	}
	strcat(optionFile, OptionsFileName);

	fprintf(stderr, "FurViewer: Writing options file \"%s\".\n", 
		optionFile);
	fflush(stderr);

	FILE *FP = fopen(optionFile, "wb");
	if(!FP)
		Error("Couldn't open \"%s\" for write.\n", optionFile);

	WriteOptionsFile(FP);

	fclose(FP);
}

void AppSettings::SetOptionValue(const char *name, 
	oaOptionDataType type,
	const char *value)
{
	assert(name != NV_NULL);
	assert(type != OA_TYPE_INVALID);
	assert(value != NV_NULL);

	oaValue Value;
	switch(type)
	{
	case OA_TYPE_INT:
		Value.Int = atoi(value);
		break;

	case OA_TYPE_FLOAT:
		Value.Float = StrToFloat(value);
		break;

	case OA_TYPE_BOOL:
		Value.Bool = atoi(value) ? OA_TRUE : OA_FALSE;
		break;

	case OA_TYPE_STRING:
		Value.String = (oaString)value;
		break;

	case OA_TYPE_ENUM:
		Value.Enum = (oaString)value;
		break; 
	}

	SetOptionValue(name, type, &Value);
}



