#include <RenderBase.hpp>
#include <exception>
#include <spdlog/spdlog.h>

namespace renderer {
    extern "C" int main(int argc, char* argv[]) {
        RenderBase::Config config;
        config.window_width = 1280;
        config.window_height = 720;

        if (argc < 2) {
            spdlog::warn("Using default scene. Usage: {} <model_path>", argv[0]);
            config.model = std::nullopt;
        }
        else if (argc == 2) {
            const char* model_path = argv[1];
            config.model = model_path;
        }
        else {
            spdlog::warn("Too many arguments. Usage: {} <model_path>", argv[0]);
            const char* model_path = argv[1];
            config.model = model_path;
        }


        try {
            RenderBase app(config);
            app.run();
        } catch (const std::exception& e) {
            spdlog::error("{}", e.what());
        }
        return EXIT_SUCCESS;
    }
}
