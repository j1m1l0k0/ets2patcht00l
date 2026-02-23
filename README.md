📦 ETS2 Patcher Tools – Como Usar

Este programa é um patcher de memória para Euro Truck Simulator 2 – versão 1.57.x, que aplica modificações em tempo real no jogo (single-player), desativando desgaste, danos e consumo de combustível.

⚠️ Uso exclusivo em modo single-player.
⚠️ Nunca use em multiplayer (TruckersMP) — risco de banimento.

🧱 Requisitos

Windows 64-bit

Euro Truck Simulator 2 – versão 1.57.x

Executar o patcher como Administrador

Jogo aberto antes de rodar o patcher

🔧 Compilação

Compile o programa usando o Visual C++ (MSVC):

cl.exe /Zi /EHsc /nologo /W4 /std:c++17 ets2patcher.cpp /link /OUT:ets2patcher.exe

Isso irá gerar o arquivo:

ets2patcher.exe
▶️ Como Usar

Abra o Euro Truck Simulator 2

Entre no perfil single-player

Pode estar no menu ou já no jogo

Execute o ets2patcher.exe como Administrador

Clique com o botão direito

“Executar como administrador”

O patcher irá:

Localizar o processo eurotrucks2.exe

Ler a memória do módulo principal do jogo

Procurar assinaturas de código (pattern scan)

Aplicar os patches automaticamente

Se tudo der certo, você verá:

[SUCCESS] All patches are ACTIVE! Have a good trip (single-player).
🛠️ Patches Aplicados

O programa aplica os seguintes patches:

Patch	Função
NoWearTruck	Remove desgaste do caminhão
NoDamageTruck	Caminhão não sofre dano
NoCargoDamage	Carga não sofre dano
NoTrailerWear	Reboque sem desgaste
NoTrailerDamage	Reboque sem dano
InfiniteFuel	Combustível infinito
🔍 Como funcionam

A maioria dos patches substitui a função original por um RET (0xC3), fazendo a função “retornar” imediatamente.

O InfiniteFuel altera um salto condicional (JZ) para JMP, impedindo o consumo de combustível.

📋 Saída do Console (Exemplo)
[INFO] Searching for patch signatures...
[OK]   NoWearTruck found @ 0x7FF6A1234567
[OK]   NoDamageTruck found @ 0x7FF6A2345678
[OK]   InfiniteFuel found @ 0x7FF6A3456789
------------------------------------------
[INFO] Checking and applying patches...
[OK]   Patch applied: NoWearTruck (RET)
[OK]   Patch applied: InfiniteFuel (jmp)
------------------------------------------
[SUCCESS] All patches are ACTIVE!

Se algum patch não for encontrado:

[WARN] NoCargoDamage NOT FOUND

➡️ Isso normalmente indica versão diferente do jogo.

❗ Possíveis Erros
❌ Processo não encontrado
Process eurotrucks2.exe not found. Start the game first.

✔ Abra o jogo antes de rodar o patcher.

❌ Falha ao abrir processo
Failed to open process (run as Administrator)

✔ Execute como Administrador.

❌ Nenhum patch encontrado
[ERROR] No patch found - check the game version.

✔ O jogo não é 1.57.x ou foi atualizado.

🔁 Reaplicação

Sempre que fechar o jogo, os patches são perdidos.

Ao abrir o ETS2 novamente, execute o patcher de novo.

⚠️ Avisos Importantes

O patcher não altera arquivos do jogo, apenas memória RAM.

Seguro para uso offline / single-player.

Nunca use em multiplayer.

Atualizações do jogo quebram os padrões — será necessário atualizar o código.

🧠 Observação Técnica

O patcher utiliza:

CreateToolhelp32Snapshot

ReadProcessMemory

WriteProcessMemory

VirtualProtectEx

Pattern Scan com máscara (x / ?)

Flush de cache de instruções

Tudo aplicado dinamicamente, sem DLL injection.
