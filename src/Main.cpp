#include "plugin.h"
#include "extensions/FontPrint.h"

using namespace plugin;

// Статическая инициализация — конструктор запускается при загрузке DLL игрой
static struct SanAndreasWebSocket {
    SanAndreasWebSocket() {
        Events::drawingEvent += [] {
            gamefont::Print("SanAndreasWebSocket: OK", 10.0f, 10.0f);
        };
    }
} g_plugin;
