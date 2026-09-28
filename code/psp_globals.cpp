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

#ifdef RAD_PSP
#include <pspiofilemgr.h>
void psp_frame_tick(int n) {
    SceUID fd = sceIoOpen("ms0:/hitr_frame.log", PSP_O_WRONLY|PSP_O_CREAT|PSP_O_APPEND, 0777);
    if (fd < 0) return;
    char b[32]; int i=0;
    const char* m = "frame "; while(m[i]){b[i]=m[i];i++;}
    int v=n; char t[12]; int k=0;
    if(v==0){t[k++]='0';} else {while(v>0){t[k++]='0'+(v%10);v/=10;}}
    for(int j=k-1;j>=0;j--) b[i++]=t[j];
    b[i++]='\n';
    sceIoWrite(fd, b, i); sceIoClose(fd);
}
#endif

#ifdef RAD_PSP
// Forward-declared in game.cpp. Pumps the radLoad queue one item per frame.
#include <radload/radload.hpp>
void psp_pump_radload_service()
{
    if (radLoad) radLoad->Service();
}
#endif
