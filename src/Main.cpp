#include <RenderBase.hpp>
#include <exception>
#include <spdlog/spdlog.h>

namespace renderer {
    extern "C" int main(int argc, char* argv[]) {
        try {
            RenderBase app;
            app.run();
        } catch (const std::exception& e) {
            spdlog::error("{}", e.what());
        }
        return EXIT_SUCCESS;
    }
}
