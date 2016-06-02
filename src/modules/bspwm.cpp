#include <thread>
#include <vector>
#include <sstream>

#include "config.hpp"
#include "lemonbuddy.hpp"
#include "bar.hpp"
#include "modules/bspwm.hpp"
#include "utils/config.hpp"
#include "utils/io.hpp"
#include "utils/memory.hpp"
#include "utils/string.hpp"

using namespace modules;
using namespace Bspwm;

#define DEFAULT_WS_ICON "workspace_icon-default"
#define DEFAULT_WS_LABEL "%icon%  %name%"

BspwmModule::BspwmModule(const std::string& name_, const std::string& monitor) : EventModule(name_)
{
  this->monitor = monitor;

  this->formatter->add(DEFAULT_FORMAT, TAG_LABEL_STATE, { TAG_LABEL_STATE }, { TAG_LABEL_MODE });

  if (this->formatter->has(TAG_LABEL_STATE)) {
    this->state_labels.insert(std::make_pair(WORKSPACE_ACTIVE, drawtypes::get_optional_config_label(name(), "label-active", DEFAULT_WS_LABEL)));
    this->state_labels.insert(std::make_pair(WORKSPACE_OCCUPIED, drawtypes::get_optional_config_label(name(), "label-occupied", DEFAULT_WS_LABEL)));
    this->state_labels.insert(std::make_pair(WORKSPACE_URGENT, drawtypes::get_optional_config_label(name(), "label-urgent", DEFAULT_WS_LABEL)));
    this->state_labels.insert(std::make_pair(WORKSPACE_EMPTY, drawtypes::get_optional_config_label(name(), "label-empty", DEFAULT_WS_LABEL)));
    this->state_labels.insert(std::make_pair(WORKSPACE_DIMMED, drawtypes::get_optional_config_label(name(), "label-dimmed")));
  }

  if (this->formatter->has(TAG_LABEL_MODE)) {
    this->mode_labels.insert(std::make_pair(MODE_LAYOUT_MONOCLE, drawtypes::get_optional_config_label(name(), "label-monocle")));
    this->mode_labels.insert(std::make_pair(MODE_LAYOUT_TILED, drawtypes::get_optional_config_label(name(), "label-tiled")));
    this->mode_labels.insert(std::make_pair(MODE_STATE_FULLSCREEN, drawtypes::get_optional_config_label(name(), "label-fullscreen")));
    this->mode_labels.insert(std::make_pair(MODE_STATE_FLOATING, drawtypes::get_optional_config_label(name(), "label-floating")));
    this->mode_labels.insert(std::make_pair(MODE_NODE_LOCKED, drawtypes::get_optional_config_label(name(), "label-locked")));
    this->mode_labels.insert(std::make_pair(MODE_NODE_STICKY, drawtypes::get_optional_config_label(name(), "label-sticky")));
    this->mode_labels.insert(std::make_pair(MODE_NODE_PRIVATE, drawtypes::get_optional_config_label(name(), "label-private")));
  }

  this->icons = std::make_unique<drawtypes::IconMap>();
  this->icons->add(DEFAULT_WS_ICON, std::make_unique<drawtypes::Icon>(config::get<std::string>(name(), DEFAULT_WS_ICON, "")));

  for (auto workspace : config::get_list<std::string>(name(), "workspace_icon", {})) {
    auto vec = string::split(workspace, ';');
    if (vec.size() == 2) this->icons->add(vec[0], std::make_unique<drawtypes::Icon>(vec[1]));
  }

  register_command_handler(name());
}

void BspwmModule::start()
{
  this->socket_fd = io::socket::open(BSPWM_SOCKET_PATH);

  if (io::socket::send(this->socket_fd, "subscribe\x00report") == 0)
    throw ModuleError("Failed to subscribe to bspwm changes");

  this->EventModule<BspwmModule>::start();
}

bool BspwmModule::has_event() {
  return io::poll_read(this->socket_fd, 100);
}

