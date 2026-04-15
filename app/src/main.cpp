#include <jni.h>
#include <android/log.h>
#include <cstdint>
#include <cstring>

#define LOG_TAG "LGL_MOD"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// ─── Base address helper ───────────────────────────────────────────
uintptr_t getBaseAddress(const char* libName) {
    char line[512];
    FILE* fp = fopen("/proc/self/maps", "r");
    if (!fp) return 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, libName)) {
            fclose(fp);
            return (uintptr_t)strtoul(line, nullptr, 16);
        }
    }
    fclose(fp);
    return 0;
}

// ─── Inline hook (ARM64 trampoline) ───────────────────────────────
void hookFunction(uintptr_t target, uintptr_t replace) {
    // LDR X16, #8 / BR X16 / <addr>
    uint32_t trampoline[4];
    trampoline[0] = 0x58000050; // LDR X16, #8
    trampoline[1] = 0xD61F0200; // BR X16
    memcpy(&trampoline[2], &replace, sizeof(uintptr_t));

    // Unprotect memory
    uintptr_t page = target & ~0xFFF;
    mprotect((void*)page, 0x1000, PROT_READ | PROT_WRITE | PROT_EXEC);
    memcpy((void*)target, trampoline, sizeof(trampoline));
    __builtin___clear_cache((char*)target, (char*)(target + sizeof(trampoline)));
}

// ─── Offset dari dump.cs Pixel World ──────────────────────────────
#define OFF_HitPlayerFromAIEnemy_1  0x2096254
#define OFF_HitPlayerFromAIEnemy_2  0x2096464
#define OFF_HitPlayerFromBlock_1    0x208A204
#define OFF_HitPlayerFromBlock_2    0x20958E4
#define OFF_CausePoisoned_Block     0x2095804
#define OFF_CausePoisoned_Enemy     0x2096B8C
#define OFF_CausePoisoned_Float     0x2096A78
#define OFF_OnTriggerEnter_Player   0x2091CE4
#define OFF_IsBlockHot              0x1F0CF30

// ─── Hook implementations ──────────────────────────────────────────

// God Mode: semua hit dari AI → return false (no damage)
bool fake_HitPlayerFromAIEnemy_1(void* aiEnemy, int dmgType) {
    LOGI("HitPlayerFromAIEnemy (AIBase) blocked");
    return false;
}

bool fake_HitPlayerFromAIEnemy_2(int hitForce, int enemyType) {
    LOGI("HitPlayerFromAIEnemy (int) blocked");
    return false;
}

// God Mode: hit dari block/trap → return false
bool fake_HitPlayerFromBlock_1(int blockType, void* point, bool trap) {
    LOGI("HitPlayerFromBlock (BlockType) blocked");
    return false;
}

bool fake_HitPlayerFromBlock_2(int hitForce, int blockType, void* point) {
    LOGI("HitPlayerFromBlock (int) blocked");
    return false;
}

// No Poison: semua overload CausePoisoned → NOP
void fake_CausePoisoned_Block(int blockType) {
    LOGI("CausePoisoned (Block) suppressed");
}

void fake_CausePoisoned_Enemy(int enemyType) {
    LOGI("CausePoisoned (Enemy) suppressed");
}

void fake_CausePoisoned_Float(float cooldown) {
    LOGI("CausePoisoned (float) suppressed");
}

// No Hot Block damage
bool fake_IsBlockHot(int blockType) {
    LOGI("IsBlockHot → always false");
    return false;
}

// ─── Entry point ───────────────────────────────────────────────────
__attribute__((constructor))
void init() {
    LOGI("LGL Mod loading...");

    uintptr_t base = getBaseAddress("libil2cpp.so");
    if (!base) {
        LOGI("ERROR: libil2cpp.so not found");
        return;
    }

    LOGI("Base: 0x%lx", base);

    hookFunction(base + OFF_HitPlayerFromAIEnemy_1, (uintptr_t)fake_HitPlayerFromAIEnemy_1);
    hookFunction(base + OFF_HitPlayerFromAIEnemy_2, (uintptr_t)fake_HitPlayerFromAIEnemy_2);
    hookFunction(base + OFF_HitPlayerFromBlock_1,   (uintptr_t)fake_HitPlayerFromBlock_1);
    hookFunction(base + OFF_HitPlayerFromBlock_2,   (uintptr_t)fake_HitPlayerFromBlock_2);
    hookFunction(base + OFF_CausePoisoned_Block,    (uintptr_t)fake_CausePoisoned_Block);
    hookFunction(base + OFF_CausePoisoned_Enemy,    (uintptr_t)fake_CausePoisoned_Enemy);
    hookFunction(base + OFF_CausePoisoned_Float,    (uintptr_t)fake_CausePoisoned_Float);
    hookFunction(base + OFF_IsBlockHot,             (uintptr_t)fake_IsBlockHot);

    LOGI("All hooks applied!");
}