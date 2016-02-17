/**
 * Copyright (c) 2015-2016, The Kovri I2P Router Project
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Config.h"

#include <string>

#include <crypto/Rand.h>

namespace i2p {
namespace util {
namespace config {

namespace bpo = boost::program_options;

std::string kovri_config, tunnels_config;
bpo::variables_map var_map;

bool ParseArgs(
    int argc,
    char* argv[]) {
  // Random generated port if none is supplied via CLI or config
  // See: i2p.i2p/router/java/src/net/i2p/router/transport/udp/UDPEndpoint.java
  // TODO(anonimal): move this elsewhere (outside of ParseArgs()) when possible
  size_t port = i2p::crypto::RandInRange<size_t>(9111, 30777);
  // Map options values from CLI and config
  bpo::options_description help("Help options");
  help.add_options()
    ("help,h",

      "General usage:\n\n"

      "$ ./kovri\n\n"

      "A random port will be generated with each use.\n"
      "You can specify a port with the --port option\n"
      "or you can set one in the config file instead.\n\n"

      "Reload configuration file:\n\n"

      "$ pkill -HUP kovri\n\n"

      "Read kovri.conf and tunnels.cfg for more options\n\n")

    ("help-with,w", bpo::value<std::string>(),

      "Help with a specific option.\n\n"

      "Available options:\n"
      "==================\n\n"

      "all     | basic | system\n"
      "network | proxy | i2pcs\n"
      "config\n\n"

      "Examples\n"
      "========\n\n"

      "List all options:\n\n"
      "$ ./kovri -w all\n\n"

      "List only basic options:\n\n"
      "$ ./kovri -w basic");

  bpo::options_description basic("\nBasic");
  basic.add_options()
    ("host", bpo::value<std::string>()->default_value("127.0.0.1"),
     "The external IP (deprecated).\n"
     "Default: external interface")
    ("port,p", bpo::value<int>()->default_value(port),
     "Port to listen on.\n"
     "Default: random (then saved to router.info)");

  bpo::options_description system("\nSystem");
  system.add_options()
    ("log,l", bpo::value<bool>()->default_value(0),
     "Enable or disable logging to file\n"
     "1 = enabled, 0 = disabled\n")

    ("daemon,d", bpo::value<bool>()->default_value(0),
     "Enable or disable daemon mode\n"
     "1 = enabled, 0 = disabled\n")

    // TODO(anonimal): clarify what --service 'really' does
    // See DaemonWin32.cpp
    ("service,s", bpo::value<bool>()->default_value(0),
     "1 if using system folders, e.g.,\n"
     "(/var/run/kovri.pid, /var/log/kovri.log, /var/lib/kovri)\n");

  bpo::options_description network("\nNetwork");
  network.add_options()
    ("v6,6", bpo::value<bool>()->default_value(0),
     "1 to enable IPv6\n"
     "1 = enabled, 0 = disabled\n")

    ("floodfill,f", bpo::value<bool>()->default_value(0),
     "1 to enable router router as floodfill\n"
     "1 = enabled, 0 = disabled\n")

    ("bandwidth,b", bpo::value<std::string>()->default_value("L"),
     "L if bandwidth is limited to 32Kbs/sec, O if not\n"
     "Always O if floodfill, otherwise L by default\n");

  // TODO(anonimal): do we want proxy/i2pcs options in CLI
  // if we can redirect future multiple running instances
  // of kovri to use unique config files per instance instead?
  bpo::options_description proxy("\nProxy");
  proxy.add_options()
    ("httpproxyport", bpo::value<int>()->default_value(4446),
     "The HTTP Proxy port to listen on\n")

    ("httpproxyaddress", bpo::value<std::string>()->default_value("127.0.0.1"),
     "The HTTP Proxy address to listen on\n")

    ("socksproxyport", bpo::value<int>()->default_value(4447),
     "The SOCKS Proxy port to listen on\n")

    ("socksproxyaddress", bpo::value<std::string>()->default_value("127.0.0.1"),
     "The SOCKS Proxy address to listen on\n")

    ("proxykeys,k", bpo::value<std::string>()->default_value(""),
     "Optional keys file for proxy's local destination\n");

  bpo::options_description i2pcs("\nI2P Control Service");
  i2pcs.add_options()
    ("i2pcontrolport", bpo::value<int>()->default_value(0),
     "Port of I2P control service (usually 7650)\n"
     "I2PControl is disabled if not specified\n")

    ("i2pcontroladdress", bpo::value<std::string>()->default_value("127.0.0.1"),
     "Address of I2P control service\n"
     "Default: 127.0.0.1 (only used if I2PControl is enabled)\n")

    ("i2pcontrolpassword", bpo::value<std::string>()->default_value("itoopie"),
     "I2P control service password\n");

  bpo::options_description config("\nConfiguration");
  config.add_options()
    ("config,c", bpo::value<std::string>(&kovri_config)->default_value(
        i2p::util::filesystem::GetFullPath("kovri.conf")),
     "Options specified on the command line take"
     "precedence over those in the config file.\n")

    ("tunnelscfg,t", bpo::value<std::string>(&tunnels_config)->default_value(
        i2p::util::filesystem::GetFullPath("tunnels.cfg")),
     "Tunnels Config file\n");
  // Default visible option
  bpo::options_description kovri(
      ":----------------------------------------------------:\n"
      "|              The Kovri I2P Router Project          |\n"
      "|                    version " KOVRI_VERSION "                   |\n"
      ":----------------------------------------------------");
  kovri.add(help);
  // Available config file options
  bpo::options_description config_options;
  config_options
    .add(basic)
    .add(system)
    .add(network)
    .add(proxy)
    .add(i2pcs)
    .add(config);
  // Available cli options
  bpo::options_description cli_options;
  cli_options
    .add(help)
    .add(basic)
    .add(system)
    .add(network)
    .add(proxy)
    .add(i2pcs)
    .add(config);
  // Map and store cli options
  bpo::store(bpo::parse_command_line(argc, argv, cli_options), var_map);
  bpo::notify(var_map);
  // Parse config after mapping cli
  ParseConfigFile(kovri_config, config_options, var_map);
  /*
   * Display --help and --help-with
   */
  if (var_map.count("help")) {
    std::cout << kovri << std::endl;
    return false;
  }
  if (var_map.count("help-with")) {
    const std::string& s = var_map["help-with"].as<std::string>();
    if (s == "all") {
      std::cout << config_options;  // We don't need .add(help)
    } else if (s == "basic") {
      std::cout << basic;
    } else if (s == "system") {
      std::cout << system;
    } else if (s == "network") {
      std::cout << network;
    } else if (s == "proxy") {
      std::cout << proxy;
    } else if (s == "i2pcs") {
      std::cout << i2pcs;
    } else if (s == "config") {
      std::cout << config;
    } else {
      std::cout << "Unknown option '" << s << "'"
      << "\nTry using --help" << std::endl;
    }
    return false;
  }
  return true;
}

void ParseConfigFile(
    std::string& config,
    bpo::options_description& config_options,
    bpo::variables_map& var_map) {
  std::ifstream ifs(config.c_str());
  if (!ifs) {
    std::cout << "Could not open " << config << "!\n";
  } else {
    bpo::store(
        bpo::parse_config_file(
            ifs,
            config_options),
        var_map);
    bpo::notify(var_map);
  }
}

}  // namespace config
}  // namespace util
}  // namespace i2p
