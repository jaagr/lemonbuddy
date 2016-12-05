#include "modules/script.hpp"
#include "drawtypes/label.hpp"

#include "modules/meta/base.inl"
#include "modules/meta/event_module.inl"

POLYBAR_NS

namespace modules {
  template class module<script_module>;
  template class event_module<script_module>;

  void script_module::setup() {
    REQ_CONFIG_VALUE(name(), m_exec, "exec");
    GET_CONFIG_VALUE(name(), m_tail, "tail");
    GET_CONFIG_VALUE(name(), m_maxlen, "maxlen");
    GET_CONFIG_VALUE(name(), m_ellipsis, "ellipsis");

    m_conf.warn_deprecated(
        name(), "maxlen", "\"format = <label>\" and \"label = %output:0:" + to_string(m_maxlen) + "%\"");

    m_actions[mousebtn::LEFT] = m_conf.get<string>(name(), "click-left", "");
    m_actions[mousebtn::MIDDLE] = m_conf.get<string>(name(), "click-middle", "");
    m_actions[mousebtn::RIGHT] = m_conf.get<string>(name(), "click-right", "");
    m_actions[mousebtn::SCROLL_UP] = m_conf.get<string>(name(), "scroll-up", "");
    m_actions[mousebtn::SCROLL_DOWN] = m_conf.get<string>(name(), "scroll-down", "");

    m_interval = chrono::duration<double>{m_conf.get<double>(name(), "interval", m_tail ? 0.0 : 5.0)};

    m_formatter->add(DEFAULT_FORMAT, TAG_LABEL, {TAG_OUTPUT, TAG_LABEL});

    if (m_formatter->has(TAG_LABEL)) {
      m_label = load_optional_label(m_conf, name(), "label", "%output%");
    } else if (m_formatter->has(TAG_OUTPUT)) {
      m_log.warn("%s: The format tag <output> is deprecated, use <label> instead", name());
    }
  }

  void script_module::stop() {
    if (m_command && m_command->is_running()) {
      m_log.warn("%s: Stopping shell command", name());
      m_command->terminate();
    }
    wakeup();
    event_module::stop();
  }

  void script_module::idle() {
    if (!m_tail) {
      sleep(m_interval);
    } else if (!m_command || !m_command->is_running()) {
      sleep(m_interval);
    }
  }

  bool script_module::has_event() {
    if (!m_tail) {
      return true;
    }

    try {
      if (!m_command || !m_command->is_running()) {
        auto exec = string_util::replace_all(m_exec, "%counter%", to_string(++m_counter));
        m_log.trace("%s: Executing '%s'", name(), exec);

        m_command = command_util::make_command(exec);
        m_command->exec(false);
      }
    } catch (const std::exception& err) {
      m_log.err("%s: %s", name(), err.what());
      throw module_error("Failed to execute tail command, stopping module...");
    }

    if (!m_command) {
      return false;
    }

    if ((m_output = m_command->readline()) == m_prev) {
      return false;
    }

    m_prev = m_output;

    return true;
  }

  bool script_module::update() {
    if (m_tail) {
      return true;
    }

    try {
      auto exec = string_util::replace_all(m_exec, "%counter%", to_string(++m_counter));
      m_log.info("%s: Executing \"%s\"", name(), exec);
      m_command = command_util::make_command(exec);
      m_command->exec();
      m_command->tail([&](string output) { m_output = output; });
    } catch (const std::exception& err) {
      m_log.err("%s: %s", name(), err.what());
      throw module_error("Failed to execute command, stopping module...");
    }

    if (m_output == m_prev) {
      return false;
    }

    m_prev = m_output;
    return true;
  }

  string script_module::get_output() {
    if (m_output.empty()) {
      return " ";
    }

    if (m_label) {
      m_label->reset_tokens();
      m_label->replace_token("%output%", m_output);
    }

    if (m_maxlen > 0 && m_output.length() > m_maxlen) {
      m_output.erase(m_maxlen);
      m_output += m_ellipsis ? "..." : "";
    }

    auto counter_str = to_string(m_counter);
    OUTPUT_ACTION(mousebtn::LEFT);
    OUTPUT_ACTION(mousebtn::MIDDLE);
    OUTPUT_ACTION(mousebtn::RIGHT);
    OUTPUT_ACTION(mousebtn::SCROLL_UP);
    OUTPUT_ACTION(mousebtn::SCROLL_DOWN);
    m_builder->append(module::get_output());

    return m_builder->flush();
  }

  bool script_module::build(builder* builder, const string& tag) const {
    if (tag == TAG_OUTPUT) {
      builder->node(m_output);
    } else if (tag == TAG_LABEL) {
      builder->node(m_label);
    } else {
      return false;
    }

    return true;
  }
}

POLYBAR_NS_END
