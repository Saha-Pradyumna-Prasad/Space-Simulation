#include "app/application.h"
#include "app/cli.h"
#include "selftest/selftest.h"

#include <memory>

int main(int argc, char **argv) {
    spacesim::CliOptions options = spacesim::parse_cli(argc, argv);
    if (options.help) {
        spacesim::print_cli_usage(argc > 0 ? argv[0] : "space_simulator");
        return 0;
    }
    if (options.selftest) {
        return spacesim::run_selftest();
    }

    std::unique_ptr<spacesim::Application> app(new spacesim::Application(options));
    return app->run(argc, argv);
}
