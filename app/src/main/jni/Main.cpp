#include <jni.h>
#include <android/log.h>
#include <cstdint>
#include <cstring>
#include <thread>
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.hpp"
#include "Menu/Menu.hpp"
#include "dobby.h"

// Variabel Kontrol (Akses dari menu)
bool godMode = false;
bool noPoison = false;

// --- Definisikan Fungsi Hook (Logic Anda) ---

// God Mode Logic
bool (*old_HitAI)(void* instance, int dmgType);
bool fake_HitAI(void* instance, int dmgType) {
    if (godMode) return false; // Blokir damage
    return old_HitAI(instance, dmgType);
}

bool (*old_HitBlock)(int blockType, void* point, bool trap);
bool fake_HitBlock(int blockType, void* point, bool trap) {
    if (godMode) return false;
    return old_HitBlock(blockType, point, trap);
}

// No Poison Logic
void (*old_Poison)(int type);
void fake_Poison(int type) {
    if (noPoison) return; // NOP (Jangan lakukan apa-apa)
    old_Poison(type);
}

// --- Daftar Menu ---
jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    jobjectArray ret;
    const char *features[] = {
            OBFUSCATE("Category_Pixel Worlds Mods"),
            OBFUSCATE("6_Toggle_God Mode (AI & Trap)"), // featNum 6
            OBFUSCATE("7_Toggle_No Poison Status"),    // featNum 7
            OBFUSCATE("RichTextView_Offsets based on PW Dump.cs")
    };

    int Total_Feature = (sizeof features / sizeof features[0]);
    ret = (jobjectArray)env->NewObjectArray(Total_Feature, env->FindClass("java/lang/String"), env->NewStringUTF(""));
    for (int i = 0; i < Total_Feature; i++)
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));
    return (ret);
}

// --- Listener Klik Menu ---
void Changes(JNIEnv *env, jclass clazz, jobject obj, jint featNum, jstring featName, jint value, jlong Lvalue, jboolean boolean, jstring text) {
    switch (featNum) {
        case 6: // God Mode
            godMode = boolean;
            break;
        case 7: // No Poison
            noPoison = boolean;
            break;
    }
}

// --- Thread Utama Hooking ---
#define targetLib OBFUSCATE("libil2cpp.so")

void hack_thread() {
    while (!isLibraryLoaded(targetLib)) {
        sleep(1);
    }

#if defined(__aarch64__)
    // Gunakan Dobby HOOK (Lebih aman dari manual memcpy)
    // Format: HOOK(NamaLib, Offset, FungsiBaru, FungsiLama)
    
    // God Mode
    HOOK(targetLib, "0x2096254", fake_HitAI, old_HitAI);
    HOOK(targetLib, "0x208A204", fake_HitBlock, old_HitBlock);

    // No Poison
    HOOK(targetLib, "0x2095804", fake_Poison, old_Poison); 
    // Tambahkan offset lainnya sesuai kebutuhan dengan pola yang sama
#endif

    LOGI("LGL Hooks Applied!");
}

__attribute__((constructor))
void lib_main() {
    std::thread(hack_thread).detach();
}
