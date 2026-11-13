#include <cassert>

import CommandLineArguments;
import Encoder;
import EncoderArgumentsBuilder;
import ResultWriter;
import std;
import UIEngine;

int main(int argc, char* argv[])
{
    try
    {
        const CommandLineArguments oCommandLineArguments{ static_cast<unsigned int>(argc - 1), &argv[1] };
        switch (oCommandLineArguments.GetExecutionMode())
        {
            case CommandLineArguments::ExecutionMode::Interactive:
            {
                UIEngine oEngine;
                return oEngine.Execute();
            }
            case CommandLineArguments::ExecutionMode::ShowHelp:
            {
                std::cout << "Normal maps encoder for the Sega Megadrive/Genesis. Usage example:\n";
                std::cout << "MDNormalMapEncoder -im \"torus_mask.bmp\" -in \"torus_normal.bmp\" -ia \"torus_albedo.bmp\" -ot \"torus_out.bmp\" -om \"torus_out.mat\" -nhs 6 -nvs 3 -amc 2\n";
                std::cout << "Do not specify any arguments for interactive mode.\n";
                std::cout << "Supported arguments:\n";
                oCommandLineArguments.PrintArgumentDescriptions();
                std::cout << "NOTE: All colors in quotes, 3 8-bit RGB separated by commas.\n";
                return 0;
            }
            case CommandLineArguments::ExecutionMode::CommandLine:
            {
                const std::chrono::time_point<std::chrono::high_resolution_clock> oTimeStart = std::chrono::high_resolution_clock::now();
                ResultWriter::WriteResult(Encoder::Encode(EncoderArgumentsBuilder::GetEncoderArguments(oCommandLineArguments)), oCommandLineArguments);
                const std::chrono::time_point<std::chrono::high_resolution_clock> oTimeEnd = std::chrono::high_resolution_clock::now();
                std::chrono::duration<float> oDuration = oTimeEnd - oTimeStart;
                std::cout << "Finished (" << oDuration.count() << " s)!\n";
                return 0;
            }
			default:
                throw std::runtime_error("Unhandled execution mode");
				return -1;
        }
    }
    catch (const std::runtime_error& oException)
    {
        std::cerr << oException.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Unhandled exception\n";
    }

    return -1;
}