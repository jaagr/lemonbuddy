#pragma once

#include <unordered_map>

#include "common.hpp"
#include "components/logger.hpp"
#include "components/types.hpp"
#include "x11/extensions/randr.hpp"

POLYBAR_NS

namespace restack {
  struct wm_restacker {
    explicit wm_restacker(std::string&& name);

    virtual ~wm_restacker() = default;
    virtual void operator()(connection& connection, const bar_settings& settings, const logger& logger) const = 0;
  };

  using restacker_map_t = std::unordered_map<std::string, wm_restacker&>;

  restacker_map_t& get_restacker_map();
  wm_restacker* get_restacker(const std::string& wm_name);


}  // namespace restack

POLYBAR_NS_END
