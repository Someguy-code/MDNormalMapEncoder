import CommandLineArguments;
import MDNormalMapEncoder;
import std;

int main(int argc, char* argv[])
{
    try
    {
        const CommandLineArguments oCommandLineArguments{ static_cast<unsigned int>(argc - 1), &argv[1] };
        if (argc == 1)
        {
            std::cout << "Normal maps encoder for the Sega Megadrive/Genesis. Usage example:\n";
            std::cout << "MDNormalMapEncoder -im \"torus_mask.bmp\" -in \"torus_normal.bmp\" -ia \"torus_albedo.bmp\" -ot \"torus_out.bmp\" -om \"torus_out.mat\" -nhs 6 -nvs 3 -amc 2\n";
            std::cout << "Possible parameter:\n";
            oCommandLineArguments.PrintArgumentDescriptions();
            std::cout << "NOTE: All colors in quotes, 3 8-bit RGB separated by commas.\n";
            return 0;
        }

        oCommandLineArguments.m_oEncoderArguments.Validate();

        MDNormalMapEncoder::Encode(oCommandLineArguments.m_oEncoderArguments);

        std::cout << "Finished!\n";
        return 0;
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