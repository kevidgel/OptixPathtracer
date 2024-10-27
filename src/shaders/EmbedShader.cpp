/**
* @file EmbedShader.cpp
* @brief Implementation of shader embedding functionality.
*/

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

namespace EmbedShader {
    void embed_shader(const std::string& name, const std::string& src, const std::string& out) {
        // Open source and output files
        std::ifstream source(src, std::ios::in | std::ios::binary);
        if (!source.is_open()) {
            throw std::runtime_error("Could not open shader source file: " + src);
        }
        std::ofstream output(out, std::ios::out | std::ios::binary);
        if (!output.is_open()) {
            throw std::runtime_error("Could not open output file: " + out);
        }

        // Load shader source into a buffer
        std::vector<char> buffer(
            (std::istreambuf_iterator<char> (source)),
            std::istreambuf_iterator<char> ()
        );

        buffer.push_back('\0'); // Null-terminate the buffer

        // Convert name into all caps with periods replaced by underscores
        std::string upper_name = name;
        for (auto& c : upper_name) {
            if (c == '.') c = '_';
            c = toupper(c);
        }

        // Convert name into all lower with periods replaced by underscores
        std::string lower_name = name;
        for (auto& c : lower_name) {
            if (c == '.') c = '_';
            c = tolower(c);
        }

        // Write to output file
        output << "/**\n* @file " << out << "\n* @brief This file is autogenerated by EmbedShader.\n*/\n\n";
        output << "#pragma once\n\n";
        output << "#ifndef " << upper_name << "_HPP\n";
        output << "#define " << upper_name << "_HPP\n\n";
        output << "#include <cstddef>\n";
        output << "#include <cstdint>\n\n";
        output << "namespace ShaderSources {\n";
        output << "    static constexpr uint32_t " << lower_name << "_size = " << buffer.size() << ";\n";
        output << "    static constexpr uint8_t " << lower_name << "_source[] = {";
        for (size_t i = 0; i < buffer.size(); ++i) {
            if (i % 12 == 0) output << "\n        ";
            output
            << "0x"
            << std::uppercase
            << std::setw(2)
            << std::setfill('0')
            << std::hex
            << (static_cast<unsigned int>(buffer[i]) & 0xFF)
            << ", ";
        }
        output << "\n    };\n}\n";
        output << "#endif // " << upper_name << "_HPP\n";
    }

    extern "C" int main(const int argc, char* argv[]) {
        if (argc != 3) {
            std::cerr << "Usage: " << argv[0] << " <source_shader> <output_file>" << std::endl;
            std::cerr << "Example: " << argv[0] << " shader.ptx shader.ptx.hpp" << std::endl;
            return EXIT_FAILURE;
        }

        const std::string src = argv[1];
        const std::string name = src.substr(src.find_last_of("/\\") + 1);
        const std::string out = argv[2];

        try {
            embed_shader(name, src, out);
        }
        catch (const std::exception& e) {
            std::cerr << argv[0] << " error: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }
}