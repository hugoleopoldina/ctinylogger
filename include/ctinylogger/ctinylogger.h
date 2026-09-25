/*
 * ctinylogger - logger mínimo, multiplataforma (Linux, Windows, Android/Termux)
 *
 * API pública. Ver README.md para exemplos completos e detalhes de
 * configuração.
 */
#ifndef CTINYLOGGER_H
#define CTINYLOGGER_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Compatibilidade: MSVC só ganhou suporte a __func__ (C99) a partir do
 * Visual Studio 2015 (_MSC_VER 1900). Em versões mais antigas, o
 * equivalente é a macro não padronizada __FUNCTION__. */
#if defined(_MSC_VER) && _MSC_VER < 1900 && !defined(__func__)
#define __func__ __FUNCTION__
#endif

/* Níveis de log, em ordem crescente de severidade. A ordem importa: é
 * usada tanto para comparação com o nível mínimo configurado
 * (ctl_set_min_level) quanto como índice do vetor de configuração por
 * nível dentro da lib. */
typedef enum {
    CTL_LEVEL_DEBUG = 0,
    CTL_LEVEL_INFO,
    CTL_LEVEL_WARNING,
    CTL_LEVEL_ERROR,
    CTL_LEVEL_COUNT /* sentinela: quantidade de níveis, não é um nível válido */
} ctl_level_t;

/* Paleta de cores básica (8 cores + variantes "brilhantes"), compatível
 * tanto com códigos ANSI (Linux/Android/Termux) quanto com os atributos
 * de cor nativos do console do Windows (SetConsoleTextAttribute). */
typedef enum {
    CTL_COLOR_DEFAULT = 0, /* mantém a cor padrão do terminal */
    CTL_COLOR_BLACK,
    CTL_COLOR_RED,
    CTL_COLOR_GREEN,
    CTL_COLOR_YELLOW,
    CTL_COLOR_BLUE,
    CTL_COLOR_MAGENTA,
    CTL_COLOR_CYAN,
    CTL_COLOR_WHITE,
    CTL_COLOR_BRIGHT_BLACK,
    CTL_COLOR_BRIGHT_RED,
    CTL_COLOR_BRIGHT_GREEN,
    CTL_COLOR_BRIGHT_YELLOW,
    CTL_COLOR_BRIGHT_BLUE,
    CTL_COLOR_BRIGHT_MAGENTA,
    CTL_COLOR_BRIGHT_CYAN,
    CTL_COLOR_BRIGHT_WHITE
} ctl_color_t;

/* Controla quando as cores são efetivamente aplicadas na saída. */
typedef enum {
    CTL_COLOR_MODE_AUTO = 0, /* cores ligadas somente se a saída for um terminal (tty) */
    CTL_COLOR_MODE_ON,       /* força cores mesmo com saída redirecionada p/ arquivo/pipe */
    CTL_COLOR_MODE_OFF       /* desativa cores completamente */
} ctl_color_mode_t;

/* ---------------------------------------------------------------------
 * Funções de log "reais". Todas aceitam formatação estilo printf e um
 * número variável de argumentos (stdarg.h). Normalmente você não chama
 * estas diretamente: use as macros log_info/log_warning/log_error/
 * log_debug abaixo, que são mais curtas e, no caso de log_debug,
 * capturam arquivo/linha/função automaticamente.
 * ------------------------------------------------------------------- */
void ctl_log_info(const char *fmt, ...);
void ctl_log_warning(const char *fmt, ...);
void ctl_log_error(const char *fmt, ...);

/* Implementação real de log_debug. Recebe arquivo, linha e função além
 * do printf-fmt; é chamada indiretamente pela macro log_debug(), que
 * preenche esses três primeiros parâmetros automaticamente via
 * __FILE__/__LINE__/__func__. */
void ctl_log_debug_impl(const char *file, int line, const char *func, const char *fmt, ...);

/* Defina CTINYLOGGER_NO_SHORT_NAMES antes de incluir este header caso
 * os nomes curtos (log_info, log_debug, ...) conflitem com algo do seu
 * projeto. As funções ctl_log_* continuam disponíveis normalmente. */
#ifndef CTINYLOGGER_NO_SHORT_NAMES
#define log_debug(...)   ctl_log_debug_impl(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define log_info(...)    ctl_log_info(__VA_ARGS__)
#define log_warning(...) ctl_log_warning(__VA_ARGS__)
#define log_error(...)   ctl_log_error(__VA_ARGS__)
#endif

/* ---------------------------------------------------------------------
 * Configuração global. Todas as funções são thread-safe (protegidas
 * pelo mesmo mutex usado nas funções de log) e podem ser chamadas a
 * qualquer momento, inclusive antes de qualquer log (nesse caso a lib
 * se auto-inicializa com valores padrão antes de aplicar a mudança).
 * ------------------------------------------------------------------- */

/* Inicializa (ou reseta) a configuração global para os valores padrão:
 *  - cores: DEBUG=ciano, INFO=verde, WARNING=amarelo, ERROR=vermelho brilhante
 *  - prefixos: "[DEBUG]", "[INFO]", "[WARNING]", "[ERROR]"
 *  - modo de cor: automático (liga só em terminal interativo)
 *  - timestamp: habilitado, formato "%H:%M:%S"
 *  - nível mínimo: CTL_LEVEL_DEBUG (nada é filtrado)
 *  - thread-safe: habilitado
 *  - WARNING/ERROR vão para stderr; DEBUG/INFO vão para stdout
 * Chamar isto é opcional — a primeira chamada de log já inicializa
 * automaticamente — mas é útil para resetar configurações em testes.
 */
void ctl_config_init(void);

/* Define a cor usada para um nível específico de log. */
void ctl_set_color(ctl_level_t level, ctl_color_t color);

/* Define o prefixo textual usado para um nível específico (ex: "[DEBUG]").
 * O texto é truncado em 31 caracteres. */
void ctl_set_prefix(ctl_level_t level, const char *prefix);

/* Define quando as cores devem ser aplicadas (ver ctl_color_mode_t). */
void ctl_set_color_mode(ctl_color_mode_t mode);

/* Liga/desliga o timestamp no início de cada linha de log. */
void ctl_set_timestamp_enabled(bool enabled);

/* Define o formato do timestamp, no formato aceito por strftime().
 * Truncado em 31 caracteres. */
void ctl_set_timestamp_format(const char *strftime_fmt);

/* Define o nível mínimo a ser exibido: mensagens de nível inferior ao
 * definido são silenciosamente descartadas (comparação numérica, ver
 * ctl_level_t). Útil para, por exemplo, desligar log_debug em builds
 * de produção sem precisar remover as chamadas do código. */
void ctl_set_min_level(ctl_level_t level);

/* Liga/desliga a proteção por mutex das funções de log/configuração.
 * Habilitado por padrão. Desligue apenas se tiver certeza de que a lib
 * será usada em um único thread (ganho marginal de performance). */
void ctl_set_thread_safe(bool enabled);

/* Quando true (padrão), níveis WARNING e ERROR são escritos em stderr
 * em vez de stdout. DEBUG e INFO sempre vão para stdout. */
void ctl_set_errors_to_stderr(bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* CTINYLOGGER_H */
