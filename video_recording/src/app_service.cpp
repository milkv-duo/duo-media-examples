#include "app_service.h"

void AppService::initialize(Application &self) {
    // 初始化应用程序
    loadConfiguration();
    ServerApplication::initialize(self);
    INITLOG();
}

void AppService::uninitialize() {
    // 清理资源
    FLUSHLOG();

    ServerApplication::uninitialize();
}

void AppService::defineOptions(Poco::Util::OptionSet &options) {
    ServerApplication::defineOptions(options);

    options.addOption(
        Poco::Util::Option("help", "h", "Display help information.")
            .required(false)
            .repeatable(false)
            .callback(Poco::Util::OptionCallback<AppService>(
                this, &AppService::handleHelp)));

    options.addOption(
        Poco::Util::Option("format", "f", "Specify output format: flv mp4.")
            .required(false)
            .repeatable(false)
            .argument("<format>")
            .callback(Poco::Util::OptionCallback<AppService>(
                this, &AppService::handleFormat)));

    options.addOption(
        Poco::Util::Option("time", "t", "Specify recording duration seconds.")
            .required(false)
            .repeatable(false)
            .argument("<seconds>")
            .callback(Poco::Util::OptionCallback<AppService>(
                this, &AppService::handleTime)));

    options.addOption(Poco::Util::Option("outpath", "o", "Specify output path.")
                          .required(false)
                          .repeatable(false)
                          .argument("<outpath>")
                          .callback(Poco::Util::OptionCallback<AppService>(
                              this, &AppService::handleOutFile)));
}

void AppService::handleHelp(const std::string &name, const std::string &value) {
    displayHelp();
    stopOptionsProcessing();
    _helpRequested = true;
}

void AppService::handleFormat(const std::string &name,
                              const std::string &value) {
    if (value == "mp4") {
        _conf._format = "mp4";
    }
}
void AppService::handleTime(const std::string &name, const std::string &value) {
    try {
        _conf._record_time = std::stoi(value);
    } catch (const std::invalid_argument &e) {
        ERROR("Invalid argument: {0}", e.what());
    }
}
void AppService::handleOutFile(const std::string &name,
                               const std::string &value) {
    _conf._out_file = value;
}

void AppService::displayHelp() {
    Poco::Util::HelpFormatter helpFormatter(options());
    helpFormatter.setCommand("AppService");
    helpFormatter.setHeader("Application Service");
    helpFormatter.setUsage("[-h] [-fflv] [-t60] [-oout.flv]");
    helpFormatter.format(std::cout);
}

int AppService::main(const std::vector<std::string> &args) {

    if (_helpRequested) {
        return Application::EXIT_OK;
    }

    INFO("app service beggin ...");
    INFO("arg: {0}, {1}, {2}", _conf._format, _conf._record_time, _conf._out_file);
    VideoRecord record;
    record.initVideoEncoder();
    record.initFfmpeg(&_conf);

    Poco::Thread thread;
	thread.start(record);
	thread.join();
    return Application::EXIT_OK;
}