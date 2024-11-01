#include <RenderBase.hpp>
#include <exception>
#include <spdlog/spdlog.h>
#include <argparse/argparse.hpp>

namespace renderer {
    extern "C" int main(int argc, char* argv[]) {
        argparse::ArgumentParser program("OptixPathtracer");
        program.add_argument("--model-path")
            .help("Path to the model file")
            .default_value("");

        program.add_argument("--env-map")
            .help("Path to the environment map file")
            .default_value("");


        try {
            program.parse_args(argc, argv);
            RenderBase::Config config;
            config.window_width = 1280;
            config.window_height = 720;
            config.env_map = program.get<std::string>("--env-map");
            if (config.env_map.value().empty()) {
                spdlog::warn("No environment map provided, using default.");
                config.env_map = std::nullopt; // Assuming this function exists
            }
            config.model = program.get<std::string>("--model-path");
            if (config.model.value().empty()) {
                spdlog::warn("No model path provided, using default.");
                config.model = std::nullopt; // Assuming this function exists
            }

            RenderBase app(config);
            app.run();
        } catch (const std::exception& e) {
            spdlog::error("{}", e.what());
        }
        return EXIT_SUCCESS;
    }
}
