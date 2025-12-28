#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include "module_base.h"
#include <vector>
#include <ESPAsyncWebServer.h>

/**
 * Module Manager - Central registry and lifecycle manager for all modules
 */
class ModuleManager {
public:
    static ModuleManager& getInstance() {
        static ModuleManager instance;
        return instance;
    }
    
    /**
     * Register a module
     */
    void registerModule(ModuleBase* module) {
        if (!module) return;
        
        modules.push_back(module);
        Serial.printf("📦 Registered module: %s (%s)\n", 
            module->getName(), module->getDescription());
    }
    
    /**
     * Initialize all registered modules
     */
    void initAll() {
        Serial.println("\n═══════════════════════════════════════");
        Serial.println("📦 Initializing Modules");
        Serial.println("═══════════════════════════════════════");
        
        for (auto* module : modules) {
            if (!module->isEnabled()) {
                Serial.printf("⏭️  [%s] Disabled, skipping\n", module->getName());
                continue;
            }
            
            Serial.printf("🔄 [%s] Initializing...\n", module->getName());
            if (module->init()) {
                Serial.printf("✅ [%s] Initialized successfully\n", module->getName());
            } else {
                Serial.printf("❌ [%s] Initialization failed\n", module->getName());
            }
        }
        
        Serial.println("═══════════════════════════════════════\n");
    }
    
    /**
     * Register all module APIs with the web server
     */
    void registerAllAPIs(AsyncWebServer& server) {
        Serial.println("🌐 Registering Module APIs...");
        
        for (auto* module : modules) {
            if (module->isEnabled() && module->isReady()) {
                module->registerAPI(server);
                Serial.printf("✅ [%s] API registered\n", module->getName());
            }
        }
        
        // Register system-wide module status endpoint
        server.on("/_api/modules/list", HTTP_GET, [this](AsyncWebServerRequest *request) {
            JsonDocument doc;
            JsonArray modulesArray = doc["modules"].to<JsonArray>();
            
            for (auto* module : this->modules) {
                JsonObject moduleObj = modulesArray.add<JsonObject>();
                module->getStatus(moduleObj);
            }
            
            doc["count"] = this->modules.size();
            
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
        });
        
        Serial.println("✅ All module APIs registered\n");
    }
    
    /**
     * Update all modules (call from main loop)
     */
    void updateAll() {
        for (auto* module : modules) {
            if (module->isEnabled() && module->isReady()) {
                module->update();
            }
        }
    }
    
    /**
     * Shutdown all modules
     */
    void shutdownAll() {
        Serial.println("🔄 Shutting down modules...");
        for (auto* module : modules) {
            module->shutdown();
        }
        Serial.println("✅ All modules shut down");
    }
    
    /**
     * Get module by name
     */
    ModuleBase* getModule(const char* name) {
        for (auto* module : modules) {
            if (strcmp(module->getName(), name) == 0) {
                return module;
            }
        }
        return nullptr;
    }
    
    /**
     * Get all modules
     */
    const std::vector<ModuleBase*>& getAllModules() const {
        return modules;
    }
    
    /**
     * Get module count
     */
    size_t getModuleCount() const {
        return modules.size();
    }

private:
    ModuleManager() {}
    ~ModuleManager() {
        for (auto* module : modules) {
            delete module;
        }
    }
    
    ModuleManager(const ModuleManager&) = delete;
    ModuleManager& operator=(const ModuleManager&) = delete;
    
    std::vector<ModuleBase*> modules;
};

// Helper macro to register modules
#define REGISTER_MODULE(ModuleClass) \
    static struct ModuleClass##_Registrar { \
        ModuleClass##_Registrar() { \
            ModuleManager::getInstance().registerModule(new ModuleClass()); \
        } \
    } ModuleClass##_registrar_instance;

#endif

