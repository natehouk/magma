// Magma — plugin entry point. License: MIT.
#include "MagmaPlugin.hpp"

START_NAMESPACE_DISTRHO

Plugin* createPlugin() { return new MagmaPlugin(); }

END_NAMESPACE_DISTRHO
