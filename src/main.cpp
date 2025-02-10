#include "Application.h"

#include <iostream>
#include <fstream>
#include <filesystem>

void RedirectOutputToFile()
{
    if (!std::filesystem::exists("./logs"))
    {
        std::filesystem::create_directory("./logs");
    }

    // Open a file stream to redirect output
    static std::ofstream out("./logs/MK_output.log");
    if (out.is_open())
    {
        std::cout.rdbuf(out.rdbuf()); // Redirect std::cout to the file
    }
    std::cout << " -- Output Log -- " << std::endl;
    static std::ofstream err("./logs/MK_error.log");
    if (err.is_open())
    {
        std::cerr.rdbuf(err.rdbuf()); // Redirect std::cerr to the file
    }
    std::cerr << " -- Error Log -- " << std::endl;
}

int MainImpl(int argc, char **argv)
{
    mk::Application app;
    return app.Run(argc, argv);
}

// int main(int argc, char **argv)
// {
//     return MainImpl(argc, argv);
// }

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
#ifndef _DEBUG
    RedirectOutputToFile();
#endif
    return MainImpl(__argc, __argv);
}