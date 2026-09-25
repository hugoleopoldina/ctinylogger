/*
 * Interface de plataforma. Cada plataforma suportada implementa estas
 * funções em seu próprio arquivo:
 *   - src/platform/posix/ctl_platform_posix.c    (Linux, Android/Termux)
 *   - src/platform/windows/ctl_platform_windows.c (Windows 7/10/11)
 *
 * Isso mantém todo o código específico de SO isolado em pastas próprias
 * (ver requisito de organização por responsabilidade), e o restante da
 * lib (ctl_log.c, ctl_config.c) nunca precisa saber em qual plataforma
 * está rodando.
 */
#ifndef CTL_PLATFORM_H
#define CTL_PLATFORM_H

#include "ctinylogger/ctinylogger.h"
#include <stdio.h>
#include <stdbool.h>

/* Inicializa recursos específicos da plataforma. Chamado uma única vez
 * por ctl_config_init(). No Windows, configura o codepage de saída para
 * UTF-8 (fallback para saída não-console); no POSIX é um no-op, pois
 * terminais Linux/Android já lidam nativamente com UTF-8. */
void ctl_platform_init(void);

/* Retorna true se 'stream' está conectado a um terminal interativo
 * (tty/console), false se foi redirecionado para arquivo/pipe. Usado
 * pelo modo de cor CTL_COLOR_MODE_AUTO. */
bool ctl_platform_isatty(FILE *stream);

/* Escreve a string UTF-8 já formatada (uma linha completa, já contendo
 * o '\n' final) em 'stream', aplicando 'color' caso 'use_color' seja
 * true e color != CTL_COLOR_DEFAULT.
 *
 * No POSIX, isso significa envolver o texto com sequências de escape
 * ANSI. No Windows, significa converter para UTF-16 e chamar
 * WriteConsoleW com o atributo de cor setado via
 * SetConsoleTextAttribute — método compatível até com o console
 * clássico do Windows 7, ao contrário de sequências ANSI (que só
 * passaram a ser suportadas nativamente a partir do Windows 10). */
void ctl_platform_write(FILE *stream, ctl_color_t color, bool use_color, const char *utf8_text);

/* Mutex minimalista multiplataforma (pthread no POSIX, CRITICAL_SECTION
 * no Windows). Implementado como tipo opaco: o conteúdo de 'struct
 * ctl_mutex' é definido separadamente em cada arquivo de plataforma. */
typedef struct ctl_mutex ctl_mutex_t;

ctl_mutex_t *ctl_platform_mutex_create(void);
void ctl_platform_mutex_lock(ctl_mutex_t *m);
void ctl_platform_mutex_unlock(ctl_mutex_t *m);
void ctl_platform_mutex_destroy(ctl_mutex_t *m);

#endif /* CTL_PLATFORM_H */
