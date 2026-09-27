# ctinylogger

Logger mínimo para C, multiplataforma (**Linux**, **Windows 7/10/11** e
**Android/Termux**), com suporte a cores, UTF-8 e formatação estilo
`printf`.

```c
#include <ctinylogger/ctinylogger.h>

int main(void) {
    log_info("Servidor iniciado na porta %d", 8080);
    log_debug("valor = %d", 42);              // mostra arquivo:linha (função)
    log_warning("cache expirado para '%s'", "sessao_123");
    log_error("falha ao conectar: %s", "timeout");
    return 0;
}
```

Saída (com cores no terminal):

```
[14:32:01] [INFO] Servidor iniciado na porta 8080
[14:32:01] [DEBUG] main.c:5 (main) valor = 42
[14:32:01] [WARNING] cache expirado para 'sessao_123'
[14:32:01] [ERROR] falha ao conectar: timeout
```

## Recursos

- **4 níveis de log**: `log_debug`, `log_info`, `log_warning`, `log_error`
- **Formatação `printf`** completa (`stdarg.h`), qualquer número de argumentos
- **`log_debug` automático**: mostra nome do arquivo (sem caminho completo), linha e função de onde foi chamado
- **Cores configuráveis** por nível, com modo automático (só colore em terminal interativo), forçado, ou desligado
- **UTF-8 / caracteres largos** funcionam nas três plataformas, inclusive emojis
- **Prefixos customizáveis** por nível (ex: trocar `[INFO]` por `[APP]`)
- **Timestamp** configurável (liga/desliga, formato `strftime`)
- **Filtro de nível mínimo** (ex: silenciar `log_debug` em produção sem remover as chamadas do código)
- **Thread-safe** por padrão (mutex interno; pode ser desligado)
- `WARNING`/`ERROR` vão para `stderr`; `DEBUG`/`INFO` vão para `stdout` (configurável)
- Zero dependências externas — só a lib padrão do C e, no Windows, a API do Console

## Instalação / build

Requer **CMake ≥ 3.15** e um compilador C11 (GCC, Clang ou MSVC).

```bash
git clone <url-do-repositorio> ctinylogger
cd ctinylogger
cmake -S . -B build
cmake --build build --config Release
```

Isso gera:

- a lib estática (`build/libctinylogger.a` no Linux/Android/Termux, `build\ctinylogger.lib` no Windows)
- o executável de exemplo `ctinylogger_example` (desligue com `-DCTINYLOGGER_BUILD_EXAMPLE=OFF`)

Para instalar no sistema (headers + lib):

```bash
cmake --install build --prefix /caminho/de/instalacao
```

### Termux (Android)

```bash
pkg install cmake clang
cmake -S . -B build && cmake --build build
```

### Windows

Funciona com MSVC (Visual Studio) ou MinGW:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

### Usando a lib em outro projeto (CMake)

```cmake
add_subdirectory(caminho/para/ctinylogger)
target_link_libraries(seu_alvo PRIVATE ctinylogger)
```

Ou, sem CMake: compile os `.c` de `src/` (mais o arquivo de
`src/platform/<posix|windows>/` correto para o seu SO) e adicione
`include/` ao *include path*. No Linux/Termux é necessário linkar com
`-lpthread`.

## API

### Funções de log

```c
log_debug(fmt, ...);    // inclui arquivo, linha e função automaticamente
log_info(fmt, ...);
log_warning(fmt, ...);
log_error(fmt, ...);
```

São macros sobre as funções reais (`ctl_log_info`, `ctl_log_warning`,
`ctl_log_error`, `ctl_log_debug_impl`). Se os nomes curtos conflitarem
com algo do seu projeto, defina `CTINYLOGGER_NO_SHORT_NAMES` antes do
`#include` — as funções `ctl_log_*` continuam disponíveis normalmente.

### Configuração global

```c
void ctl_config_init(void);                                   // opcional, auto-chamado no 1º log
void ctl_set_color(ctl_level_t level, ctl_color_t color);
void ctl_set_prefix(ctl_level_t level, const char *prefix);
void ctl_set_color_mode(ctl_color_mode_t mode);                // AUTO | ON | OFF
void ctl_set_timestamp_enabled(bool enabled);
void ctl_set_timestamp_format(const char *strftime_fmt);
void ctl_set_min_level(ctl_level_t level);                     // filtra níveis abaixo deste
void ctl_set_thread_safe(bool enabled);
void ctl_set_errors_to_stderr(bool enabled);                   // WARNING/ERROR -> stderr (padrão: true)
```

Exemplo de customização:

```c
ctl_set_color(CTL_LEVEL_INFO, CTL_COLOR_BRIGHT_MAGENTA);
ctl_set_prefix(CTL_LEVEL_INFO, "[APP]");
ctl_set_timestamp_format("%Y-%m-%d %H:%M:%S");
ctl_set_min_level(CTL_LEVEL_INFO);   // desliga log_debug
```

### Cores disponíveis (`ctl_color_t`)

`CTL_COLOR_DEFAULT`, `BLACK`, `RED`, `GREEN`, `YELLOW`, `BLUE`,
`MAGENTA`, `CYAN`, `WHITE` e as variantes `BRIGHT_*` de cada uma.

### Padrões de fábrica

| Nível   | Cor padrão      | Prefixo     | Stream |
|---------|-----------------|-------------|--------|
| DEBUG   | Ciano           | `[DEBUG]`   | stdout |
| INFO    | Verde           | `[INFO]`    | stdout |
| WARNING | Amarelo         | `[WARNING]` | stderr |
| ERROR   | Vermelho brilhante | `[ERROR]` | stderr |

## Organização do projeto

```
ctinylogger/
├── include/ctinylogger/ctinylogger.h   # API pública (único header que você inclui)
├── src/
│   ├── ctl_log.c        # núcleo de log (independente de plataforma)
│   ├── ctl_config.c     # configuração global + mutex
│   ├── ctl_internal.h   # tipos internos compartilhados
│   └── platform/
│       ├── ctl_platform.h            # interface comum de plataforma
│       ├── posix/ctl_platform_posix.c    # Linux + Android/Termux (ANSI + pthread)
│       └── windows/ctl_platform_windows.c # Windows (WriteConsoleW + CRITICAL_SECTION)
├── examples/example.c
├── CMakeLists.txt
└── README.md
```

Linux e Android/Termux compartilham a mesma implementação POSIX (mesma
libc-compatível o suficiente: terminal UTF-8 nativo + sequências ANSI +
pthreads). O Windows tem sua própria implementação porque usa uma API de
console completamente diferente — isso é o que garante a saída correta
mesmo no console clássico do Windows 7, que não entende ANSI nem lida
bem com UTF-8 via `printf`.

## Notas de compatibilidade

- **Windows 7/10/11**: cor via `SetConsoleTextAttribute` e texto via
  `WriteConsoleW` (UTF-16 nativo) — não depende de Virtual Terminal
  Processing nem de ajuste de codepage do console, por isso funciona de
  forma idêntica em qualquer uma das três versões. Quando a saída é
  redirecionada para arquivo, cai para escrita UTF-8 direta.
- **Linux/Android/Termux**: cor via códigos de escape ANSI; desativada
  automaticamente quando a saída não é um terminal (`isatty`), a menos
  que `ctl_set_color_mode(CTL_COLOR_MODE_ON)` seja usado para forçar.
- Mensagens de log podem ter qualquer tamanho — internamente a lib usa
  um buffer fixo (rápido) para o caso comum e cai para alocação
  dinâmica apenas quando a mensagem excede esse limite.

## Licença

MIT — sinta-se livre para usar, modificar e distribuir.