bool BspwmModule::update()
{
  std::string data;

  if ((data = io::readline(this->socket_fd)).empty())
    return false;

  if (data == this->prev_data)
    return false;

  this->prev_data = data;

  unsigned long n, m;

  while ((n = data.find("\n")) != std::string::npos)
    data.erase(n);

  if (data.empty())
    return false;

  auto prefix = std::string(BSPWM_STATUS_PREFIX);

  if (data.find(prefix) != 0) {
    log_error("Received unknown status -> "+ data);
    return false;
  }

  // Cut out the relevant section for the current monitor
  if ((n = data.find(prefix +"M"+ this->monitor +":")) != std::string::npos) {
    if ((m = data.find(":m")) != std::string::npos) data = data.substr(n, m);
  } else if ((n = data.find(prefix +"m"+ this->monitor +":")) != std::string::npos) {
    if ((m = data.find(":M")) != std::string::npos) data = data.substr(n, m);
  } else if ((n = data.find("M"+ this->monitor +":")) != std::string::npos) {
    data.erase(0, n);
  } else if ((n = data.find("m"+ this->monitor +":")) != std::string::npos) {
    data.erase(0, n);
  }

  if (data.find(prefix) == 0) data.erase(0, 1);

  log_trace(data);

  this->modes.clear();
  this->workspaces.clear();

  bool monitor_focused = true;
  int workspace_n = 0;

  for (auto &&tag : string::split(data, ':')) {
    if (tag.empty()) continue;

    auto value = tag.size() > 0 ? tag.substr(1) : "";
    auto workspace_flag = WORKSPACE_NONE;
    auto mode_flag = MODE_NONE;

    switch (tag[0]) {
      case 'm': monitor_focused = false; break;
      case 'M': monitor_focused = true; break;
      case 'F': workspace_flag = WORKSPACE_ACTIVE; break;
      case 'O': workspace_flag = WORKSPACE_ACTIVE; break;
      case 'o': workspace_flag = WORKSPACE_OCCUPIED; break;
      case 'U': workspace_flag = WORKSPACE_URGENT; break;
      case 'u': workspace_flag = WORKSPACE_URGENT; break;
      case 'f': workspace_flag = WORKSPACE_EMPTY; break;
      case 'L':
        switch (value[0]) {
          case 0: break;
          case 'M': mode_flag = MODE_LAYOUT_MONOCLE; break;
          case 'T': mode_flag = MODE_LAYOUT_TILED; break;
          default: log_warning("[modules::Bspwm] Undefined L => "+ value);
        }
        break;

      case 'T':
        switch (value[0]) {
          case 0: break;
          case 'T': break;
          case '=': mode_flag = MODE_STATE_FULLSCREEN; break;
          case 'F': mode_flag = MODE_STATE_FLOATING; break;
          default: log_warning("[modules::Bspwm] Undefined T => "+ value);
        }
        break;

      case 'G':
        repeat(value.length())
        {
          switch (value[repeat_i]) {
            case 0: break;
            case 'L': mode_flag = MODE_NODE_LOCKED; break;
            case 'S': mode_flag = MODE_NODE_STICKY; break;
            case 'P': mode_flag = MODE_NODE_PRIVATE; break;
            default: log_warning("[modules::Bspwm] Undefined G => "+ value.substr(repeat_i, 1));
          }

          if (mode_flag != MODE_NONE && !this->mode_labels.empty())
            this->modes.emplace_back(&this->mode_labels.find(mode_flag)->second);
        }
        continue;
      default: log_warning("[modules::Bspwm] Undefined tag => "+ tag.substr(0, 1));
    }

    if (workspace_flag != WORKSPACE_NONE && this->formatter->has(TAG_LABEL_STATE)) {
      std::unique_ptr<drawtypes::Icon> &icon = this->icons->get(value, DEFAULT_WS_ICON);
      std::unique_ptr<drawtypes::Label> label = this->state_labels.find(workspace_flag)->second->clone();

      if (!monitor_focused)
        label->replace_defined_values(this->state_labels.find(WORKSPACE_DIMMED)->second);

      label->replace_token("%name%", value);
      label->replace_token("%icon%", icon->text);
      label->replace_token("%index%", std::to_string(++workspace_n));

      this->workspaces.emplace_back(std::make_unique<Workspace>(workspace_flag, std::move(label)));
    }

    if (mode_flag != MODE_NONE && !this->mode_labels.empty())
      this->modes.emplace_back(&this->mode_labels.find(mode_flag)->second);
  }

  if (!monitor_focused) this->modes.clear();

  return true;
}

bool BspwmModule::build(Builder *builder, const std::string& tag)
{
  if (tag != TAG_LABEL_STATE)
    return false;

  int workspace_n = 0;

  for (auto &&ws : this->workspaces) {
    if (!ws.get()->label->text.empty())
      builder->cmd(Cmd::LEFT_CLICK, std::string(EVENT_CLICK) + std::to_string(++workspace_n));

    builder->node(ws.get()->label);

    if (ws->flag == WORKSPACE_ACTIVE && this->formatter->has(TAG_LABEL_MODE)) {
      for (auto &&mode : this->modes)
        builder->node(mode->get());
    }

    if (!ws.get()->label->text.empty())
      builder->cmd_close(true);
  }

  return true;
}

bool BspwmModule::handle_command(const std::string& cmd)
{
  if (cmd.find(EVENT_CLICK) == std::string::npos || cmd.length() <= std::strlen(EVENT_CLICK))
    return false;

  int status = std::system(("bspc desktop -f "+ this->monitor +":^"+ cmd.substr(std::strlen(EVENT_CLICK))).c_str());
  log_trace(std::to_string(status));

  return true;
}
