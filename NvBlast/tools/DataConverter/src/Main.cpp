#include "NvBlastExtDataConverter.h"
#include <string>
#include <iostream>
#include "tclap/CmdLine.h"
#include <stdint.h>
#include <iterator>
#include <fstream>
#include <functional>

#define IDE_DEBUG_MODE 0

int customMain(std::vector<std::string>& args)
{
	try 
	{
		// setup cmd line
		TCLAP::CmdLine cmd("Blast SDK: Data Converter", ' ', "0.1");

		TCLAP::ValueArg<int32_t> outversionArg("v", "outversion", "Output binary block version. Pass -1 or ignore this parameter to convert to latest version.", false, -1, "outversion");
		cmd.add(outversionArg);
		TCLAP::ValueArg<std::string> outfileArg("o", "outfile", "Output binary file.", true, "", "outfile");
		cmd.add(outfileArg);
		TCLAP::ValueArg<std::string> infileArg("i", "infile", "Input binary file.", true, "", "infile");
		cmd.add(infileArg);

		// parse cmd input
		cmd.parse(args);

		// get cmd parse results
		std::string infile = infileArg.getValue();
		std::string outfile = outfileArg.getValue();
		int32_t outBlockVersion = outversionArg.getValue();

		// read input file
		std::ifstream infileStream(infile.c_str(), std::ios::binary);
		if (!infileStream.is_open())
		{
			std::cerr << "FAIL: Can't open input file: " << infile << std::endl;
			return 0;
		}
		std::vector<char> inBlock((std::istreambuf_iterator<char>(infileStream)), std::istreambuf_iterator<char>());
		infileStream.close();

		// convert
		std::vector<char> outBlock;
		if (Nv::Blast::convertDataBlock(outBlock, inBlock, outBlockVersion >= 0 ? (uint32_t*)&outBlockVersion : nullptr))
		{
			std::ofstream outfileStream(outfile, std::ios::binary | std::ios::out);
			if (!outfileStream.is_open())
			{
				std::cerr << "FAIL: Can't open output file: " << outfile << std::endl;
				return 0;
			}
			outfileStream.write((char*)&outBlock[0], outBlock.size());
			outfileStream.close();

			std::cout << "Conversion success, result written in file: " << outfile << std::endl;
		}
		else
		{
			std::cout << "Conversion failed." << std::endl;
		}
	}
	catch (TCLAP::ArgException &e)  // catch any exceptions
	{
		std::cout << "error: " << e.error() << " for arg " << e.argId() << std::endl;
	}

	return 0;
}

int main(int argc, const char* const* argv)
{
	std::vector<std::string> args;
#if IDE_DEBUG_MODE
	NV_UNUSED(argc);
	NV_UNUSED(argv);

	args.push_back("");
	args.push_back("-i wall.blast");
	args.push_back("-o wall_new.blast");
	args.push_back("-v 5");
#else 
	for (int i = 0; i < argc; i++)
		args.push_back(argv[i]);
#endif
	return customMain(args);
}
