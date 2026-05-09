#include <windows.h>
#include <tlhelp32.h>
#include <cstdint>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

// --------------------------------------------------
template<typename T>
struct Optional {
    bool has;
    T val;
    Optional() : has(false) {}
    Optional(const T& v) : has(true), val(v) {}
};

// --------------------------------------------------
DWORD getProcessIdByName(const std::wstring& name) {
    PROCESSENTRY32W pe = { sizeof(pe) };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;

    if (Process32FirstW(snap, &pe)) {
        do {
            if (name == pe.szExeFile) {
                CloseHandle(snap);
                return pe.th32ProcessID;
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return 0;
}

Optional<MODULEENTRY32W> getModuleByName(DWORD pid, const std::wstring& name) {
    MODULEENTRY32W me = { sizeof(me) };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return Optional<MODULEENTRY32W>();

    if (Module32FirstW(snap, &me)) {
        do {
            if (name == me.szModule) {
                CloseHandle(snap);
                return Optional<MODULEENTRY32W>(me);
            }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return Optional<MODULEENTRY32W>();
}

// --------------------------------------------------
bool parsePattern(const std::string& s, std::vector<uint8_t>& bytes, std::string& mask) {
    bytes.clear();
    mask.clear();
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok) {
        if (tok == "??" || tok == "?") {
            bytes.push_back(0);
            mask.push_back('?');
        } else {
            unsigned int v;
            std::stringstream ss;
            ss << std::hex << tok;
            ss >> v;
            if (ss.fail()) return false;
            bytes.push_back(static_cast<uint8_t>(v));
            mask.push_back('x');
        }
    }
    return true;
}

uintptr_t scanPattern(const uint8_t* data, size_t size, const std::vector<uint8_t>& pat,
                      const std::string& mask, uintptr_t base) {
    size_t len = pat.size();
    if (!len || size < len) return 0;

    for (size_t i = 0; i <= size - len; ++i) {
        bool ok = true;
        for (size_t j = 0; j < len; ++j) {
            if (mask[j] == 'x' && data[i + j] != pat[j]) {
                ok = false;
                break;
            }
        }
        if (ok) return base + i;
    }
    return 0;
}

// --------------------------------------------------
bool readRemote(HANDLE h, uintptr_t addr, std::vector<uint8_t>& out, size_t size) {
    out.resize(size);
    SIZE_T r = 0;
    return ReadProcessMemory(h, (LPCVOID)addr, out.data(), size, &r) && r == size;
}

bool writeRemote(HANDLE h, uintptr_t addr, const std::vector<uint8_t>& data) {
    DWORD oldProt;
    SIZE_T written;
    if (!VirtualProtectEx(h, (LPVOID)addr, data.size(), PAGE_EXECUTE_READWRITE, &oldProt))
        return false;

    bool ok = WriteProcessMemory(h, (LPVOID)addr, data.data(), data.size(), &written) &&
              written == data.size();

    FlushInstructionCache(h, (LPCVOID)addr, data.size());
    VirtualProtectEx(h, (LPVOID)addr, data.size(), oldProt, &oldProt);
    return ok;
}

// --------------------------------------------------
struct PatchInfo {
    uintptr_t addr = 0;
    std::vector<uint8_t> patchBytes;
    std::string name;
    bool isPartial = false;
};

// Funções para adicionar patches (substituindo lambdas)
void addFullRetPatch(std::vector<PatchInfo>& patches, HANDLE hProc, const uint8_t* image, size_t imageSize, uintptr_t base, const std::string& patStr, const std::string& name) {
    std::vector<uint8_t> pat;
    std::string mask;
    if (!parsePattern(patStr, pat, mask)) return;

    uintptr_t addr = scanPattern(image, imageSize, pat, mask, base);
    if (!addr) {
        std::cout << name << " NAO ENCONTRADO\n";
        return;
    }

    PatchInfo pi;
    pi.addr = addr;
    pi.name = name;
    pi.patchBytes.assign(pat.size(), 0xC3);
    patches.push_back(pi);
    std::cout << name << " encontrado @ 0x" << std::hex << addr << std::dec << "\n";
}

void addInfiniteFuelPatch(std::vector<PatchInfo>& patches, HANDLE hProc, const uint8_t* image, size_t imageSize, uintptr_t base) {
    const std::string patStr = "8B FA 74 0B 48 8B 01 8B D7";
    std::vector<uint8_t> pat;
    std::string mask;
    if (!parsePattern(patStr, pat, mask)) return;

    uintptr_t addr = scanPattern(image, imageSize, pat, mask, base);
    if (!addr) {
        std::cout << "InfiniteFuel NAO ENCONTRADO\n";
        return;
    }

    PatchInfo pi;
    pi.addr = addr + 2;
    pi.name = "InfiniteFuel";
    pi.isPartial = true;
    pi.patchBytes = {0xEB};
    patches.push_back(pi);
    std::cout << "InfiniteFuel encontrado @ 0x" << std::hex << addr << " (patch em +2 = 0x" << (addr + 2) << ")" << std::dec << "\n";
}

// --------------------------------------------------
int main() {
    std::cout << "ETS2 Patcher AUTOMATICO (LCC-Win64) - 1.57.x - by j1m1l0k0 - Jan 04/01/2026\n";

    const std::wstring procName = L"eurotrucks2.exe";
    DWORD pid = 0;
    for (int i = 0; i < 20 && !pid; ++i) {
        pid = getProcessIdByName(procName);
        if (!pid) Sleep(1000);  // 1 segundo
    }

    if (!pid) {
        std::cerr << "Processo eurotrucks2.exe nao encontrado. Inicie o jogo primeiro.\n";
        Sleep(5000);
        return 1;
    }

    auto mod = getModuleByName(pid, procName);
    if (!mod.has) {
        std::cerr << "Modulo principal nao encontrado.\n";
        return 1;
    }

    HANDLE hProc = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProc) {
        std::cerr << "Falha ao abrir o processo (execute como Administrador).\n";
        return 1;
    }

    uintptr_t base = (uintptr_t)mod.val.modBaseAddr;
    size_t size = mod.val.modBaseSize;
    std::vector<uint8_t> image(size);

    if (!ReadProcessMemory(hProc, (LPCVOID)base, image.data(), size, nullptr)) {
        std::cerr << "Falha ao ler memoria do jogo.\n";
        CloseHandle(hProc);
        return 1;
    }

    std::vector<PatchInfo> patches;

    addFullRetPatch(patches, hProc, image.data(), image.size(), base, "40 55 56 57 48 83 EC 70 48 8B B1 ?? 01 00 00", "NoWearTruck");
    addFullRetPatch(patches, hProc, image.data(), image.size(), base, "48 83 EC 18 4C 8B 81 ?? 01 00 00", "NoDamageTruck");
    addFullRetPatch(patches, hProc, image.data(), image.size(), base, "48 83 EC 48 4C 8B 81 C8 00 00 00 0F 29 7C 24 20 0F 28 FA", "NoCargoDamage");
    addFullRetPatch(patches, hProc, image.data(), image.size(), base, "40 55 41 56 41 57 48 81 EC 80 00 00 00 48 8B A9 ?? 01 00 00", "NoTrailerWear");
    addFullRetPatch(patches, hProc, image.data(), image.size(), base, "48 8B 81 ?? 01 00 00 0F 57 ED", "NoTrailerDamage");
    addInfiniteFuelPatch(patches, hProc, image.data(), image.size(), base);

    if (patches.empty()) {
        std::cerr << "Nenhum patch encontrado - verifique a versao do jogo.\n";
        CloseHandle(hProc);
        Sleep(5000);
        return 1;
    }

    std::cout << "\nVerificando e aplicando patches...\n";
    bool allApplied = true;
    for (const auto& p : patches) {
        std::vector<uint8_t> current;
        if (!readRemote(hProc, p.addr, current, p.patchBytes.size())) {
            std::cout << "ERRO ao ler memoria para " << p.name << "\n";
            allApplied = false;
            continue;
        }

        if (current == p.patchBytes) {
            std::cout << "JA APLICADO: " << p.name << (p.isPartial ? " (jmp)" : " (RET)") << "\n";
        } else {
            if (writeRemote(hProc, p.addr, p.patchBytes)) {
                std::cout << "APLICADO: " << p.name << (p.isPartial ? " (jmp)" : " (RET)") << "\n";
            } else {
                std::cout << "FALHA ao aplicar: " << p.name << "\n";
                allApplied = false;
            }
        }
    }

    CloseHandle(hProc);

    if (allApplied && !patches.empty()) {
        std::cout << "\nTodos os patches estao ATIVOS! Boa viagem (single-player).\n";
    } else {
        std::cout << "\nAlguns patches falharam.\n";
    }

    Sleep(4000);
    return 0;
}
