#include <jni.h>
#include <android/log.h>
#include <GLES2/gl2.h>
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cmath>
#include <pthread.h>

// --- SEUS OFFSETS REAIS ---
constexpr uintptr_t GameFacade       = 0xAD68E18;
constexpr uintptr_t GameFacade_P2    = 0xB8;
constexpr uintptr_t BaseGame_Match   = 0x90;
constexpr uintptr_t Match_PlayerDict = 0xC0;
constexpr uintptr_t Match_LocalPlayer= 0xD8;
constexpr uintptr_t Dict_Entries     = 0x18;
constexpr uintptr_t Dict_Count       = 0x20;
constexpr uintptr_t Dict_EntryStride = 0x18;
constexpr uintptr_t Dict_EntryValue  = 0x10;

constexpr uintptr_t Camera_Follow    = 0x698;
constexpr uintptr_t Camera_Object    = 0x30;
constexpr uintptr_t Camera_Data      = 0x10;
constexpr uintptr_t Camera_Matrix    = 0x100;
constexpr uintptr_t Bones_Head       = 0x6a8;
constexpr uintptr_t Transform_Matrix = 0x38; 

uintptr_t base_address = 0;
int g_pid = -1;
bool aimbot_ativo = true;
bool esp_ativo = true;

template <typename T>
T read_mem(uintptr_t address) {
    if (g_pid == -1 || address == 0) return T{};
    T value;
    struct iovec local[1]; struct iovec remote[1];
    local[0].iov_base = &value; local[0].iov_len = sizeof(T);
    remote[0].iov_base = (void*)address; remote[0].iov_len = sizeof(T);
    process_vm_readv(g_pid, local, 1, remote, 1, 0);
    return value;
}

int get_pid(const char* package_name) {
    char cmd[256]; snprintf(cmd, sizeof(cmd), "pidof %s", package_name);
    FILE* fp = popen(cmd, "r");
    if (fp) { char pid_str[16]; if (fgets(pid_str, sizeof(pid_str), fp) != NULL) { pclose(fp); return atoi(pid_str); } pclose(fp); }
    return -1;
}

uintptr_t get_base_address(int pid, const char* lib_name) {
    char path[64]; snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    FILE* fp = fopen(path, "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, lib_name)) {
                uintptr_t addr; sscanf(line, "%lx-%*lx", &addr); fclose(fp); return addr;
            }
        }
        fclose(fp);
    }
    return 0;
}

struct Matrix4x4 { float m[4][4]; };
bool WorldToScreen(float x, float y, float z, Matrix4x4 matrix, float &sx, float &sy) {
    float w = matrix.m[3][0] * x + matrix.m[3][1] * y + matrix.m[3][2] * z + matrix.m[3][3];
    if (w < 0.01f) return false;
    float px = matrix.m[0][0] * x + matrix.m[0][1] * y + matrix.m[0][2] * z + matrix.m[0][3];
    float py = matrix.m[1][0] * x + matrix.m[1][1] * y + matrix.m[1][2] * z + matrix.m[1][3];
    float width = 1080.0f, height = 2400.0f;
    sx = (width / 2.0f) + (px * width / 2.0f) / w;
    sy = (height / 2.0f) - (py * height / 2.0f) / w;
    return true;
}

// Loop do Hack (Chamado pelo template)
void UpdateHack() {
    if (g_pid == -1) {
        g_pid = get_pid("com.dts.freefireth");
        if (g_pid != -1) base_address = get_base_address(g_pid, "libil2cpp.so");
    }
    if (base_address == 0) return;

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    int centerX = io.DisplaySize.x / 2;
    int centerY = io.DisplaySize.y / 2;

    uintptr_t gameFacade = read_mem<uintptr_t>(base_address + GameFacade);
    if (gameFacade == 0) return;
    uintptr_t gameFacade2 = read_mem<uintptr_t>(gameFacade + GameFacade_P2);
    if (gameFacade2 == 0) return;
    uintptr_t match = read_mem<uintptr_t>(gameFacade2 + BaseGame_Match);
    if (match == 0) return;

    uintptr_t localPlayer = read_mem<uintptr_t>(match + Match_LocalPlayer);
    Matrix4x4 viewMatrix;
    if (localPlayer != 0) {
        uintptr_t camFollow = read_mem<uintptr_t>(localPlayer + Camera_Follow);
        uintptr_t camObj = read_mem<uintptr_t>(camFollow + Camera_Object);
        uintptr_t camData = read_mem<uintptr_t>(camObj + Camera_Data);
        if (camData != 0) viewMatrix = read_mem<Matrix4x4>(camData + Camera_Matrix);
    }

    if (aimbot_ativo) {
        draw->AddCircle(ImVec2(centerX, centerY), 250.0f, IM_COL32(255, 0, 255, 255), 64, 2.0f);
    }

    uintptr_t playerDict = read_mem<uintptr_t>(match + Match_PlayerDict);
    if (playerDict != 0) {
        uintptr_t entries = read_mem<uintptr_t>(playerDict + Dict_Entries);
        int count = read_mem<int>(playerDict + Dict_Count);

        for (int i = 0; i < count; i++) {
            uintptr_t entity = read_mem<uintptr_t>(entries + 0x20 + (i * Dict_EntryStride) + Dict_EntryValue);
            if (entity == 0 || entity == localPlayer) continue;
            bool isDead = read_mem<bool>(entity + 0x7C);
            if (isDead) continue;

            uintptr_t headTransform = read_mem<uintptr_t>(entity + Bones_Head);
            if (headTransform == 0) continue;
            
            Matrix4x4 headMatrix = read_mem<Matrix4x4>(headTransform + Transform_Matrix);
            float ex = headMatrix.m[3][0];
            float ey = headMatrix.m[3][1];
            float ez = headMatrix.m[3][2];

            float screenX, screenY;
            if (WorldToScreen(ex, ey, ez, viewMatrix, screenX, screenY)) {
                if (esp_ativo) {
                    draw->AddRect(ImVec2(screenX - 30, screenY - 30), ImVec2(screenX + 30, screenY + 30), IM_COL32(255, 0, 0, 255), 0, 0, 2.0f);
                    draw->AddLine(ImVec2(centerX, 0), ImVec2(screenX, screenY - 30), IM_COL32(0, 255, 0, 255), 1.5f);
                }
            }
        }
    }
}

// Hook do ImGui (Padrão LGL)
extern "C" void imgui_init() {}
extern "C" void imgui_draw() { UpdateHack(); }
