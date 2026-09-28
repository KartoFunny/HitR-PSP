//=============================================================================
// psp_globals.cpp
//
// Глобальные символы, определённые в исключённых платформенных файлах.
//=============================================================================

#include <radlinkedclass.hpp>
#include <radscript.hpp>

// gFont — массив шрифта. Оригинал в win32platform.cpp/xboxplatform.cpp/ps2platform.cpp
// (все три исключены из PSP-сборки). Размер 61075 взят из supersprintdrawable.h.
// Реальный шрифт не критичен для первого запуска — пустого массива хватит.
unsigned char gFont[61075] = {0};

// gTuneSound — флаг tuning (определён в soundrenderingmanager.cpp, исключён)
bool gTuneSound = false;

// VALUES / DYNA_VALUES — глобальные tuning-таблицы
int VALUES = 0;
int DYNA_VALUES = 0;

// Специализации шаблона radLinkedClass<IRadNameSpace> — нужны из-за того,
// что в radLinkedClass.hpp мы убрали inline из static-полей (C++14).
template<> IRadNameSpace* radLinkedClass<IRadNameSpace>::s_pLinkedClassHead = nullptr;
template<> IRadNameSpace* radLinkedClass<IRadNameSpace>::s_pLinkedClassTail = nullptr;

//=============================================================================
// GCReverbController — заглушка (используется в soundeffectplayer.cpp,
// потому что PSP попадает в #else-ветку для GC).
//=============================================================================
#include <sound/soundfx/gcreverbcontroller.h>

GCReverbController::GCReverbController() : m_reverbInterface(nullptr) {}
GCReverbController::~GCReverbController() {}
void GCReverbController::SetReverbOn(reverbSettings*) {}
void GCReverbController::SetReverbOff() {}
