#include <arpa/inet.h>
#include <ev.h>
#include <net/network_layer.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <utils/utils.h>
#include <valt/valt.h>

#include <CLI/CLI.hpp>
#include <iostream>

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

int main(int argc, char* argv[]) {
    try {
        auto logger = spdlog::stdout_color_mt("console");
        logger->flush_on(spdlog::level::info);
        spdlog::set_default_logger(logger);
        spdlog::set_level(spdlog::level::info);
    } catch (const spdlog::spdlog_ex& ex) {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
    }

    ValtConfig cfg;

    auto app = std::make_unique<CLI::App>("Valt Database");

    app->add_option("--port", cfg.port, "Raw TCP port");
    app->add_option("--tls_port", cfg.tls_port, "TLS secured port");
    app->add_option("--cert-file", cfg.cert_path, "File path to TLS certificate");
    app->add_option("--key-file", cfg.private_key, "File path to TLS private key");
    app->add_option("--max_connections", cfg.max_connections, "Max connections");
    app->add_option("--max_pending_connections", cfg.max_pending_connections, "Max pending connections");
    app->add_option("--max-memory", cfg.max_memory, "Max memory");
    app->add_flag("--no-auth", cfg.no_auth, "Disable authentication");
    app->set_version_flag("--version", "0.1.0");

    try {
        app->parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app->exit(e);
    }

    int server_fd = utils::init_server(cfg.port, cfg.max_pending_connections);
    create_accept_watcher(server_fd, nullptr);

    int tls_server_fd = -1;
    if (cfg.cert_path.size() > 0 && cfg.private_key.size() > 0) {
        SSL_CTX* ssl_ctx = utils::create_ssl_context(cfg.cert_path, cfg.private_key);
        if (!ssl_ctx) {
            throw std::runtime_error("Error setting up SSL context");
        }

        tls_server_fd = utils::init_server(cfg.tls_port, cfg.max_pending_connections);
        create_accept_watcher(tls_server_fd, ssl_ctx);
    }

    if (cfg.no_auth) {
        Valt& valt = Valt::getInstance(&cfg);
        valt.disable_authentication();
    } else {
        char* master_key = std::getenv("VALT_MASTER_KEY");
        if (master_key == nullptr) {
            spdlog::info(
                "Authentication enabled but no master key found, exiting. To disable "
                "authenticaiton, pass --no-auth flag.");
            close(server_fd);
            if (tls_server_fd) close(tls_server_fd);
            return 1;
        }
    }

    ev_run(EV_DEFAULT, 0);

    close(server_fd);
    if (tls_server_fd) close(tls_server_fd);
    return 0;
}
