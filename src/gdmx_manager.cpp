#include "gdmx_manager.h"

GDMXManager& GDMXManager::get()
{
  static GDMXManager instance{};
  return instance;
}

void GDMXManager::join(size_t id) {}

void GDMXManager::unjoin(size_t id) {}
