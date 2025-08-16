#include <iostream>
#include <filesystem>

#include "DDA Extractor/dda_manager.h"

void ShowUsage()
{
	std::cout << "" << std::endl;

	std::cout << "Usage: DDA_Extractor.exe <input_path> <output_path>" << std::endl;
	std::cout << "Example: DDA_Extractor.exe \"C:\\path\\to\\dda_folder\" \"C:\\path\\to\\output\"" << std::endl;
	std::cout << "The input_path should contains the content of the game (TRACKS, FLASH folders, INGAME.UBR, SPRITES.UBR files...) " << std::endl;
	std::cout << "Do not include the \\ at the end of the input_path and output_path." << std::endl;
}

void LaunchExtraction(const std::string& inputPath, const std::string& outputPath);

int main(int argc, char* argv[])
{
	if (argc > 3)
	{
		ShowUsage();
		return 1;
	}

	std::string inputPath;
	std::string outputPath;

	for (size_t i = 0; i < argc; i++)
	{
		std::string argument = argv[i];
		if (argument.size() >= 4)
		{
			if (argument.substr(argument.size() - 4) == ".exe")
			{
				continue;
			}
		}

		if(i == 1)
		{
			inputPath = argument;
		}
		else if (i == 2)
		{
			outputPath = argument;
		}
		else
		{
			std::cout << "Unknown argument: " << argument << "\n";
			return 1;
		}
	}

	// Check if input and output paths are valid
	bool stopProgram = false;
	if (inputPath.empty())
	{
		std::cout << "[ERROR] Input path cannot be empty." << std::endl;
		stopProgram = true;
	}

	if (outputPath.empty())
	{
		std::cout << "[ERROR] Output path cannot be empty." << std::endl;
		stopProgram = true;
	}

	if (!stopProgram && !std::filesystem::exists(inputPath))
	{
		std::cout << "[ERROR] Input path does not exists." << std::endl;
		stopProgram = true;
	}

	if(stopProgram)
	{
		ShowUsage();
		return 1;
	}

	// Fix input and output paths
	if(inputPath[inputPath.size() -1] - 1 != '\\' || inputPath[inputPath.size() - 1] - 1 != '/')
	{
		inputPath += "\\";
	}

	if (outputPath[outputPath.size() - 1] - 1 != '\\' || outputPath[outputPath.size() - 1] - 1 != '/')
	{
		outputPath += "\\";
	}

	LaunchExtraction(inputPath, outputPath);

    std::cout << "Done!\n";
}

void LaunchExtraction(const std::string& inputPath, const std::string& outputPath)
{
	//TODO: Multithread extraction, one thread per file
	DDAManager ddaManager = DDAManager(inputPath);

	ddaManager.ExtractData(DDAGameFile::AIRPORT, outputPath);
	ddaManager.ExtractData(DDAGameFile::BMOVIE, outputPath);
	ddaManager.ExtractData(DDAGameFile::BRON_2ND, outputPath);
	ddaManager.ExtractData(DDAGameFile::BRONX, outputPath);
	ddaManager.ExtractData(DDAGameFile::CHIN_2ND, outputPath);
	ddaManager.ExtractData(DDAGameFile::CHINATWN, outputPath);
	ddaManager.ExtractData(DDAGameFile::CONSTR, outputPath);
	ddaManager.ExtractData(DDAGameFile::DAM, outputPath);
	ddaManager.ExtractData(DDAGameFile::ENGINE, outputPath);
	ddaManager.ExtractData(DDAGameFile::GLADIATO, outputPath);
	ddaManager.ExtractData(DDAGameFile::GODS, outputPath);
	ddaManager.ExtractData(DDAGameFile::JUSTICE, outputPath);
	ddaManager.ExtractData(DDAGameFile::REFINERY, outputPath);
	ddaManager.ExtractData(DDAGameFile::SHIPYARD, outputPath);
	ddaManager.ExtractData(DDAGameFile::STEELWRK, outputPath);
	ddaManager.ExtractData(DDAGameFile::SUBWAY, outputPath);
	ddaManager.ExtractData(DDAGameFile::VEGA_2ND, outputPath);
	ddaManager.ExtractData(DDAGameFile::VEGAS, outputPath);
	ddaManager.ExtractData(DDAGameFile::WINBOWL, outputPath);

	ddaManager.ExtractData(DDAGameFile::SPRITES, outputPath);
	ddaManager.ExtractData(DDAGameFile::INGAME, outputPath);

	ddaManager.ExtractData(DDAGameFile::DD4FRONT, outputPath);
	ddaManager.ExtractData(DDAGameFile::DD4GAME, outputPath);
	ddaManager.ExtractData(DDAGameFile::DD4START, outputPath);
}