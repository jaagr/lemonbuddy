#pragma once

#include "modules/meta/event_module.hpp"
#include "utils/bspwm.hpp"

POLYBAR_NS

namespace modules {
  enum class state_ws {
    WORKSPACE_NONE,
    WORKSPACE_ACTIVE,
    WORKSPACE_URGENT,
    WORKSPACE_EMPTY,
    WORKSPACE_OCCUPIED,
    WORKSPACE_FOCUSED_URGENT,
    WORKSPACE_FOCUSED_EMPTY,
    WORKSPACE_FOCUSED_OCCUPIED,
    WORKSPACE_DIMMED,  // used when the monitor is out of focus
    WORKSPACE_DIMMED_ACTIVE,
    WORKSPACE_DIMMED_URGENT,
    WORKSPACE_DIMMED_EMPTY,
    WORKSPACE_DIMMED_OCCUPIED
  };
  enum class state_mode {
    MODE_NONE,
    MODE_LAYOUT_MONOCLE,
    MODE_LAYOUT_TILED,
    MODE_STATE_FULLSCREEN,
    MODE_STATE_FLOATING,
    MODE_NODE_LOCKED,
    MODE_NODE_STICKY,
    MODE_NODE_PRIVATE
  };

  struct bspwm_monitor {
    vector<pair<state_ws, label_t>> workspaces;
    vector<label_t> modes;
    label_t label;
    string name;
    bool focused = false;
  };

  class bspwm_module : public event_module<bspwm_module> {
   public:
    using event_module::event_module;

    void setup();
    void stop();
    bool has_event();
    bool update();
    string get_output();
    bool build(builder* builder, const string& tag) const;
    bool handle_event(string cmd);
    bool receive_events() const {
      return true;
    }

   private:
    static constexpr auto DEFAULT_WS_ICON = "ws-icon-default";
    static constexpr auto DEFAULT_WS_LABEL = "%icon% %name%";
    static constexpr auto DEFAULT_MONITOR_LABEL = "%name%";
    static constexpr auto TAG_LABEL_MONITOR = "<label-monitor>";
    static constexpr auto TAG_LABEL_STATE = "<label-state>";
    static constexpr auto TAG_LABEL_MODE = "<label-mode>";
    static constexpr auto EVENT_PREFIX = "bwm";
    static constexpr auto EVENT_CLICK = "bwmf";
    static constexpr auto EVENT_SCROLL_UP = "bwmn";
    static constexpr auto EVENT_SCROLL_DOWN = "bwmp";

    bspwm_util::connection_t m_subscriber;

    vector<unique_ptr<bspwm_monitor>> m_monitors;

    map<state_mode, label_t> m_modelabels;
    map<state_ws, label_t> m_statelabels;
    label_t m_monitorlabel;
    iconset_t m_icons;

    bool m_click = true;
    bool m_scroll = true;
    bool m_pinworkspaces = true;
    unsigned long m_hash;

    // used while formatting output
    size_t m_index = 0;
  };
}

POLYBAR_NS_END
